/*
 * Copyright (c) 2020 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

//#define LOG_NDEBUG 0
#define LOG_TAG "AmlMpPlayerDemo_ESQueue"
#include <utils/AmlMpLog.h>
#include <utils/AmlMpBuffer.h>
#include <utils/AmlMpBitReader.h>
#include <utils/AmlMpUtils.h>
#include <vector>
#include "AmlESQueue.h"

static const char* mName = LOG_TAG;

namespace aml_mp {
static const int kPesHeaderSize = 14;


static bool IsSeeminglyValidADTSHeader(
        const uint8_t *ptr, size_t size, size_t *frameLength) {
    if (size < 7) {
        // Not enough data to verify header.
        return false;
    }

    if (ptr[0] != 0xff || (ptr[1] >> 4) != 0x0f) {
        return false;
    }

    unsigned layer = (ptr[1] >> 1) & 3;

    if (layer != 0) {
        return false;
    }

    unsigned ID = (ptr[1] >> 3) & 1;
    unsigned profile_ObjectType = ptr[2] >> 6;

    if (ID == 1 && profile_ObjectType == 3) {
        // MPEG-2 profile 3 is reserved.
        return false;
    }

    size_t frameLengthInHeader =
            ((ptr[3] & 3) << 11) + (ptr[4] << 3) + ((ptr[5] >> 5) & 7);
    if (frameLengthInHeader > size) {
        return false;
    }

    *frameLength = frameLengthInHeader;
    return true;
}

static bool IsSeeminglyValidMPEGAudioHeader(const uint8_t *ptr, size_t size) {
    if (size < 3) {
        // Not enough data to verify header.
        return false;
    }

    if (ptr[0] != 0xff || (ptr[1] >> 5) != 0x07) {
        return false;
    }

    unsigned ID = (ptr[1] >> 3) & 3;

    if (ID == 1) {
        return false;  // reserved
    }

    unsigned layer = (ptr[1] >> 1) & 3;

    if (layer == 0) {
        return false;  // reserved
    }

    unsigned bitrateIndex = (ptr[2] >> 4);

    if (bitrateIndex == 0x0f) {
        return false;  // reserved
    }

    unsigned samplingRateIndex = (ptr[2] >> 2) & 3;

    if (samplingRateIndex == 3) {
        return false;  // reserved
    }

    return true;
}

// Parse AC3 header assuming the current ptr is start position of syncframe,
// update metadata only applicable, and return the payload size
static unsigned parseAC3SyncFrame(
        const uint8_t *ptr, size_t size) {
    static const unsigned channelCountTable[] = {2, 1, 2, 3, 3, 4, 4, 5};
    static const unsigned samplingRateTable[] = {48000, 44100, 32000};

    static const unsigned frameSizeTable[19][3] = {
        { 64, 69, 96 },
        { 80, 87, 120 },
        { 96, 104, 144 },
        { 112, 121, 168 },
        { 128, 139, 192 },
        { 160, 174, 240 },
        { 192, 208, 288 },
        { 224, 243, 336 },
        { 256, 278, 384 },
        { 320, 348, 480 },
        { 384, 417, 576 },
        { 448, 487, 672 },
        { 512, 557, 768 },
        { 640, 696, 960 },
        { 768, 835, 1152 },
        { 896, 975, 1344 },
        { 1024, 1114, 1536 },
        { 1152, 1253, 1728 },
        { 1280, 1393, 1920 },
    };

    AmlMpBitReader bits(ptr, size);
    if (bits.numBitsLeft() < 16) {
        return 0;
    }
    if (bits.getBits(16) != 0x0B77) {
        return 0;
    }

    if (bits.numBitsLeft() < 16 + 2 + 6 + 5 + 3 + 3) {
        MLOGV("Not enough bits left for further parsing");
        return 0;
    }
    bits.skipBits(16);  // crc1

    unsigned fscod = bits.getBits(2);
    if (fscod == 3) {
        MLOGW("Incorrect fscod in AC3 header");
        return 0;
    }

    unsigned frmsizecod = bits.getBits(6);
    if (frmsizecod > 37) {
        MLOGW("Incorrect frmsizecod in AC3 header");
        return 0;
    }

    unsigned bsid = bits.getBits(5);
    if (bsid > 8) {
        MLOGW("Incorrect bsid in AC3 header. Possibly E-AC-3?");
        return 0;
    }

    bits.skipBits(3); // bsmod
    unsigned acmod = bits.getBits(3);

    if ((acmod & 1) > 0 && acmod != 1) {
        if (bits.numBitsLeft() < 2) {
            return 0;
        }
        bits.skipBits(2); //cmixlev
    }
    if ((acmod & 4) > 0) {
        if (bits.numBitsLeft() < 2) {
            return 0;
        }
        bits.skipBits(2); //surmixlev
    }
    if (acmod == 2) {
        if (bits.numBitsLeft() < 2) {
            return 0;
        }
        bits.skipBits(2); //dsurmod
    }

    if (bits.numBitsLeft() < 1) {
        return 0;
    }
    unsigned lfeon = bits.getBits(1);

    unsigned samplingRate = samplingRateTable[fscod];
    unsigned payloadSize = frameSizeTable[frmsizecod >> 1][fscod];
    if (fscod == 1) {
        payloadSize += frmsizecod & 1;
    }
    payloadSize <<= 1;  // convert from 16-bit words to bytes

    unsigned channelCount = channelCountTable[acmod] + lfeon;

    return payloadSize;
}

// Parse EAC3 header assuming the current ptr is start position of syncframe,
// update metadata only applicable, and return the payload size
// ATSC A/52:2012 E2.3.1
static unsigned parseEAC3SyncFrame(
    const uint8_t *ptr, size_t size) {
    static const unsigned channelCountTable[] = {2, 1, 2, 3, 3, 4, 4, 5};
    static const unsigned samplingRateTable[] = {48000, 44100, 32000};
    static const unsigned samplingRateTable2[] = {24000, 22050, 16000};

    AmlMpBitReader bits(ptr, size);
    if (bits.numBitsLeft() < 16) {
        MLOGE("Not enough bits left for further parsing");
        return 0;
    }
    if (bits.getBits(16) != 0x0B77) {
        MLOGE("No valid sync word in EAC3 header");
        return 0;
    }

    // we parse up to bsid so there needs to be at least that many bits
    if (bits.numBitsLeft() < 2 + 3 + 11 + 2 + 2 + 3 + 1 + 5) {
        MLOGE("Not enough bits left for further parsing");
        return 0;
    }

    unsigned strmtyp = bits.getBits(2);
    if (strmtyp == 3) {
        MLOGE("Incorrect strmtyp in EAC3 header");
        return 0;
    }

    unsigned substreamid = bits.getBits(3);
    // only the first independent stream is supported
    if ((strmtyp == 0 || strmtyp == 2) && substreamid != 0)
        return 0;

    unsigned frmsiz = bits.getBits(11);
    unsigned fscod = bits.getBits(2);

    unsigned samplingRate = 0;
    if (fscod == 0x3) {
        unsigned fscod2 = bits.getBits(2);
        if (fscod2 == 3) {
            MLOGW("Incorrect fscod2 in EAC3 header");
            return 0;
        }
        samplingRate = samplingRateTable2[fscod2];
    } else {
        samplingRate = samplingRateTable[fscod];
        bits.skipBits(2); // numblkscod
    }

    unsigned acmod = bits.getBits(3);
    unsigned lfeon = bits.getBits(1);
    unsigned bsid = bits.getBits(5);
    if (bsid < 11 || bsid > 16) {
        MLOGW("Incorrect bsid in EAC3 header. Could be AC-3 or some unknown EAC3 format");
        return 0;
    }

    unsigned payloadSize = frmsiz + 1;
    payloadSize <<= 1;  // convert from 16-bit words to bytes

    return payloadSize;
}

////////////////////////////////////////////////////////////////////////////////
ElementaryStreamQueue::ElementaryStreamQueue(Mode mode)
: mMode(mode)
{

}

void ElementaryStreamQueue::clear()
{
    if (mBuffer != nullptr) {
        mBuffer->setRange(0, 0);
    }

    mRangeInfos.clear();
    mAUIndex = 0;
}

int ElementaryStreamQueue::appendData(const void* data, size_t size, int64_t pts, int32_t payloadOffset)
{
    if (mBuffer == nullptr || mBuffer->size() == 0) {
        switch (mMode) {
        case H264:
        case H265:
        case MPEG_VIDEO:
        {
            uint8_t *ptr = (uint8_t *)data;
            ssize_t startOffset = -1;
            for (size_t i = 0; i+2 < size; ++i) {
                if (!memcmp("\x00\x00\x01", &ptr[i], 3)) {
                    startOffset = i;
                    break;
                }
            }

            if (startOffset < 0) {
                return -1;
            }

            data = &ptr[startOffset];
            size -= startOffset;
        }
        break;

        case MPEG4_VIDEO:
        {
            uint8_t* ptr = (uint8_t*)data;
            ssize_t startOffset = -1;
            for (size_t i = 0; i+2 < size; ++i) {
                if (!memcmp("\x00\x00\x01", &ptr[i], 3)) {
                    startOffset = i;
                    break;
                }
            }

            if (startOffset < 0) {
                return -1;
            }

            data = &ptr[startOffset];
            size -= startOffset;

            break;
        }

        case AAC:
        {
            uint8_t* ptr = (uint8_t*)data;
            ssize_t startOffset = -1;
            size_t frameLength;

            for (size_t i = 0; i < size; ++i) {
                if (IsSeeminglyValidADTSHeader(&ptr[i], size -i, &frameLength)) {
                    startOffset = i;
                    break;
                }
            }

            if (startOffset < 0) {
                return -1;
            }

            if (startOffset > 0) {
                ALOGI("found something resembling an AAC syncword at "
                    "offset %zd", startOffset);
            }

            if (frameLength != size - startOffset) {
                MLOGV( "First ADTS AAC frame length is %zd bytes, "
                    "while the buffer size is %zd bytes, startOffset:%zd.",
                    frameLength, size - startOffset, startOffset);
            }

            data = &ptr[startOffset];
            size -= startOffset;

            break;
        }

        case AC3:
        case EAC3:
        {
            uint8_t *ptr = (uint8_t *)data;
            size_t startOffset = -1;
            for (size_t i = 0; i < size; ++i) {
                unsigned payloadSize = 0;
                if (mMode == AC3) {
                    payloadSize = parseAC3SyncFrame(&ptr[i], size - i);
                } else if (mMode == EAC3) {
                    payloadSize = parseEAC3SyncFrame(&ptr[i], size - i);
                }

                if (payloadSize > 0) {
                    startOffset = i;
                    break;
                }
            }

            if (startOffset < 0) {
                return -1;
            }

            if (startOffset > 0) {
                ALOGI("found something resembling an (E)AC3 syncword at "
                    "offset %zd",
                    startOffset);
            }

            data = &ptr[startOffset];
            size -= startOffset;

            break;
        }

        case MPEG_AUDIO:
        {
            uint8_t* ptr = (uint8_t*) data;
            ssize_t startOffset = -1;
            for (size_t i = 0; i < size; ++i) {
                if (IsSeeminglyValidMPEGAudioHeader(&ptr[i], size - i)) {
                    startOffset = i;
                    break;
                }
            }

            if (startOffset < 0) {
                return -1;
            }

            data = &ptr[startOffset];
            size -= startOffset;

            break;
        }

        default:
            return -1;
        }
    }

    size_t neededSize = (mBuffer == NULL ? 0 : mBuffer->size()) + size;
    if (mBuffer == NULL || neededSize > mBuffer->capacity()) {
        neededSize = (neededSize + 65535) & ~65535;

        MLOGI("resizing buffer to size %zu", neededSize);

        sptr<AmlMpBuffer> buffer = new AmlMpBuffer(neededSize);
        if (mBuffer != NULL) {
            memcpy(buffer->data(), mBuffer->data(), mBuffer->size());
            buffer->setRange(0, mBuffer->size());
        } else {
            buffer->setRange(0, 0);
        }

        mBuffer = buffer;
    }

    memcpy(mBuffer->data() + mBuffer->size(), data, size);
    mBuffer->setRange(0, mBuffer->size() + size);

    RangeInfo info;
    info.mLength = size;
    info.mPts = pts;
    info.mPesOffset = payloadOffset;
    mRangeInfos.push_back(info);

    return 0;
}

sptr<AmlMpBuffer> ElementaryStreamQueue::dequeueAccessUnit()
{
    if (mBuffer == nullptr) {
        return nullptr;
    }

    switch (mMode) {
    case H264:
        return dequeueAccessUnitH264();

    case H265:
        return dequeueAccessUnitH265();

    case AAC:
        return dequeueAccessUnitAAC();

    case AC3:
    case EAC3:
        return dequeueAccessUnitEAC3();

    case MPEG_VIDEO:
        return dequeueAccessUnitMPEGVideo();

    case MPEG4_VIDEO:
        return dequeueAccessUnitMPEG4Video();

    case MPEG_AUDIO:
        return dequeueAccessUnitMPEGAudio();

    default:
        return nullptr;
    }
}

struct NALPosition {
    uint32_t nalOffset;
    uint32_t nalSize;

};

int getNextNALUnit(
        const uint8_t **_data, size_t *_size,
        const uint8_t **nalStart, size_t *nalSize,
        bool startCodeFollows = false) {
    const uint8_t *data = *_data;
    size_t size = *_size;

    *nalStart = NULL;
    *nalSize = 0;

    if (size < 3) {
        return -EAGAIN;
    }

    size_t offset = 0;

    // A valid startcode consists of at least two 0x00 bytes followed by 0x01.
    for (; offset + 2 < size; ++offset) {
        if (data[offset + 2] == 0x01 && data[offset] == 0x00
                && data[offset + 1] == 0x00) {
            break;
        }
    }
    if (offset + 2 >= size) {
        *_data = &data[offset];
        *_size = 2;
        return -EAGAIN;
    }
    offset += 3;

    size_t startOffset = offset;

    for (;;) {
        while (offset < size && data[offset] != 0x01) {
            ++offset;
        }

        if (offset == size) {
            if (startCodeFollows) {
                offset = size + 2;
                break;
            }

            return -EAGAIN;
        }

        if (data[offset - 1] == 0x00 && data[offset - 2] == 0x00) {
            break;
        }

        ++offset;
    }

    size_t endOffset = offset - 2;
    while (endOffset > startOffset + 1 && data[endOffset - 1] == 0x00) {
        --endOffset;
    }

    *nalStart = &data[startOffset];
    *nalSize = endOffset - startOffset;

    if (offset + 2 < size) {
        *_data = &data[offset - 2];
        *_size = size - offset + 2;
    } else {
        *_data = NULL;
        *_size = 0;
    }

    return 0;
}

static sptr<AmlMpBuffer> FindNAL(const uint8_t *data, size_t size, unsigned nalType) {
    const uint8_t *nalStart;
    size_t nalSize;
    while (getNextNALUnit(&data, &size, &nalStart, &nalSize, true) == 0) {
        if (nalSize > 0 && (nalStart[0] & 0x1f) == nalType) {
            sptr<AmlMpBuffer> buffer = new AmlMpBuffer(nalSize);
            memcpy(buffer->data(), nalStart, nalSize);
            return buffer;
        }
    }

    return NULL;
}

unsigned parseUE(AmlMpBitReader *br) {
    unsigned numZeroes = 0;
    while (br->getBits(1) == 0) {
        ++numZeroes;
    }

    unsigned x = br->getBits(numZeroes);

    return x + (1u << numZeroes) - 1;
}

sptr<AmlMpBuffer> ElementaryStreamQueue::dequeueAccessUnitH264()
{
    const uint8_t* data = mBuffer->data();

    size_t size = mBuffer->size();
    std::vector<NALPosition> nals;

    size_t totalSize = 0;
    size_t seiCount = 0;

    int err;
    const uint8_t* nalStart;
    size_t nalSize;
    bool foundSlice = false;
    bool foundIDR = false;

    while ((err = getNextNALUnit(&data, &size, &nalStart, &nalSize)) == 0) {
        if (nalSize == 0) continue;

        unsigned nalType = nalStart[0] & 0x1f;
        bool flush = false;
        if (nalType == 1 || nalType == 5) {
            if (nalType == 5) {
                foundIDR = true;
            }

            if (foundSlice) {
                AmlMpBitReader br(nalStart + 1, nalSize);
                unsigned first_mb_in_slice = parseUE(&br);
                if (first_mb_in_slice == 0) {
                    flush = true;
                }
            }

            foundSlice = true;
        } else if ((nalType == 9 || nalType == 7) && foundSlice) {
            flush = true;
        } else if (nalType == 6 && nalSize > 0) {
            ++seiCount;
        }

        if (flush) {
            size_t auSize = 4 * nals.size() + totalSize;
            sptr<AmlMpBuffer> accessUnit = new AmlMpBuffer(auSize + kPesHeaderSize);

            size_t dstOffset = kPesHeaderSize;
            for (size_t i = 0; i < nals.size(); ++i) {
                const NALPosition &pos = nals.at(i);
                unsigned nalType = mBuffer->data()[pos.nalOffset] & 0x1f;

                memcpy(accessUnit->data() + dstOffset, "\x00\x00\x00\x01", 4);
                memcpy(accessUnit->data() + dstOffset + 4, mBuffer->data() + pos.nalOffset, pos.nalSize);
                dstOffset += pos.nalSize + 4;
            }

            const NALPosition &pos = nals.at(nals.size() - 1);
            size_t nextScan = pos.nalOffset + pos.nalSize;
            memmove(mBuffer->data(),
                    mBuffer->data() + nextScan,
                    mBuffer->size() - nextScan);
            mBuffer->setRange(0, mBuffer->size() - nextScan);

            int64_t pts = fetchTimestamp(nextScan);
            if (pts < 0) {
                return nullptr;
            }
            makePESHeader(accessUnit->data(), accessUnit->capacity(), auSize, pts);

            MLOGV("dequeueAccessUnitH264[%d]: AU %p(%zu) dstOffset:%zu, nals:%zu, totalSize:%zu,auSize:%zu, PTS:%.3fs",
                    mAUIndex, accessUnit->data(), accessUnit->size(),
                    dstOffset, nals.size(), totalSize, auSize, pts/9e4);
            ++mAUIndex;

            //std::string result;
            //hexdump(accessUnit->data(), std::min(accessUnit->size(), (size_t)32), result);
            //MLOGV("h264:%s", result.c_str());

            return accessUnit;
        }

        NALPosition pos;
        pos.nalOffset = nalStart - mBuffer->data();
        pos.nalSize = nalSize;
        nals.push_back(pos);

        totalSize += nalSize;
    }

    return nullptr;
}



sptr<AmlMpBuffer> ElementaryStreamQueue::dequeueAccessUnitH265()
{
    const uint8_t* data = mBuffer->data();

    size_t size = mBuffer->size();
    std::vector<NALPosition> nals;

    size_t totalSize = 0;
    size_t seiCount = 0;

    int err;
    const uint8_t* nalStart;
    size_t nalSize;
	bool frame_start_found = false;
	int preVCLIndex = -1; // number of nals until VCL type found

    while ((err = getNextNALUnit(&data, &size, &nalStart, &nalSize)) == 0) {
        if (nalSize == 0) continue;

        unsigned nalType = (nalStart[0]>>1) & 0x3f;
        bool flush = false;
       if ((nalType >= 32 && nalType <= 35) || nalType == 39 ||
            (nalType >= 41 && nalType <= 44) || (nalType >= 48 && nalType <= 55)) {
            if (frame_start_found && (preVCLIndex > 0)) {
                flush = true;
            }
        } else if (nalType <= 9 || (nalType >= 16 && nalType <= 21)) {
            if (frame_start_found) {
                int first_slice_segment_in_pic_flag = (nalStart[2] & 0x80) >> 7;
                if (first_slice_segment_in_pic_flag && preVCLIndex > 0 && frame_start_found) {
                    flush = true;
                }
            }
            frame_start_found = true;
        }

        if (flush) {
            size_t auSize = 4 * nals.size() + totalSize;
            sptr<AmlMpBuffer> accessUnit = new AmlMpBuffer(auSize + kPesHeaderSize);

            size_t dstOffset = kPesHeaderSize;
            for (size_t i = 0; i < nals.size(); ++i) {
                const NALPosition &pos = nals.at(i);

                memcpy(accessUnit->data() + dstOffset, "\x00\x00\x00\x01", 4);
                memcpy(accessUnit->data() + dstOffset + 4, mBuffer->data() + pos.nalOffset, pos.nalSize);
                dstOffset += pos.nalSize + 4;
            }

            const NALPosition &pos = nals.at(nals.size() - 1);
            size_t nextScan = pos.nalOffset + pos.nalSize;
            memmove(mBuffer->data(),
                    mBuffer->data() + nextScan,
                    mBuffer->size() - nextScan);
            mBuffer->setRange(0, mBuffer->size() - nextScan);

            int64_t pts = fetchTimestamp(nextScan);
            if (pts < 0) {
                return nullptr;
            }
            makePESHeader(accessUnit->data(), accessUnit->capacity(), auSize, pts);

            MLOGV("dequeueAccessUnitH265[%d]: AU %p(%zu) dstOffset:%zu, nals:%zu, totalSize:%zu,auSize:%zu, PTS:%.3fs",
                    mAUIndex, accessUnit->data(), accessUnit->size(),
                    dstOffset, nals.size(), totalSize, auSize, pts/9e4);
            ++mAUIndex;

            //std::string result;
            //hexdump(accessUnit->data(), std::min(accessUnit->size(), (size_t)32), result);
            //MLOGV("h264:%s", result.c_str());

            return accessUnit;
        }

        NALPosition pos;
        pos.nalOffset = nalStart - mBuffer->data();
        pos.nalSize = nalSize;
        nals.push_back(pos);

        totalSize += nalSize;
        if (nalType < 32) { // VCL type
            preVCLIndex = nals.size();
        }
    }
    return nullptr;
}

sptr<AmlMpBuffer> ElementaryStreamQueue::dequeueAccessUnitAAC()
{
    if (mBuffer->size() == 0) {
        return nullptr;
    }

    if (mRangeInfos.empty()) {
        return nullptr;
    }

    const RangeInfo &info = *mRangeInfos.begin();
    if (info.mLength == 0 || mBuffer->size() < info.mLength) {
        return nullptr;
    }

    if (info.mPts < 0) {
        MLOGE("Negative info.mPts");
        return nullptr;
    }

    struct ADTSPosition {
        size_t offset;
        size_t headerSize;
        size_t length;

    };

    std::vector<ADTSPosition> frames;
    size_t offset = 0;
    while (offset < info.mLength) {
        if (offset + 7 > mBuffer->size()) {
            return nullptr;
        }

        AmlMpBitReader bits(mBuffer->data() + offset, mBuffer->size() - offset);
        if (bits.getBits(12) != 0xfffu) {
            //MLOGE("Wrong adts_fixed_header");
            ++offset;
            continue;
        }

        bits.skipBits(3);  // ID, layer
        bool protection_absent = bits.getBits(1) != 0;

        bits.skipBits(12);
        bits.skipBits(2);
        unsigned aac_frame_length = bits.getBits(13);
        if (aac_frame_length == 0) {
            MLOGE("Invalid AAC frame length!");
            return nullptr;
        }
        bits.skipBits(11);  // adts_buffer_fullness
        unsigned number_of_raw_data_blocks_in_frame = bits.getBits(2);

        if (number_of_raw_data_blocks_in_frame != 0) {
            MLOGE("Should not reach here.");
            return NULL;
        }

        if (offset + aac_frame_length > mBuffer->size()) {
            return NULL;
        }

        size_t headerSize = protection_absent ? 7 : 9;

        ADTSPosition frame = {
            .offset = offset,
            .headerSize = headerSize,
            .length = aac_frame_length,
        };
        frames.push_back(frame);

        offset += aac_frame_length;
    }

    int64_t pts = fetchTimestamp(offset);

    sptr<AmlMpBuffer> accessUnit = new AmlMpBuffer(offset + kPesHeaderSize);
    size_t pesHeaderSize = makePESHeader(accessUnit->data(), accessUnit->capacity(), offset, pts);
    memcpy(accessUnit->data() + pesHeaderSize, mBuffer->data(), offset);

    MLOGV("dequeueAccessUnit_AAC[%d]: AU:%p(%zu) offset:%zu, info.mLength %zu, frames:%zu, PTS:%.3fs",
                        mAUIndex, accessUnit->data(), accessUnit->size(),
                        offset,
                        info.mLength,
                        frames.size(),
                        pts/9e4);

    mAUIndex++;

    memmove(mBuffer->data(), mBuffer->data() + offset,
            mBuffer->size() - offset);
    mBuffer->setRange(0, mBuffer->size() - offset);

    return accessUnit;
}

sptr<AmlMpBuffer> ElementaryStreamQueue::dequeueAccessUnitEAC3()
{
    unsigned syncStartPos = 0;  // in bytes
    unsigned payloadSize = 0;

    MLOGV("dequeueAccessUnitEAC3[%d]: mBuffer %p(%zu)", mAUIndex,
            mBuffer->data(), mBuffer->size());

    while (true) {
        if (syncStartPos + 2 >= mBuffer->size()) {
            return NULL;
        }

        uint8_t *ptr = mBuffer->data() + syncStartPos;
        size_t size = mBuffer->size() - syncStartPos;
        if (mMode == AC3) {
            payloadSize = parseAC3SyncFrame(ptr, size);
        } else if (mMode == EAC3) {
            payloadSize = parseEAC3SyncFrame(ptr, size);
        }
        if (payloadSize > 0) {
            break;
        }

        MLOGV("dequeueAccessUnitEAC3[%d]: syncStartPos %u payloadSize %u",
                mAUIndex, syncStartPos, payloadSize);

        ++syncStartPos;
    }

    if (mBuffer->size() < syncStartPos + payloadSize) {
        MLOGV("Not enough buffer size for E/AC3");
        return NULL;
    }

    int64_t pts = fetchTimestamp(syncStartPos + payloadSize);
    if (pts < 0ll) {
        MLOGE("negative pts");
        return NULL;
    }

    mAUIndex++;

    sptr<AmlMpBuffer> accessUnit = new AmlMpBuffer(syncStartPos + payloadSize + kPesHeaderSize);
    memcpy(accessUnit->data() + kPesHeaderSize, mBuffer->data(), syncStartPos + payloadSize);

    makePESHeader(accessUnit->data(), accessUnit->capacity(), syncStartPos + payloadSize, pts);

    memmove(
            mBuffer->data(),
            mBuffer->data() + syncStartPos + payloadSize,
            mBuffer->size() - syncStartPos - payloadSize);

    mBuffer->setRange(0, mBuffer->size() - syncStartPos - payloadSize);

    return accessUnit;
}

uint32_t U32_AT(const uint8_t *ptr) {
    return ptr[0] << 24 | ptr[1] << 16 | ptr[2] << 8 | ptr[3];
}

bool GetMPEGAudioFrameSize(
        uint32_t header, size_t *frame_size,
        int *out_sampling_rate, int *out_channels,
        int *out_bitrate, int *out_num_samples) {
    *frame_size = 0;

    if (out_sampling_rate) {
        *out_sampling_rate = 0;
    }

    if (out_channels) {
        *out_channels = 0;
    }

    if (out_bitrate) {
        *out_bitrate = 0;
    }

    if (out_num_samples) {
        *out_num_samples = 1152;
    }

    if ((header & 0xffe00000) != 0xffe00000) {
        return false;
    }

    unsigned version = (header >> 19) & 3;

    if (version == 0x01) {
        return false;
    }

    unsigned layer = (header >> 17) & 3;

    if (layer == 0x00) {
        return false;
    }

    // we can get protection value from (header >> 16) & 1

    unsigned bitrate_index = (header >> 12) & 0x0f;

    if (bitrate_index == 0 || bitrate_index == 0x0f) {
        // Disallow "free" bitrate.
        return false;
    }

    unsigned sampling_rate_index = (header >> 10) & 3;

    if (sampling_rate_index == 3) {
        return false;
    }

    static const int kSamplingRateV1[] = { 44100, 48000, 32000 };
    int sampling_rate = kSamplingRateV1[sampling_rate_index];
    if (version == 2 /* V2 */) {
        sampling_rate /= 2;
    } else if (version == 0 /* V2.5 */) {
        sampling_rate /= 4;
    }

    unsigned padding = (header >> 9) & 1;

    if (layer == 3) {
        // layer I

        static const int kBitrateV1[] = {
            32, 64, 96, 128, 160, 192, 224, 256,
            288, 320, 352, 384, 416, 448
        };

        static const int kBitrateV2[] = {
            32, 48, 56, 64, 80, 96, 112, 128,
            144, 160, 176, 192, 224, 256
        };

        int bitrate =
            (version == 3 /* V1 */)
                ? kBitrateV1[bitrate_index - 1]
                : kBitrateV2[bitrate_index - 1];

        if (out_bitrate) {
            *out_bitrate = bitrate;
        }

        *frame_size = (12000 * bitrate / sampling_rate + padding) * 4;

        if (out_num_samples) {
            *out_num_samples = 384;
        }
    } else {
        // layer II or III

        static const int kBitrateV1L2[] = {
            32, 48, 56, 64, 80, 96, 112, 128,
            160, 192, 224, 256, 320, 384
        };

        static const int kBitrateV1L3[] = {
            32, 40, 48, 56, 64, 80, 96, 112,
            128, 160, 192, 224, 256, 320
        };

        static const int kBitrateV2[] = {
            8, 16, 24, 32, 40, 48, 56, 64,
            80, 96, 112, 128, 144, 160
        };

        int bitrate;
        if (version == 3 /* V1 */) {
            bitrate = (layer == 2 /* L2 */)
                ? kBitrateV1L2[bitrate_index - 1]
                : kBitrateV1L3[bitrate_index - 1];

            if (out_num_samples) {
                *out_num_samples = 1152;
            }
        } else {
            // V2 (or 2.5)

            bitrate = kBitrateV2[bitrate_index - 1];
            if (out_num_samples) {
                *out_num_samples = (layer == 1 /* L3 */) ? 576 : 1152;
            }
        }

        if (out_bitrate) {
            *out_bitrate = bitrate;
        }

        if (version == 3 /* V1 */) {
            *frame_size = 144000 * bitrate / sampling_rate + padding;
        } else {
            // V2 or V2.5
            size_t tmp = (layer == 1 /* L3 */) ? 72000 : 144000;
            *frame_size = tmp * bitrate / sampling_rate + padding;
        }
    }

    if (out_sampling_rate) {
        *out_sampling_rate = sampling_rate;
    }

    if (out_channels) {
        int channel_mode = (header >> 6) & 3;

        *out_channels = (channel_mode == 3) ? 1 : 2;
    }

    return true;
}

sptr<AmlMpBuffer> ElementaryStreamQueue::dequeueAccessUnitMPEGAudio()
{
    const uint8_t *data = mBuffer->data();
    size_t size = mBuffer->size();

    if (size < 4) {
        return nullptr;
    }

    uint32_t header = U32_AT(data);

    size_t frameSize;
    int samplingRate, numChannels, bitrate, numSamples;
    if (!GetMPEGAudioFrameSize(header, &frameSize, &samplingRate, &numChannels, &bitrate, &numSamples)) {
        MLOGE("Failed to get audio frame size");
        mBuffer->setRange(0, 0);
        return nullptr;
    }

    if (size < frameSize) {
        return nullptr;
    }

    int64_t pts = fetchTimestamp(frameSize);
    if (pts < 0) {
        MLOGE("Negative timeUs");
        return nullptr;
    }

    unsigned layer = 4 - ((header >> 17) & 3);
    sptr<AmlMpBuffer> accessUnit = new AmlMpBuffer(frameSize + kPesHeaderSize);
    size_t pesHeaderSize = makePESHeader(accessUnit->data(), accessUnit->capacity(), frameSize, pts);
    memcpy(accessUnit->data() + pesHeaderSize, data, frameSize);

    memmove(mBuffer->data(), mBuffer->data() + frameSize, mBuffer->size() - frameSize);
    mBuffer->setRange(0, mBuffer->size() - frameSize);

    return accessUnit;
}

sptr<AmlMpBuffer> ElementaryStreamQueue::dequeueAccessUnitMPEGVideo()
{
   const uint8_t *data = mBuffer->data();
    size_t size = mBuffer->size();

    std::vector<size_t> userDataPositions;

    bool sawPictureStart = false;
    int pprevStartCode = -1;
    int prevStartCode = -1;
    int currentStartCode = -1;
    bool gopFound = false;
    bool isClosedGop = false;
    bool brokenLink = false;

    size_t offset = 0;
    while (offset + 3 < size) {
        if (memcmp(&data[offset], "\x00\x00\x01", 3)) {
            ++offset;
            continue;
        }

        pprevStartCode = prevStartCode;
        prevStartCode = currentStartCode;
        currentStartCode = data[offset + 3];

        if (currentStartCode == 0xb3 && mFormat == nullptr) {
            memmove(mBuffer->data(), mBuffer->data() + offset, size - offset);
            size -= offset;
            (void)fetchTimestamp(offset);
            offset = 0;
            mBuffer->setRange(0, size);
        }

        if ((prevStartCode == 0xb3 && currentStartCode != 0xb5)
                || (pprevStartCode == 0xb3 && prevStartCode == 0xb5)) {
            // seqHeader without/with extension
            if (mFormat == nullptr) {
                if (size < 7u) {
                    MLOGE("Size too small");
                    return NULL;
                }

                unsigned width =
                    (data[4] << 4) | data[5] >> 4;

                unsigned height =
                    ((data[5] & 0x0f) << 8) | data[6];

                mFormat = new AmlMpBuffer(1);

                MLOGI("found MPEG2 video codec config (%d x %d)", width, height);
            }
        }

        if (mFormat != nullptr && currentStartCode == 0xb8) {
            // GOP layer
            if (offset + 7 >= size) {
                MLOGE("Size too small");
                return NULL;
            }
            gopFound = true;
            isClosedGop = (data[offset + 7] & 0x40) != 0;
            brokenLink = (data[offset + 7] & 0x20) != 0;
        }

        if (mFormat != nullptr && currentStartCode == 0xb2) {
            userDataPositions.push_back(offset);
        }

        if (mFormat != nullptr && currentStartCode == 0x00) {
            // Picture start

            if (!sawPictureStart) {
                sawPictureStart = true;
            } else {
                sptr<AmlMpBuffer> accessUnit = new AmlMpBuffer(offset + kPesHeaderSize);
                int64_t pts = fetchTimestamp(offset);
                if (pts < 0LL) {
                    MLOGE("Negative pts");
                    return NULL;
                }

                makePESHeader(accessUnit->data(), accessUnit->capacity(), offset, pts);

                memcpy(accessUnit->data() + kPesHeaderSize, data, offset);

                MLOGV("dequeueAccessUnitMPEGVideo[%d]: AU %p(%zu) offset:%zu, PTS:%.3fs",
                    mAUIndex, accessUnit->data(), accessUnit->size(),
                    offset, pts/9e4);

                ++mAUIndex;

                memmove(mBuffer->data(),
                        mBuffer->data() + offset,
                        mBuffer->size() - offset);

                mBuffer->setRange(0, mBuffer->size() - offset);

                offset = 0;

                if (gopFound && (!brokenLink || isClosedGop)) {
                    //accessUnit->meta()->setInt32("isSync", 1);
                }

                // hexdump(accessUnit->data(), accessUnit->size());

                return accessUnit;
            }
        }

        ++offset;
    }

    return NULL;
}

static ssize_t getNextChunkSize(
        const uint8_t *data, size_t size) {
    static const char kStartCode[] = "\x00\x00\x01";

    // per ISO/IEC 14496-2 6.2.1, a chunk has a 3-byte prefix + 1-byte start code
    // we need at least <prefix><start><next prefix> to successfully scan
    if (size < 3 + 1 + 3) {
        return -EAGAIN;
    }

    if (memcmp(kStartCode, data, 3)) {
        return -EAGAIN;
    }

    size_t offset = 4;
    while (offset + 2 < size) {
        if (!memcmp(&data[offset], kStartCode, 3)) {
            return offset;
        }

        ++offset;
    }

    return -EAGAIN;
}

bool ExtractDimensionsFromVOLHeader(
        const uint8_t *data, size_t size, int32_t *width, int32_t *height) {
    AmlMpBitReader br(&data[4], size - 4);
    br.skipBits(1);  // random_accessible_vol
    unsigned video_object_type_indication = br.getBits(8);

    AML_MP_CHECK_NE(video_object_type_indication,
             0x21u /* Fine Granularity Scalable */);

    if (br.getBits(1)) {
        br.skipBits(4); //video_object_layer_verid
        br.skipBits(3); //video_object_layer_priority
    }
    unsigned aspect_ratio_info = br.getBits(4);
    if (aspect_ratio_info == 0x0f /* extended PAR */) {
        br.skipBits(8);  // par_width
        br.skipBits(8);  // par_height
    }
    if (br.getBits(1)) {  // vol_control_parameters
        br.skipBits(2);  // chroma_format
        br.skipBits(1);  // low_delay
        if (br.getBits(1)) {  // vbv_parameters
            br.skipBits(15);  // first_half_bit_rate
            AML_MP_CHECK(br.getBits(1));  // marker_bit
            br.skipBits(15);  // latter_half_bit_rate
            AML_MP_CHECK(br.getBits(1));  // marker_bit
            br.skipBits(15);  // first_half_vbv_buffer_size
            AML_MP_CHECK(br.getBits(1));  // marker_bit
            br.skipBits(3);  // latter_half_vbv_buffer_size
            br.skipBits(11);  // first_half_vbv_occupancy
            AML_MP_CHECK(br.getBits(1));  // marker_bit
            br.skipBits(15);  // latter_half_vbv_occupancy
            AML_MP_CHECK(br.getBits(1));  // marker_bit
        }
    }
    unsigned video_object_layer_shape = br.getBits(2);
    //CHECK_EQ(video_object_layer_shape, 0x00u [> rectangular <]);

    AML_MP_CHECK(br.getBits(1));  // marker_bit
    unsigned vop_time_increment_resolution = br.getBits(16);
    AML_MP_CHECK(br.getBits(1));  // marker_bit

    if (br.getBits(1)) {  // fixed_vop_rate
        // range [0..vop_time_increment_resolution)

        // vop_time_increment_resolution
        // 2 => 0..1, 1 bit
        // 3 => 0..2, 2 bits
        // 4 => 0..3, 2 bits
        // 5 => 0..4, 3 bits
        // ...

        AML_MP_CHECK_GT(vop_time_increment_resolution, 0u);
        --vop_time_increment_resolution;

        unsigned numBits = 0;
        while (vop_time_increment_resolution > 0) {
            ++numBits;
            vop_time_increment_resolution >>= 1;
        }

        br.skipBits(numBits);  // fixed_vop_time_increment
    }

    AML_MP_CHECK(br.getBits(1));  // marker_bit
    unsigned video_object_layer_width = br.getBits(13);
    AML_MP_CHECK(br.getBits(1));  // marker_bit
    unsigned video_object_layer_height = br.getBits(13);
    AML_MP_CHECK(br.getBits(1));  // marker_bit

    br.skipBits(1); // interlaced

    *width = video_object_layer_width;
    *height = video_object_layer_height;

    return true;
}

sptr<AmlMpBuffer> ElementaryStreamQueue::dequeueAccessUnitMPEG4Video()
{
    uint8_t *data = mBuffer->data();
    size_t size = mBuffer->size();

    enum {
        SKIP_TO_VISUAL_OBJECT_SEQ_START,
        EXPECT_VISUAL_OBJECT_START,
        EXPECT_VO_START,
        EXPECT_VOL_START,
        WAIT_FOR_VOP_START,
        SKIP_TO_VOP_START,

    } state;

    if (mFormat == NULL) {
        state = SKIP_TO_VISUAL_OBJECT_SEQ_START;
    } else {
        state = SKIP_TO_VOP_START;
    }

    int32_t width = -1, height = -1;

    size_t offset = 0;
    ssize_t chunkSize;
    while ((chunkSize = getNextChunkSize(
                    &data[offset], size - offset)) > 0) {
        bool discard = false;

        unsigned chunkType = data[offset + 3];

        switch (state) {
            case SKIP_TO_VISUAL_OBJECT_SEQ_START:
            {
                if (chunkType == 0xb0) {
                    // Discard anything before this marker.

                    state = EXPECT_VISUAL_OBJECT_START;
                } else {
                    discard = true;
                    offset += chunkSize;
                    MLOGW("b/74114680, advance to next chunk");
                }
                break;
            }

            case EXPECT_VISUAL_OBJECT_START:
            {
                if (chunkType != 0xb5) {
                    MLOGE("Unexpected chunkType");
                    return NULL;
                }
                state = EXPECT_VO_START;
                break;
            }

            case EXPECT_VO_START:
            {
                if (chunkType > 0x1f) {
                    MLOGE("Unexpected chunkType");
                    return NULL;
                }
                state = EXPECT_VOL_START;
                break;
            }

            case EXPECT_VOL_START:
            {
                if ((chunkType & 0xf0) != 0x20) {
                    MLOGE("Wrong chunkType");
                    return NULL;
                }

                if (!ExtractDimensionsFromVOLHeader(
                            &data[offset], chunkSize,
                            &width, &height)) {
                    MLOGE("Failed to get dimension");
                    return NULL;
                }

                state = WAIT_FOR_VOP_START;
                break;
            }

            case WAIT_FOR_VOP_START:
            {
                if (chunkType == 0xb3 || chunkType == 0xb6) {
                    // group of VOP or VOP start.

                    mFormat = new AmlMpBuffer(1);

                    MLOGI("found MPEG4 video codec config (%d x %d), chunkType:%#x",
                         width, height, chunkType);

                    // hexdump(csd->data(), csd->size());

                    //discard = true;
                    state = SKIP_TO_VOP_START;
                }

                break;
            }

            case SKIP_TO_VOP_START:
            {
                if (chunkType == 0xb6) {
                    int vopCodingType = (data[offset + 4] & 0xc0) >> 6;

                    offset += chunkSize;

                    sptr<AmlMpBuffer> accessUnit = new AmlMpBuffer(offset + kPesHeaderSize);
                    memcpy(accessUnit->data() + kPesHeaderSize, data, offset);

                    memmove(data, &data[offset], size - offset);
                    size -= offset;
                    mBuffer->setRange(0, size);

                    int64_t pts = fetchTimestamp(offset);
                    if (pts < 0LL) {
                        MLOGE("Negative pts");
                        return NULL;
                    }
                    makePESHeader(accessUnit->data(), accessUnit->capacity(), offset, pts);

                    offset = 0;

                    //if (vopCodingType == 0) {  // intra-coded VOP
                        //accessUnit->meta()->setInt32("isSync", 1);
                    //}

                    MLOGV("returning MPEG4 video access unit at time %.3fs",
                         pts/9e4);

                    // hexdump(accessUnit->data(), accessUnit->size());

                    return accessUnit;
                } else if (chunkType != 0xb3) {
                    //offset += chunkSize;
                    //discard = true;
                }

                break;
            }

            default:
                MLOGE("Unknown state: %d", state);
                return NULL;
        }

        if (discard) {
            (void)fetchTimestamp(offset);
            memmove(data, &data[offset], size - offset);
            size -= offset;
            offset = 0;
            mBuffer->setRange(0, size);
        } else {
            offset += chunkSize;
        }
    }

    return NULL;
}

int64_t ElementaryStreamQueue::fetchTimestamp(
        size_t size, int32_t *pesOffset) {
    int64_t pts = -1;
    bool first = true;

    while (size > 0) {
        if (mRangeInfos.empty()) {
            return pts;
        }

        RangeInfo *info = &*mRangeInfos.begin();

        if (first) {
            pts = info->mPts;
            if (pesOffset != NULL) {
                *pesOffset = info->mPesOffset;
            }
            first = false;
        }

        if (info->mLength > size) {
            info->mLength -= size;
            size = 0;
        } else {
            size -= info->mLength;

            mRangeInfos.erase(mRangeInfos.begin());
            info = NULL;
        }

    }

    return pts;
}

size_t ElementaryStreamQueue::makePESHeader(uint8_t* data, size_t size, size_t frameLength, int64_t pts)
{
    if (size < 14) {
        return 0;
    }

    uint8_t* p = data;

    p[0] = 0x00;
    p[1] = 0x00;
    p[2] = 0x01;
    p += 3;

    p[0] = 0xE0;
    ++p;

    int pes_packet_length = 8 + frameLength;
    if (pes_packet_length <= 65535) {
        p[0] = pes_packet_length >> 8;
        p[1] = pes_packet_length & 0xff;
    } else {
        p[0] = 0;
        p[1] = 0;
    }
    p += 2;

    p[0] = 0x80;
    p[1] = 0x80;
    p[2] = 5; //PTS size
    p += 3;

    p[0] = 0x20 | (pts>>29&0x0e) | 0x01;
    p[1] = pts>>22 & 0xff;
    p[2] = (pts>>14 & 0xfe) | 0x01;
    p[3] = pts>>7 & 0xff;
    p[4] = (pts<<1 & 0xfe) | 0x01;
    p += 5;

    return p - data;
}

}
