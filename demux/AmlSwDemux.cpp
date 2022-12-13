/*
 * Copyright (c) 2020 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */
//#define LOG_NDEBUG 0
#define LOG_TAG "AmlMpPlayerDemo_AmlSwDemux"
#include <utils/AmlMpLog.h>
#include "AmlSwDemux.h"
#include "AmlESQueue.h"
#include <utils/AmlMpUtils.h>
#include <map>
#include <utils/AmlMpEventLooper.h>
#include <utils/AmlMpBuffer.h>
#include <utils/AmlMpMessage.h>
#include <utils/AmlMpEventHandlerReflector.h>
#include <utils/AmlMpBitReader.h>
#include <inttypes.h>
#include <Aml_MP/Aml_MP.h>
#include <fcntl.h>

static const char* mName = LOG_TAG;

namespace aml_mp {

#define ERROR_SIZE (-1)
#define ERROR_CRC  (-2)
#define MY_LOGV(x, y) \
    do { unsigned tmp = y; (void)tmp; MLOGV(x, tmp); } while (0)

const size_t kTSPacketSize = 188;

class SwTsParser: public AmlDemuxBase::ITsParser
{
public:
    enum {
        // From ISO/IEC 13818-1: 2000 (E), Table 2-29
        STREAMTYPE_RESERVED             = 0x00,
        STREAMTYPE_MPEG1_VIDEO          = 0x01,
        STREAMTYPE_MPEG2_VIDEO          = 0x02,
        STREAMTYPE_MPEG1_AUDIO          = 0x03,
        STREAMTYPE_MPEG2_AUDIO          = 0x04,
        STREAMTYPE_PES_PRIVATE_DATA     = 0x06,
        STREAMTYPE_MPEG2_AUDIO_ADTS     = 0x0f,
        STREAMTYPE_MPEG4_VIDEO          = 0x10,
        STREAMTYPE_METADATA             = 0x15,
        STREAMTYPE_H264                 = 0x1b,

        // From ATSC A/53 Part 3:2009, 6.7.1
        STREAMTYPE_AC3                  = 0x81,

        // Stream type 0x83 is non-standard,
        // it could be LPCM or TrueHD AC3
        STREAMTYPE_LPCM_AC3             = 0x83,
        STREAMTYPE_EAC3                 = 0x87,

        //Sample Encrypted types
        STREAMTYPE_H264_ENCRYPTED       = 0xDB,
        STREAMTYPE_AAC_ENCRYPTED        = 0xCF,
        STREAMTYPE_AC3_ENCRYPTED        = 0xC1,
    };

    SwTsParser(const std::function<FilterCallback>& cb);
    ~SwTsParser();
    int feedTs(const uint8_t* buffer, size_t size) override;
    void reset() override;
    int addDemuxFilter(int pid, const Aml_MP_DemuxFilterParams* params) override;
    void removeDemuxFilter(int pid) override;

private:
    struct PSISection;
    struct Program;
    struct Stream;

    void parseAdaptationField(AmlMpBitReader *br, unsigned PID);
    int parseTS(AmlMpBitReader *br);
    void parseProgramAssociationTable(AmlMpBitReader *br);
    int parsePID(AmlMpBitReader* br, unsigned PID, unsigned continuity_counter, unsigned payload_unit_start_indicator);
    int programMapPID() const {return mProgramMapPID;}

    void initCrcTable();
    uint32_t crc32(const uint8_t *p_start, size_t length);

    std::vector<sptr<Program> > mPrograms;
    int mPcrPid = 0x1FFF;
    size_t mNumTSPacketsParsed = 0;

    uint32_t mCrcTable[256];
    std::map<unsigned, sptr<PSISection> > mPSISections;
    unsigned mProgramMapPID = 0x1FFF;

    std::map<unsigned, sptr<Stream>> mStreams;

private:
    SwTsParser(const SwTsParser&);
    SwTsParser& operator= (const SwTsParser&) = delete;
};

struct SwTsParser::PSISection : public AmlMpRefBase {
    PSISection(int pid, SwTsParser* tsParser);

    int append(const void *data, size_t size);
    void clear();

    bool isComplete() const;
    bool isEmpty() const;

    const uint8_t *data() const;
    size_t size() const;
    void setRange(size_t offset, size_t length);
    size_t sectionLength() const;
    const sptr<AmlMpBuffer> rawBuffer() const {return mBuffer;}
    bool isChanged() const {return mChanged;}
    bool needCheckVersionChange();
    int sectionVersion() const;

    bool parse(int PID, unsigned continuity_counter,
                   unsigned payload_unit_start_indicator,
                   AmlMpBitReader *br);
    int parseSection(AmlMpBitReader* br);

protected:
    virtual ~PSISection();

private:
    sptr<AmlMpBuffer> mBuffer;
    int mPid = 0x1FFF;
    SwTsParser* mTsParser = nullptr;
    //int32_t mExpectedContinuityCounter = -1;
    bool mPayloadStarted = false;

    mutable bool mGuessed = false;
    std::map<int, int> mSectionVersions;
    bool mChanged = false;

    //DISALLOW_EVIL_CONSTRUCTORS(PSISection);
	PSISection(const PSISection&) = delete;
	PSISection& operator=(const PSISection&) = delete;
};

struct SwTsParser::Program : public AmlMpRefBase {
    Program(SwTsParser *parser, unsigned programNumber, unsigned programMapPID)
    : mParser(parser)
    , mProgramNumber(programNumber)
    , mProgramMapPID(programMapPID)
    {

    }

    unsigned number() const { return mProgramNumber; }
    void updateProgramMapPID(unsigned programMapPID) {
        mProgramMapPID = programMapPID;
    }

private:
    SwTsParser *mParser __unused;
    unsigned mProgramNumber;
    unsigned mProgramMapPID;

private:
    Program(const Program&) = delete;
    Program& operator=(const Program&) = delete;
};

struct SwTsParser::Stream : public AmlMpRefBase
{
public:
    Stream(int pid, const Aml_MP_DemuxFilterParams* params);
    int parse(unsigned continuity_counter, unsigned payload_unit_start_indicator, AmlMpBitReader* br);
    sptr<AmlMpBuffer> dequeueFrame();

private:
    int flush();
    int parsePES(AmlMpBitReader* br);
    void onPayloadData(unsigned PTS_DTS_flags, uint64_t PTS, uint64_t DTS, const uint8_t* data, size_t size, int32_t payloadOffset);

    int mPid;
    Aml_MP_CodecID mCodecType;
    Aml_MP_DemuxFilterType mType;
    uint32_t mFlags;

    int32_t mExpectedContinuityCounter = -1;
    sptr<AmlMpBuffer> mBuffer;
    bool mPayloadStarted = false;

    sptr<ElementaryStreamQueue> mQueue;
    std::list<sptr<AmlMpBuffer>> mFrames;

    Stream(const Stream&) = delete;
    Stream& operator=(const Stream&) = delete;
};

///////////////////////////////////////////////////////////////////////////////
AmlSwDemux::AmlSwDemux()
: mRemainingBytesBuffer(new AmlMpBuffer(kTSPacketSize))
{
    mRemainingBytesBuffer->setRange(0, 0);
}

AmlSwDemux::~AmlSwDemux()
{
    close();
}

int AmlSwDemux::open(bool isHardwareSource, Aml_MP_DemuxId demuxId, bool isSecureBuffer)
{
    AML_MP_UNUSED(demuxId);
    AML_MP_UNUSED(isSecureBuffer);

    if (isHardwareSource) {
        MLOGE("swdemux don't support hw source!!!");
        return -1;
    }

    if (mTsParser == nullptr) {
        mTsParser = new SwTsParser([this](int pid, const sptr<AmlMpBuffer>& data, int version) {
            return notifyData(pid, data, version);
        });
    }

    return 0;
}

int AmlSwDemux::close()
{
    flush();

    if (mLooper != nullptr) {
        mLooper->unregisterHandler(mHandler->id());
        mLooper->stop();
        mLooper.clear();
    }

    if (mDumpFd >= 0) {
        MLOGI("close dumpfd %d", mDumpFd);
        ::close(mDumpFd);
        mDumpFd = -1;
    }

    return 0;
}

int AmlSwDemux::start()
{
#if 0
    if (mDumpFd < 0) {
        MLOGI("open dump swdemux file!");
        mDumpFd = ::open("/data/dump_swdemux.ts", O_CREAT | O_WRONLY | O_TRUNC, 0644);
        if (mDumpFd < 0) {
            MLOGE("open dump file filed!");
        }
    }
#endif

    if (mHandler == nullptr) {
        mHandler = new AmlMpEventHandlerReflector<AmlSwDemux>(this);
    }

    if (mLooper == nullptr) {
        mLooper = new AmlMpEventLooper;
        mLooper->setName("swDemux");
        mLooper->registerHandler(mHandler);
        int ret = mLooper->start();
        MLOGI("start swDemux looper, ret = %d", ret);
    }

    return 0;
}

int AmlSwDemux::stop()
{
    if (mStopped) {
        return 0;
    }

    mStopped = true;

    return 0;
}

int AmlSwDemux::flush()
{
    ++mBufferGeneration;

    sptr<AmlMpMessage> msg = new AmlMpMessage(kWhatFlush, mHandler);

    sptr<AmlMpMessage> response;
    msg->postAndAwaitResponse(&response);
    MLOG();

    return 0;
}

int AmlSwDemux::feedTs(const uint8_t* buffer, size_t size)
{
    if (mStopped) {
        return -1;
    }

    sptr<AmlMpBuffer> data = new AmlMpBuffer((void*)buffer, size);
    data->setRange(0, size);

    sptr<AmlMpMessage> msg = new AmlMpMessage(kWhatFeedData, mHandler);
    msg->setBuffer("buffer", data);
    msg->setInt32("generation", mBufferGeneration);

    sptr<AmlMpMessage> response;
    msg->postAndAwaitResponse(&response);

    if (mDumpFd >= 0) {
        if (::write(mDumpFd, buffer, size) != (int)size) {
            MLOGE("write dump file failed!!!");
        }
    }

    return size;
}

int AmlSwDemux::addDemuxFilter(int pid, const Aml_MP_DemuxFilterParams* params)
{
    sptr<AmlMpMessage> msg = new AmlMpMessage(kWhatAddPid, mHandler);
    msg->setInt32("pid", pid);
    msg->setInt32("type", params->type);
    msg->setInt32("codec-type", params->codecType);
    msg->setInt32("flags", params->flags);
    msg->post();

    return 0;
}

int AmlSwDemux::removeDemuxFilter(int pid)
{
    sptr<AmlMpMessage> msg = new AmlMpMessage(kWhatRemovePid, mHandler);
    msg->setInt32("pid", pid);
    msg->post();

    return 0;
}

bool AmlSwDemux::isStopped() const
{
    return mStopped.load(std::memory_order_relaxed);
}

void AmlSwDemux::onMessageReceived(const sptr<AmlMpMessage>& msg)
{
    switch (msg->what()) {
    case kWhatFeedData:
    {
        sptr<AmlMpBuffer> data;
        AML_MP_CHECK(msg->findBuffer("buffer", &data));

        sptr<AReplyToken> replyID;
        AML_MP_CHECK(msg->senderAwaitsResponse(&replyID));

        int32_t generation;
        AML_MP_CHECK(msg->findInt32("generation", &generation));

        sptr<AmlMpMessage> response = new AmlMpMessage;
        if (generation != mBufferGeneration) {
            MLOGW("kWhatFeedData break, %d %d", generation, mBufferGeneration.load());
        } else {
            onFeedData(data);
        }

        response->postReply(replyID);
    }
    break;

    case kWhatAddPid:
    {
        int pid = AML_MP_INVALID_PID;
        Aml_MP_DemuxFilterParams params;
        msg->findInt32("pid", &pid);
        msg->findInt32("type", (int32_t*)&params.type);
        msg->findInt32("codec-type", (int32_t*)&params.codecType);
        msg->findInt32("flags", (int32_t*)&params.flags);
        onAddFilterPid(pid, &params);
    }
    break;

    case kWhatRemovePid:
    {
        int pid = AML_MP_INVALID_PID;
        msg->findInt32("pid", &pid);
        onRemoveFilterPid(pid);
    }
    break;

    case kWhatFlush:
    {
        onFlush();

        sptr<AReplyToken> replyID;
        AML_MP_CHECK(msg->senderAwaitsResponse(&replyID));
        sptr<AmlMpMessage> response = new AmlMpMessage;
        response->postReply(replyID);
    }
    break;

    default:
        AML_MP_TRESPASS();
        break;
    }
}

void AmlSwDemux::onFeedData(const sptr<AmlMpBuffer>& entry)
{
    int err = 0;

    if (mRemainingBytesBuffer->size() > 0) {
        //CHECK_LE(mRemainingBytesBuffer->size(), kTSPacketSize);
        size_t paddingSize = kTSPacketSize - mRemainingBytesBuffer->size();
        size_t copySize = paddingSize < entry->size() ? paddingSize : entry->size();

        memcpy(mRemainingBytesBuffer->data() + mRemainingBytesBuffer->size(), entry->data(), copySize);
        mRemainingBytesBuffer->setRange(mRemainingBytesBuffer->offset(), mRemainingBytesBuffer->size() + copySize);
        entry->setRange(entry->offset() + copySize, entry->size() - copySize);

        if (mRemainingBytesBuffer->size() == kTSPacketSize) {
            err = mTsParser->feedTs(mRemainingBytesBuffer->data(), kTSPacketSize);
            if (err != 0) {
                const uint8_t* p = mRemainingBytesBuffer->data();
                MLOGI("%d %" PRId64 ", feedTSPacket err:%d, %#x, %#x, %#x, %#x, %#x", __LINE__, mOutBufferCount.load(), err,
                        p[0], p[1], p[2], p[3], p[4]);
            }
            mRemainingBytesBuffer->setRange(0, 0);
        }
    }

    while (entry->size() > 0) {
        if (entry->size() < kTSPacketSize) {
            if (*entry->data() == 0x47) {
                memcpy(mRemainingBytesBuffer->data(), entry->data(), entry->size());
                mRemainingBytesBuffer->setRange(mRemainingBytesBuffer->offset(), mRemainingBytesBuffer->size() + entry->size());
            } else {
                MLOGW("left data doesn't start with 0x47, discard it!");
            }
            break;
        }

        if (*entry->data() != 0x47) {
            MLOGI("mOutBufferCount:%" PRId64 ", entry start bytes:%#x, offset:%zu, size:%zu",
                  mOutBufferCount.load(), *entry->data(), entry->offset(), entry->size());
            //const uint8_t* p = entry->data();
            //MLOGV("entry bytes: %#x %#x %#x %#x %#x", p[0], p[1], p[2], p[3], p[4]);

            err = resync(entry);
            if (err < 0) {
                MLOGE("resync ts buffer failed! %d, size:%zu", err, entry->size());
                return;
            } else {
                MLOGV("resync add offset:%d", err);
                continue;
            }
        }

        err = mTsParser->feedTs(entry->data(), kTSPacketSize);
        if (err != 0) {
            MLOGE("%d feedTSPacket failed, err:%d", __LINE__, err);
        }
        entry->setRange(entry->offset() + kTSPacketSize, entry->size() - kTSPacketSize);
    }
}

int AmlSwDemux::resync(const sptr<AmlMpBuffer>& buffer)
{
    const uint8_t* p = buffer->data();
    size_t size = buffer->size();

    if (p == nullptr || size < kTSPacketSize)
        return -1;

    bool synced = false;
    size_t offset = 0;
    while (offset < size) {
        if (p[offset] != 0x47) {
            ++offset;
            continue;
        }

        if (offset + kTSPacketSize < size) {
            if (p[offset + kTSPacketSize] != 0x47) {
                ++offset;
                MLOGV("next ts packet header check failed, try sync again...");
                continue;
            }
        }

        synced = true;
        break;
    }

    if (synced) {
        buffer->setRange(buffer->offset() + offset, buffer->size() - offset);
        return offset;
    }

    return -1;
}

void AmlSwDemux::onFlush()
{
    mOutBufferCount = 0;

    if (mTsParser != nullptr) {
        mTsParser->reset();
    }

    mRemainingBytesBuffer->setRange(0, 0);
}

void AmlSwDemux::onAddFilterPid(int pid, Aml_MP_DemuxFilterParams* params)
{
    if (mTsParser != nullptr) {
        MLOGI("add filter pid:%d(%#x)", pid, pid);
        mTsParser->addDemuxFilter(pid, params);
    }
}

void AmlSwDemux::onRemoveFilterPid(int pid)
{
    if (mTsParser != nullptr) {
        MLOGI("remove filter pid:%d(%#x)", pid, pid);
        mTsParser->removeDemuxFilter(pid);
    }
}

///////////////////////////////////////////////////////////////////////////////
SwTsParser::SwTsParser(const std::function<FilterCallback>& cb)
: ITsParser(cb)
{
    MLOG();

    mPSISections.emplace(0 /* PID */, new PSISection(0, this));
    mPSISections.emplace(1 /* PID */, new PSISection(1, this));
    initCrcTable();
}

SwTsParser::~SwTsParser()
{
    MLOG();
}

void SwTsParser::initCrcTable()
{
#if 0
    uint32_t poly = 0x04C11DB7;

    for (int i = 0; i < 256; i++) {
        uint32_t crc = i << 24;
        for (int j = 0; j < 8; j++) {
            crc = (crc << 1) ^ ((crc & 0x80000000) ? (poly) : 0);
        }
        mCrcTable[i] = crc;
    }
#endif

    for (uint32_t i = 0; i < 256; i++) {
        uint32_t k = 0;
        for (uint32_t j = (i << 24) | 0x800000; j != 0x80000000; j <<= 1) {
            k = (k << 1) ^ (((k ^ j) & 0x80000000) ? 0x04c11db7 : 0);
        }

        mCrcTable[i] = k;
    }
}

/**
 * Compute CRC32 checksum for buffer starting at offset start and for length
 * bytes.
 */
uint32_t SwTsParser::crc32(const uint8_t *p_start, size_t length)
{
#if 0
    uint32_t crc = 0xFFFFFFFF;
    const uint8_t *p;

    for (p = p_start; p < p_start + length; p++) {
        crc = (crc << 8) ^ mCrcTable[((crc >> 24) ^ *p) & 0xFF];
    }

    return crc;
#endif

    uint32_t crc32_reg = 0xFFFFFFFF;
    for (uint32_t i = 0; i < length; i++) {
        crc32_reg = (crc32_reg << 8) ^ mCrcTable[((crc32_reg >> 24) ^ *p_start++) & 0xFF];
    }
    return crc32_reg;
}

int SwTsParser::feedTs(const uint8_t* buffer, size_t size)
{
    AML_MP_UNUSED(size);
    //CHECK_EQ(size, kTSPacketSize);

    AmlMpBitReader br(buffer, kTSPacketSize);
    return parseTS(&br);
}

void SwTsParser::reset()
{
    MLOG();

    //for (size_t i = 0; i < mPSISections.size(); ++i) {
        //sptr<PSISection> &p = mPSISections.editValueAt(i);
        //p->clear();
    //}

    for (auto& p : mPSISections) {
        p.second->clear();
    }
}

int SwTsParser::addDemuxFilter(int pid, const Aml_MP_DemuxFilterParams* params)
{
    if (pid == 0x1FFF)
        return -1;

    if (params->type == AML_MP_DEMUX_FILTER_PSI) {
        if (mPSISections.find(pid) == mPSISections.end()) {
            MLOGW("add section pid:%d(%#x)", pid, pid);
            mPSISections.emplace(pid, new PSISection(pid, this));
        }
    } else {
        if (mStreams.find(pid) == mStreams.end()) {
            MLOGW("add pes filter:%d(%#x)", pid, pid);
            mStreams.emplace(pid, new Stream(pid, params));
        }
    }


    return 0;
}

void SwTsParser::removeDemuxFilter(int pid)
{
    if (pid == 0x1FFF)
        return;

    if (mPSISections.find(pid) != mPSISections.end()) {
        MLOGW("remove section pid:%d(%#x)", pid, pid);
        mPSISections.erase(pid);
    } else if (mStreams.find(pid) != mStreams.end()) {
        MLOGW("remove pes pid:%d(%#x)", pid, pid);
        mStreams.erase(pid);
    }
}

void SwTsParser::parseAdaptationField(AmlMpBitReader *br, unsigned PID)
{
    unsigned adaptation_field_length = br->getBits(8);

    if (adaptation_field_length > 0) {
        unsigned discontinuity_indicator = br->getBits(1);

        if (discontinuity_indicator) {
            MLOGV("PID 0x%04x: discontinuity_indicator = 1 (!!!)", PID);
        }

        br->skipBits(2);
        unsigned PCR_flag = br->getBits(1);

        size_t numBitsRead = 4;

        if (PCR_flag && (mPcrPid == 0x1FFF || mPcrPid == (int)PID)) {
            br->skipBits(4);
            uint64_t PCR_base = br->getBits(32);
            PCR_base = (PCR_base << 1) | br->getBits(1);

            //PCR_base = recoverPTS(PCR_base, mLastRecoveredPCR);

            br->skipBits(6);
            unsigned PCR_ext = br->getBits(9);
            (void)PCR_ext;

            // The number of bytes from the start of the current
            // MPEG2 transport stream packet up and including
            // the final byte of this PCR_ext field.
            //size_t byteOffsetFromStartOfTSPacket =
                //(188 - br->numBitsLeft() / 8);

            //int64_t PCR = PCR_base * 300 + PCR_ext;

            //MLOGV("PID 0x%04x: PCR = 0x%016" PRIx64 " (%.2f)",
                  //PID, PCR, PCR / 27E6);

            // The number of bytes received by this parser up to and
            // including the final byte of this PCR_ext field.
            //size_t byteOffsetFromStart =
                //mNumTSPacketsParsed * 188 + byteOffsetFromStartOfTSPacket;

            //for (size_t i = 0; i < mPrograms.size(); ++i) {
                //updatePCR(PID, PCR, byteOffsetFromStart);
            //}

            numBitsRead += 52;
        }

        //CHECK_GE(adaptation_field_length * 8, numBitsRead);

        br->skipBits(adaptation_field_length * 8 - numBitsRead);
    }
}

int SwTsParser::parseTS(AmlMpBitReader *br)
{
    MLOGV("---");

    unsigned sync_byte = br->getBits(8);
    if (sync_byte != 0x47u) {
        MLOGE("[error] parseTS: return error as sync_byte=0x%x", sync_byte);
        return -EINVAL;
    }

    if (br->getBits(1)) {  // transport_error_indicator
        // silently ignore.
        return 0;
    }

    unsigned payload_unit_start_indicator = br->getBits(1);
    MLOGV("payload_unit_start_indicator = %u", payload_unit_start_indicator);

    MY_LOGV("transport_priority = %u", br->getBits(1));

    unsigned PID = br->getBits(13);
    MLOGV("PID = 0x%04x", PID);

    MY_LOGV("transport_scrambling_control = %u", br->getBits(2));

    unsigned adaptation_field_control = br->getBits(2);
    MLOGV("adaptation_field_control = %u", adaptation_field_control);

    unsigned continuity_counter = br->getBits(4);
    MLOGV("PID = 0x%04x, continuity_counter = %u", PID, continuity_counter);

    // MLOGI("PID = 0x%04x, continuity_counter = %u", PID, continuity_counter);

    if (adaptation_field_control == 2 || adaptation_field_control == 3) {
        parseAdaptationField(br, PID);
    }

    int err = 0;

    if (adaptation_field_control == 1 || adaptation_field_control == 3) {
        err = parsePID(
                br, PID, continuity_counter, payload_unit_start_indicator);
    }

    ++mNumTSPacketsParsed;

    return err;
}

void SwTsParser::parseProgramAssociationTable(AmlMpBitReader *br)
{
    unsigned table_id = br->getBits(8);
    MLOGV("  table_id = %u", table_id);
    if (table_id != 0x00u) {
        MLOGE("PAT data error!");
        return ;
    }
    unsigned section_syntax_indictor = br->getBits(1);
    MY_LOGV("  section_syntax_indictor = %u", section_syntax_indictor);
    //CHECK_EQ(section_syntax_indictor, 1u);

    //CHECK_EQ(br->getBits(1), 0u);
    (br->getBits(1), 0u);
    MY_LOGV("  reserved = %u", br->getBits(2));

    unsigned section_length = br->getBits(12);
    MLOGV("  section_length = %u", section_length);
    //CHECK_EQ(section_length & 0xc00, 0u);

    MY_LOGV("  transport_stream_id = %u", br->getBits(16));
    MY_LOGV("  reserved = %u", br->getBits(2));
    MY_LOGV("  version_number = %u", br->getBits(5));
    MY_LOGV("  current_next_indicator = %u", br->getBits(1));
    MY_LOGV("  section_number = %u", br->getBits(8));
    MY_LOGV("  last_section_number = %u", br->getBits(8));

    size_t numProgramBytes = (section_length - 5 /* header */ - 4 /* crc */);
    //CHECK_EQ((numProgramBytes % 4), 0u);

    for (size_t i = 0; i < numProgramBytes / 4; ++i) {
        unsigned program_number = br->getBits(16);
        MLOGV("    program_number = %u", program_number);

        MY_LOGV("    reserved = %u", br->getBits(3));

        if (program_number == 0) {
            MY_LOGV("    network_PID = 0x%04x", br->getBits(13));
        } else {
            unsigned programMapPID = br->getBits(13);

            MLOGV("    program_map_PID = 0x%04x, %d", programMapPID, programMapPID);

            if (mProgramMapPID == 0x1FFF) {
                MLOGE("first update programMapPID:%d", programMapPID);
            }
            mProgramMapPID = programMapPID;

            bool found = false;
            for (size_t index = 0; index < mPrograms.size(); ++index) {
                const sptr<Program> &program = mPrograms.at(index);

                if (program->number() == program_number) {
                    program->updateProgramMapPID(programMapPID);
                    found = true;
                    break;
                }
            }

            if (!found) {
                mPrograms.push_back(
                        new Program(this, program_number, programMapPID));
            }

            if (mPSISections.find(programMapPID) == mPSISections.end()) {
                mPSISections.emplace(programMapPID, new PSISection(programMapPID, this));
            }
        }
    }

    MY_LOGV("  CRC = 0x%08x", br->getBits(32));
}

int SwTsParser::parsePID(
        AmlMpBitReader *br, unsigned PID,
        unsigned continuity_counter,
        unsigned payload_unit_start_indicator) {
    auto sectionIndex = mPSISections.find(PID);

    if (sectionIndex != mPSISections.end()) {
        sptr<PSISection> section = mPSISections.at(PID);

        if (!section->parse(PID, continuity_counter, payload_unit_start_indicator, br)) {
            MLOGW("pre parse failed!!!! PID = %d", PID);
            return 0;
        }

        AML_MP_CHECK((br->numBitsLeft() % 8) == 0);
        int err = section->append(br->data(), br->numBitsLeft() / 8);

        if (err != 0) {
            MLOGW("section append data %zu size failed!", br->numBitsLeft()/8);
            return err;
        }

        if (!section->isComplete()) {
            return 0;
        }

        AmlMpBitReader sectionBits2(section->data(), section->size());
        bool notifyListener = true;

        err = section->parseSection(&sectionBits2);
        if (err != ERROR_SIZE) {
            section->setRange(0, section->sectionLength());
        }

        if (err != 0) {
            notifyListener = false;

            MLOGE("parse failed:%d, section pid:%d, buffer size:%zu, section length:%zu", err, PID,
                    section->size(), section->sectionLength());
#if 0
            //
            MLOGE("Dump data:");
            hexdump(section->data(), std::min(section->size(), (size_t)184));

            char path[50];
            sprintf(path, "/data/crc_err_%d.bin", PID);
            int fd = ::open(path, O_RDONLY);
            if (fd < 0) {
                MLOGI("dump to %s", path);
                FILE* fp = fopen(path, "w");
                if (fp == NULL) {
                    MLOGE("open %s failed!", path);
                } else {
                    fwrite(section->data(), 1, section->size(), fp);
                    fclose(fp);
                }
            } else {
                ::close(fd);
            }
            //
#endif

#if 0
            if (err == ERROR_CRC) {
                notifyListener = true;
                MLOGW("crc error ignored!");
            }
#endif

        } else {
            MLOGV("section pid:%d, set range:%zu", PID, section->sectionLength());
            //section->setRange(0, section->sectionLength());
        }

        if (section != NULL) {
            if (notifyListener) {
                int version = section->needCheckVersionChange() ? section->sectionVersion() : -1;
                mFilterCallback(PID, section->rawBuffer(), version);
            }

            //section->clear();
        }


        AmlMpBitReader sectionBits(section->data(), section->size());

        if (PID == 0) {
            parseProgramAssociationTable(&sectionBits);
        } else {
#if 0
            bool handled = false;
            for (size_t i = 0; i < mPrograms.size(); ++i) {
                status_t err;
                if (!mPrograms.editItemAt(i)->parsePSISection(
                            PID, &sectionBits, &err)) {
                    continue;
                }

                if (err != 0) {
                    return err;
                }

                handled = true;
                break;
            }

            if (!handled) {
                //mPSISections.removeItem(PID);
                //section.clear();

            }
#endif
        }

        if (section != NULL) {
            section->clear();
        }

        return 0;
    }

    auto streamIndex = mStreams.find(PID);
    if (streamIndex != mStreams.end()) {
        sptr<Stream> stream = streamIndex->second;
        int err = stream->parse(continuity_counter, payload_unit_start_indicator, br);
        if (err != 0) {
            return err;
        }

        sptr<AmlMpBuffer> buffer;
        while ((buffer = stream->dequeueFrame()) != nullptr) {
            mFilterCallback(PID, buffer, -1);
        }
    }

    return 0;
}

////////////////////////////////////////////////////////////////////////////////
static inline uint16_t U16_AT(const uint8_t *ptr) {
    return ptr[0] << 8 | ptr[1];
}

SwTsParser::PSISection::PSISection(int pid, SwTsParser* tsParser)
: mPid(pid)
, mTsParser(tsParser)
{
}

SwTsParser::PSISection::~PSISection() {
}

int SwTsParser::PSISection::append(const void *data, size_t size) {
    if (mBuffer == NULL || mBuffer->size() + size > mBuffer->capacity()) {
        size_t newCapacity =
            (mBuffer == NULL) ? size : mBuffer->capacity() + size;

        newCapacity = (newCapacity + 1023) & ~1023;

        sptr<AmlMpBuffer> newBuffer = new AmlMpBuffer(newCapacity);

        if (mBuffer != NULL) {
            memcpy(newBuffer->data(), mBuffer->data(), mBuffer->size());
            newBuffer->setRange(0, mBuffer->size());
        } else {
            newBuffer->setRange(0, 0);
        }

        mBuffer = newBuffer;
    }

    memcpy(mBuffer->data() + mBuffer->size(), data, size);
    mBuffer->setRange(0, mBuffer->size() + size);

    return 0;
}

void SwTsParser::PSISection::clear() {
    if (mBuffer != NULL) {
        mBuffer->setRange(0, 0);
    }

    mChanged = false;
}

bool SwTsParser::PSISection::isComplete() const {
    if (mBuffer == NULL || mBuffer->size() < 3) {
        return false;
    }

    unsigned sectionLength = U16_AT(mBuffer->data() + 1) & 0xfff;
    if (sectionLength > 4093) {
        MLOGE("section size error:%d", sectionLength);
        return true;
    }

    return mBuffer->size() >= sectionLength + 3;
}

bool SwTsParser::PSISection::isEmpty() const {
    return mBuffer == NULL || mBuffer->size() == 0;
}

const uint8_t *SwTsParser::PSISection::data() const {
    return mBuffer == NULL ? NULL : mBuffer->data();
}

size_t SwTsParser::PSISection::size() const {
    return mBuffer == NULL ? 0 : mBuffer->size();
}

void SwTsParser::PSISection::setRange(size_t offset, size_t length)
{
    if (mBuffer == nullptr || mBuffer->capacity() < length) {
        MLOGE("section setRange failed! length:%zu", length);
        return;
    }

    mBuffer->setRange(offset, length);
}

size_t SwTsParser::PSISection::sectionLength() const {
    if (mBuffer == NULL || mBuffer->size() < 3) {
        return 0;
    }

    unsigned sectionLength = U16_AT(mBuffer->data() + 1) & 0xfff;
    return sectionLength + 3;
}

bool SwTsParser::PSISection::needCheckVersionChange()
{
    int pid = mPid;
    int programMapPID = mTsParser->programMapPID();

    return pid == 0 || pid == 1 || (programMapPID!=0x1FFF && programMapPID==pid);
}

int SwTsParser::PSISection::sectionVersion() const
{
    auto index = mSectionVersions.find(0);
    if (index == mSectionVersions.end())
        return -1;

    int version = mSectionVersions.at(0);
    return version;
}

bool SwTsParser::PSISection::parse(int PID, unsigned continuity_counter,
                   unsigned payload_unit_start_indicator,
                   AmlMpBitReader *br)
{
    (void)PID;
    (void)continuity_counter;
#if 0
    if (mExpectedContinuityCounter >= 0 && (unsigned)mExpectedContinuityCounter != continuity_counter) {
        MLOGW("section discontinuity on stream pid 0x%04x(%d)", PID, PID);

        mPayloadStarted = false;
        mBuffer->setRange(0, 0);
        mExpectedContinuityCounter = -1;

        if (!payload_unit_start_indicator) {
            return false;
        }
    }

    mExpectedContinuityCounter = (continuity_counter + 1) & 0x0f;
#endif

    if (payload_unit_start_indicator) {
        // Otherwise we run the danger of receiving the trailing bytes
        // of a section packet that we never saw the start of and assuming
        // we have a a complete section packet.
        if (!isEmpty()) {
            MLOGW("parsePID encounters payload_unit_start_indicator when section is not empty");
            clear();
        }

        unsigned skip = br->getBits(8);
        if (skip > 0) {
            MLOGW("skip %d bytes!", skip);
        }
        br->skipBits(skip * 8);

        mPayloadStarted = true;
    }

    if (!mPayloadStarted) {
        return false;
    }

    return true;
}

int SwTsParser::PSISection::parseSection(AmlMpBitReader* br)
{
    unsigned table_id = br->getBits(8);
    MY_LOGV("  table_id = %u", table_id);

    unsigned section_syntax_indicator = br->getBits(1);
    MY_LOGV("  section_syntax_indicator = %u", section_syntax_indicator);

    unsigned private_indicator =  br->getBits(1);

    if ((section_syntax_indicator ^ private_indicator) != 1) {
        if (!mGuessed) {
            MLOGW("HAVE DSM-CC data!!!");
        }
        mGuessed = true;
    }

    MY_LOGV("  reserved = %u", br->getBits(2));

    unsigned section_length = br->getBits(12);
    MLOGV("  section_length = %u", section_length);
    //CHECK_EQ(section_length & 0xc00, 0u);
    //CHECK_LE(section_length, 4093u);
    if (section_length > 4093) {
        return ERROR_SIZE;
    }

    if (section_syntax_indicator) {
        br->skipBits(18);
        int versionNumber = br->getBits(5);
        MY_LOGV("  version_number = %u", versionNumber);
        //MY_LOGV("  version_number = %u", br->getBits(5));
        MY_LOGV("  current_next_indicator = %u", br->getBits(1));
        int sectionNumber = br->getBits(8);
        MY_LOGV("  section_number = %u", sectionNumber);
        MY_LOGV("  last_section_number = %u", br->getBits(8));

        if (needCheckVersionChange()) {
            auto index = mSectionVersions.find(sectionNumber);
            if (index == mSectionVersions.end()) {
                mChanged = true;
                mSectionVersions.emplace(sectionNumber, versionNumber);
                MLOGE("section FIRST changed, PID:%d, sectionNumber:%d, versionNumber:%d", mPid, sectionNumber, versionNumber);
            } else {
                int &lastVersionNumber = mSectionVersions.at(sectionNumber);
                if (lastVersionNumber != versionNumber) {
                    MLOGE("section changed, PID:%d, sectionNumber:%d, versionNumber:%d, lastVersionNumber:%d", mPid, sectionNumber, versionNumber, lastVersionNumber);
                    mChanged = true;
                    lastVersionNumber = versionNumber;
                }
            }
        }

        uint32_t crc = mTsParser->crc32(mBuffer->data(), section_length + 3);
        if (crc != 0) {
            MLOGE("crc error: %#x", crc);
            return ERROR_CRC;
        }
    }

    return 0;
}

////////////////////////////////////////////////////////////////////////////////
static const size_t kInitialStreamBufferSize = 1 * 1024 * 1024;

SwTsParser::Stream::Stream(int pid, const Aml_MP_DemuxFilterParams* params)
{
    mPid = pid;
    mCodecType = params->codecType;
    mType = params->type;
    mFlags = params->flags;

    ElementaryStreamQueue::Mode mode = ElementaryStreamQueue::INVALID;

    switch (mCodecType) {
    case AML_MP_VIDEO_CODEC_H264:
        mode = ElementaryStreamQueue::H264;
        break;

    case AML_MP_VIDEO_CODEC_HEVC:
        mode = ElementaryStreamQueue::H265;
        break;

    case AML_MP_AUDIO_CODEC_AAC:
        mode = ElementaryStreamQueue::AAC;
        break;

    case AML_MP_AUDIO_CODEC_MP2:
    case AML_MP_AUDIO_CODEC_MP3:
        mode = ElementaryStreamQueue::MPEG_AUDIO;
        break;

    case AML_MP_VIDEO_CODEC_MPEG12:
        mode = ElementaryStreamQueue::MPEG_VIDEO;
        break;

    case AML_MP_VIDEO_CODEC_MPEG4:
        mode = ElementaryStreamQueue::MPEG4_VIDEO;
        break;

    case AML_MP_AUDIO_CODEC_AC3:
        mode = ElementaryStreamQueue::AC3;
        break;

    case AML_MP_AUDIO_CODEC_EAC3:
        mode = ElementaryStreamQueue::EAC3;
        break;

    default:
        ALOGE("stream PID 0x%02x has invalid codec type %s", mPid, mpCodecId2Str(mCodecType));
        return;
    }

    mQueue = new ElementaryStreamQueue(mode);

    mBuffer = new AmlMpBuffer(kInitialStreamBufferSize);
    mBuffer->setRange(0, 0);
}

int SwTsParser::Stream::parse(unsigned continuity_counter, unsigned payload_unit_start_indicator, AmlMpBitReader* br)
{
    if (mExpectedContinuityCounter >= 0 && (unsigned)mExpectedContinuityCounter != continuity_counter) {
        MLOGI("discontinuity on stream pid 0x%04x, continuity_counter:%u", mPid, continuity_counter);
        mPayloadStarted = false;
        mBuffer->setRange(0, 0);
        mExpectedContinuityCounter = -1;

        if (!payload_unit_start_indicator) {
            return 0;
        }
    }

    mExpectedContinuityCounter = (continuity_counter + 1) & 0x0f;

    if (payload_unit_start_indicator) {
        if (mPayloadStarted) {
            int err = flush();
            if (err != 0) {
                MLOGW("Error (%08x) happened while flushing; we simply discard "
                                  "the PES packet and continue.", err);
            }
        }

        mPayloadStarted = true;
    }

    if (!mPayloadStarted) {
        return 0;
    }

    size_t payloadSizeBits = br->numBitsLeft();
    if (payloadSizeBits % 8 != 0u) {
        MLOGE("wrong value!");
        return -1;
    }

    size_t neededSize = mBuffer->size() + payloadSizeBits/8;
    if (mBuffer == nullptr || neededSize > mBuffer->capacity()) {
        neededSize = (neededSize + 65535) & ~65535;

        sptr<AmlMpBuffer> newBuffer = new AmlMpBuffer(neededSize);
        if (mBuffer != nullptr) {
            memcpy(newBuffer->data(), mBuffer->data(), mBuffer->size());
            newBuffer->setRange(0, mBuffer->size());
        } else {
            newBuffer->setRange(0, 0);
        }
        mBuffer = newBuffer;
    }

    memcpy(mBuffer->data() + mBuffer->size(), br->data(), payloadSizeBits / 8);
    mBuffer->setRange(0, mBuffer->size() + payloadSizeBits / 8);

    return 0;
}

int SwTsParser::Stream::flush()
{
    int err = 0;

    if (mBuffer == nullptr || mBuffer->size() == 0) {
        return 0;
    }

#if 0
    std::string result;
    hexdump(mBuffer->data(), std::min(mBuffer->size(), (size_t)16), result);
    MLOGI("flush:%s", result.c_str());
#endif

    AmlMpBitReader br(mBuffer->data(), mBuffer->size());
    err = parsePES(&br);
    mBuffer->setRange(0, 0);

    return err;
}

int SwTsParser::Stream::parsePES(AmlMpBitReader* br)
{
    const uint8_t* basePtr = br->data();

    unsigned packet_startcode_prefix = br->getBits(24);
    if (packet_startcode_prefix != 1) {
        MLOGE("Supposedly payload_unit_start=1 unit does not start "
            "with startcode.");

        return -1;
    }

    unsigned stream_id = br->getBits(8);
    MLOGV("stream_id = 0x%02x", stream_id);

    unsigned PES_packet_length = br->getBits(16);
    MLOGV("PES_packet_length = %u", PES_packet_length);

    if (stream_id != 0xbc  // program_stream_map
            && stream_id != 0xbe  // padding_stream
            && stream_id != 0xbf  // private_stream_2
            && stream_id != 0xf0  // ECM
            && stream_id != 0xf1  // EMM
            && stream_id != 0xff  // program_stream_directory
            && stream_id != 0xf2  // DSMCC
            && stream_id != 0xf8) {  // H.222.1 type E
        if (br->getBits(2) != 2u) {
            return -1;
        }

        unsigned PES_scrambling_control = br->getBits(2);
        MLOGV("PES_scrambling_control = %u", PES_scrambling_control);

        MY_LOGV("PES_priority = %u", br->getBits(1));
        MY_LOGV("data_alignment_indicator = %u", br->getBits(1));
        MY_LOGV("copyright = %u", br->getBits(1));
        MY_LOGV("original_or_copy = %u", br->getBits(1));

        unsigned PTS_DTS_flags = br->getBits(2);
        ALOGV("PTS_DTS_flags = %u", PTS_DTS_flags);

        unsigned ESCR_flag = br->getBits(1);
        ALOGV("ESCR_flag = %u", ESCR_flag);

        unsigned ES_rate_flag = br->getBits(1);
        ALOGV("ES_rate_flag = %u", ES_rate_flag);

        unsigned DSM_trick_mode_flag = br->getBits(1);
        ALOGV("DSM_trick_mode_flag = %u", DSM_trick_mode_flag);

        unsigned additional_copy_info_flag = br->getBits(1);
        ALOGV("additional_copy_info_flag = %u", additional_copy_info_flag);

        MY_LOGV("PES_CRC_flag = %u", br->getBits(1));
        MY_LOGV("PES_extension_flag = %u", br->getBits(1));

        unsigned PES_header_data_length = br->getBits(8);
        ALOGV("PES_header_data_length = %u", PES_header_data_length);

        unsigned optional_bytes_remaining = PES_header_data_length;

        uint64_t PTS = 0, DTS = 0;

        if (PTS_DTS_flags == 2 || PTS_DTS_flags == 3) {
            if (optional_bytes_remaining < 5u) {
                return -1;
            }

            if (br->getBits(4) != PTS_DTS_flags) {
                return -1;
            }
            PTS = ((uint64_t)br->getBits(3)) << 30;
            if (br->getBits(1) != 1u) {
                return -1;
            }
            PTS |= ((uint64_t)br->getBits(15)) << 15;
            if (br->getBits(1) != 1u) {
                return -1;
            }
            PTS |= br->getBits(15);
            if (br->getBits(1) != 1u) {
                return -1;
            }

            ALOGV("(%d) PTS = 0x%016" PRIx64 " (%.2f)", mPid, PTS, PTS / 90000.0);

            optional_bytes_remaining -= 5;

            if (PTS_DTS_flags == 3) {
                if (optional_bytes_remaining < 5u) {
                    return -1;
                }

                if (br->getBits(4) != 1u) {
                    return -1;
                }

                DTS = ((uint64_t)br->getBits(3)) << 30;
                if (br->getBits(1) != 1u) {
                    return -1;
                }
                DTS |= ((uint64_t)br->getBits(15)) << 15;
                if (br->getBits(1) != 1u) {
                    return -1;
                }
                DTS |= br->getBits(15);
                if (br->getBits(1) != 1u) {
                    return -1;
                }

                ALOGV("DTS = %" PRIu64, DTS);

                optional_bytes_remaining -= 5;
            }
        }

        if (ESCR_flag) {
            if (optional_bytes_remaining < 6u) {
                return -1;
            }

            br->getBits(2);

            uint64_t ESCR = ((uint64_t)br->getBits(3)) << 30;
            if (br->getBits(1) != 1u) {
                return -1;
            }
            ESCR |= ((uint64_t)br->getBits(15)) << 15;
            if (br->getBits(1) != 1u) {
                return -1;
            }
            ESCR |= br->getBits(15);
            if (br->getBits(1) != 1u) {
                return -1;
            }

            ALOGV("ESCR = %" PRIu64, ESCR);
            MY_LOGV("ESCR_extension = %u", br->getBits(9));

            if (br->getBits(1) != 1u) {
                return -1;
            }

            optional_bytes_remaining -= 6;
        }

        if (ES_rate_flag) {
            if (optional_bytes_remaining < 3u) {
                return -1;
            }

            if (br->getBits(1) != 1u) {
                return -1;
            }
            MY_LOGV("ES_rate = %u", br->getBits(22));
            if (br->getBits(1) != 1u) {
                return -1;
            }

            optional_bytes_remaining -= 3;
        }

        br->skipBits(optional_bytes_remaining * 8);

        // ES data follows.
        int32_t pesOffset = br->data() - basePtr;

        if (PES_packet_length != 0) {
            if (PES_packet_length < PES_header_data_length + 3) {
                return -1;
            }

            unsigned dataLength =
                PES_packet_length - 3 - PES_header_data_length;

            if (br->numBitsLeft() < dataLength * 8) {
                ALOGE("PES packet does not carry enough data to contain "
                     "payload. (numBitsLeft = %zu, required = %u)",
                     br->numBitsLeft(), dataLength * 8);

                return -1;
            }

            ALOGV("There's %u bytes of payload, PES_packet_length=%u, offset=%d",
                    dataLength, PES_packet_length, pesOffset);

            onPayloadData(
                    PTS_DTS_flags, PTS, DTS,
                    br->data(), dataLength, pesOffset);

            br->skipBits(dataLength * 8);
        } else {
            onPayloadData(
                    PTS_DTS_flags, PTS, DTS,
                    br->data(), br->numBitsLeft() / 8, pesOffset);

            size_t payloadSizeBits = br->numBitsLeft();
            if (payloadSizeBits % 8 != 0u) {
                return -1;
            }

            ALOGV("There's %zu bytes of payload, offset=%d",
                    payloadSizeBits / 8, pesOffset);
        }
    } else if (stream_id == 0xbe) {  // padding_stream
        if (PES_packet_length == 0u) {
            return -1;
        }
        br->skipBits(PES_packet_length * 8);
    } else {
        if (PES_packet_length == 0u) {
            return -1;
        }
        br->skipBits(PES_packet_length * 8);
    }

    return 0;
}


void SwTsParser::Stream::onPayloadData(unsigned PTS_DTS_flags __unused, uint64_t PTS, uint64_t DTS __unused, const uint8_t* data, size_t size, int32_t payloadOffset)
{
    int err = mQueue->appendData(data, size, PTS, payloadOffset);
    if (err != 0) {
        MLOGE("append data failed!");
        return;
    }

    sptr<AmlMpBuffer> accessUnit;
    while ((accessUnit = mQueue->dequeueAccessUnit()) != nullptr) {
        mFrames.push_back(accessUnit);
    }
}

sptr<AmlMpBuffer> SwTsParser::Stream::dequeueFrame()
{
    sptr<AmlMpBuffer> accessUnit;

    if (mFrames.empty()) {
        return nullptr;
    }

    accessUnit = *mFrames.begin();
    mFrames.pop_front();

    return accessUnit;
}

}
