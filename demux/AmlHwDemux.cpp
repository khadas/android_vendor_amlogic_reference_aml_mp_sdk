/*
 * Copyright (c) 2020 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

//#define LOG_NDEBUG 0
#define LOG_TAG "AmlMpPlayerDemo_AmlHwDemux"
#include <utils/AmlMpLog.h>
#include <Aml_MP/Aml_MP.h>
#include "AmlHwDemux.h"
#include <utils/AmlMpHandle.h>
#include <utils/AmlMpBuffer.h>
#include <utils/AmlMpEventLooper.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <sstream>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>

#define CONFIG_AMLOGIC_DVB_COMPAT // use for write_ts to demux
extern "C" {
#include <dmx.h>
}


static const char* mName = LOG_TAG;

namespace aml_mp {
static const int kPesFilterBufferSize = 2 * 1024 * 1024;

class HwTsParser : public AmlDemuxBase::ITsParser, public LooperCallback
{
public:
    HwTsParser(const std::function<FilterCallback>& cb, const std::string& name);
    ~HwTsParser();
    int dvr_open(int demuxId, bool isHardwareSource, bool isSecureBuffer);
    int feedTs(const uint8_t* buffer, size_t size);
    int dvr_close();
    void reset();
    int addDemuxFilter(int pid, const Aml_MP_DemuxFilterParams* params) override;
    void removeDemuxFilter(int pid) override;

private:
    virtual int handleEvent(int fd, int events, void* data);

    std::string mDemuxName;
    bool mIsSecurebuffer = false;

    std::mutex mLock;
    std::map<int, int> mChannelFds; //pid, fd
    int mDvrFd;
    sptr<AmlMpBuffer> mCallbackBuffer;

private:
    HwTsParser(const HwTsParser&) = delete;
    HwTsParser& operator=(const HwTsParser&) = delete;
};

///////////////////////////////////////////////////////////////////////////////
AmlHwDemux::AmlHwDemux()
{
    MLOG("mstopped = %d", mStopped.load());

    mIsHardwareSource = false;
    mIsSecureBuffer = false;
}

AmlHwDemux::~AmlHwDemux()
{
    close();
}

int AmlHwDemux::open(bool isHardwareSource, Aml_MP_DemuxId demuxId, bool isSecureBuffer)
{
    mDemuxId = demuxId;
    mIsHardwareSource = isHardwareSource;
    mIsSecureBuffer = isSecureBuffer;

    std::stringstream s;
    s << "/dev/dvb0.demux" << mDemuxId;
    mDemuxName = s.str();
    MLOGI("mDemuxName:%s", mDemuxName.c_str());

    if (mTsParser == nullptr) {
        mTsParser = new HwTsParser([this](int pid, const sptr<AmlMpBuffer>& data, int version) {
            return notifyData(pid, data, version);
        }, mDemuxName);
    }

    return 0;
}

int AmlHwDemux::close()
{
    MLOG();

    mTsParser.clear();

    return 0;
}

int AmlHwDemux::start()
{
    MLOGI("start");

    if (mTsParser == nullptr) {
        MLOGE("Demux not open, start fail");
        return -1;
    }

    mTsParser->dvr_open(mDemuxId, mIsHardwareSource, mIsSecureBuffer);
    if (mLooper == nullptr) {
        mLooper = new Looper(Looper::PREPARE_ALLOW_NON_CALLBACKS);
        if (mLooper == nullptr) {
            MLOGE("create looper failed!");
            return -1;
        }
    }

    mThread = std::thread([this] {
        threadLoop();
    });

    return 0;
}

int AmlHwDemux::stop()
{
    MLOG();

    {
        std::lock_guard<std::mutex> _l(mLock);
        mStopped = true;
    }

    if (mLooper) {
        mLooper->wake();
    }

    MLOGI("join...");
    if (mThread.joinable()) {
        mThread.join();
    }

    mTsParser->dvr_close();

    MLOGI("stopped!");
    return 0;
}

int AmlHwDemux::flush()
{
    MLOG();

    return 0;
}

int AmlHwDemux::feedTs(const uint8_t* buffer, size_t size)
{
    std::lock_guard<std::mutex> _l(mLock);
    if (mStopped) {
        return 0;
    }
    return mTsParser->feedTs(buffer, size);
}

int AmlHwDemux::addDemuxFilter(int pid, const Aml_MP_DemuxFilterParams* params)
{
    int channelFd = mTsParser->addDemuxFilter(pid, params);

    std::unique_ptr<FilterParams> filterParams(new FilterParams);
    filterParams->pid = pid;
    filterParams->fd = channelFd;
    filterParams->params = *params;

    int ret = mLooper->addFd(
            channelFd,
            Looper::POLL_CALLBACK,
            Looper::EVENT_ERROR|Looper::EVENT_INPUT,
            mTsParser,
            filterParams.get());

    if (ret <= 0) {
        MLOGE("addFd failed! fd:%d", channelFd);
    } else {
        mFilterParams.emplace(pid, std::move(filterParams));
    }

    return ret;
}

int AmlHwDemux::removeDemuxFilter(int pid)
{
    auto it = mFilterParams.find(pid);
    if (it == mFilterParams.end()) {
        MLOGE("removeFd failed! invalid pid:%d", pid);
        return -1;
    }
    std::unique_ptr<FilterParams> filterParams = std::move(it->second);
    mFilterParams.erase(it);

    int channelFd = filterParams->fd;
    int ret = mLooper->removeFd(channelFd);
    if (ret <= 0) {
        MLOGE("removeFd failed! fd:%d", channelFd);
    }

    mTsParser->removeDemuxFilter(pid);

    return ret;
}

bool AmlHwDemux::isStopped() const
{
    return mStopped.load(std::memory_order_relaxed);
}

void AmlHwDemux::threadLoop()
{
    int ret = 0;
    ALOGI("hwdemux thread start!");

    if (mLooper == nullptr) {
        MLOGE("looper is NULL!");
        return;
    }
    Looper::setForThread(mLooper);

    for (;;) {
        ret = mLooper->pollOnce(10 * 1000);
        if (ret == Looper::POLL_ERROR) {
            MLOGE("error!");
            break;
        }

        bool stopped = mStopped.load(std::memory_order_relaxed);
        if (stopped) {
            MLOGE("request stop...");
            break;
        }
    }

    MLOGI("AmlHwDemux threadLoop exited!");
}

///////////////////////////////////////////////////////////////////////////////
HwTsParser::HwTsParser(const std::function<FilterCallback>& cb, const std::string& name)
: ITsParser(cb)
, mDemuxName(name)
{
    MLOG();
    mDvrFd = -1;
}

HwTsParser::~HwTsParser()
{
    MLOG();
}


int HwTsParser::dvr_open(int demuxId, bool isHardwareSource, bool isSecurebuffer) {
    int ret = 0;
    char name[32];
    snprintf(name, sizeof(name), "/dev/dvb0.dvr%d", demuxId);
    mDvrFd = open(name, O_WRONLY);
    if (mDvrFd == -1) {
        MLOGE("cannot open \"%s\" (%s)", name, strerror(errno));
        return -1;
    }
    MLOGI("open %s  ok \n", name);

    mIsSecurebuffer = isSecurebuffer;
    if (!isHardwareSource) {
        if (isSecurebuffer) {
            MLOGI("set ---> INPUT_LOCAL_SEC \n");
            ret = ioctl(mDvrFd, DMX_SET_INPUT, INPUT_LOCAL_SEC);
        } else {
            MLOGI("set ---> INPUT_LOCAL \n");
            ret = ioctl(mDvrFd, DMX_SET_INPUT, INPUT_LOCAL);
        }
    } else {
        MLOGI("set ---> INPUT_DEMOD \n" );
        ret = ioctl(mDvrFd, DMX_SET_INPUT, INPUT_DEMOD);
    }
    MLOGI("DMX_SET_INPUT ret:%d\n", ret);
    if (ret < 0) {
        MLOGE("dvr_open ioctl failed %s\n", strerror(errno));
        return -1;
    }
    return 0;
}

int HwTsParser::dvr_close() {
    if (mDvrFd > 0) {
        close(mDvrFd);
    }
    return 0;
}

int HwTsParser::feedTs(const uint8_t* buffer, size_t size)
{
    int ret;
    int left = size;
    int off = 0;

    if (mDvrFd < 0) {
        return -1;
    }
    uint8_t* buffer_secure = NULL;
    struct dmx_sec_ts_data ts_sec_data;
    if (mIsSecurebuffer) {
        ts_sec_data.buf_start = (uint32_t)(long)buffer;
        ts_sec_data.buf_end = (uint32_t)(long)(buffer+size);
        buffer_secure = (uint8_t *)(&ts_sec_data);
        left = sizeof(struct dmx_sec_ts_data);
    }
    while (left > 0) {
        if (mIsSecurebuffer) {
            ret = write(mDvrFd, buffer_secure, left);
            // MLOGI("Write DVR data: buffer_secure:%p, left:%d, ret:%d", buffer_secure, left, ret);
        } else {
            ret = write(mDvrFd, buffer + off, left);
        }
        if (ret == -1) {
            if (errno != EINTR) {
                MLOGE("Write DVR data failed: %s", strerror(errno));
                break;
            }
            ret = 0;
        }
        left -= ret;
        off += ret;
    }
    return (size - left);
}

int HwTsParser::addDemuxFilter(int pid, const Aml_MP_DemuxFilterParams* params)
{
    int fd = -1;
    int ret = 0;
    fd = ::open(mDemuxName.c_str(), O_RDWR);
    if (fd < 0) {
        MLOG("open %s failed! %s", mDemuxName.c_str(), strerror(errno));
        return -1;
    }

    if (params->type == AML_MP_DEMUX_FILTER_PSI) {
        int buffersize = 32 * 1024;
        ret = ioctl(fd, DMX_SET_BUFFER_SIZE, buffersize);
        if (ret < 0) {
            MLOG("set buffer size failed!");
            ::close(fd);
            return -1;
        }

        struct dmx_sct_filter_params filter_param;
        memset(&filter_param, 0, sizeof(filter_param));
        filter_param.pid = pid;
        filter_param.flags = params->flags;

        ret = ioctl(fd, DMX_SET_FILTER, &filter_param);
        if (ret < 0) {
            MLOG("set filter failed!");
            ::close(fd);
            return -1;
        }
    } else {
        int buffersize = kPesFilterBufferSize;
        ret = ioctl(fd, DMX_SET_BUFFER_SIZE, buffersize);
        if (ret < 0) {
            MLOG("set pes buffer size failed!");
            ::close(fd);
            return -1;
        }

        struct dmx_pes_filter_params filter_param;
        memset(&filter_param, 0, sizeof(filter_param));
        filter_param.pid = pid;
        filter_param.flags = params->flags;
        filter_param.input = DMX_IN_FRONTEND;
        filter_param.output = DMX_OUT_TAP;
        if (params->type == AML_MP_DEMUX_FILTER_AUDIO) {
            filter_param.pes_type = DMX_PES_AUDIO;
        } else if (params->type == AML_MP_DEMUX_FILTER_VIDEO) {
            filter_param.pes_type = DMX_PES_VIDEO0;
        }

        MLOGI("create pes filter, pid:%d, type:%d, fd:%d, flags:%#x", pid, params->type, fd, params->flags);
        ret = ioctl(fd, DMX_SET_PES_FILTER, &filter_param);
        if (ret < 0) {
            MLOG("set pes filter failed! %d", ret);
            ::close(fd);
            return -1;
        }
    }

    ret = ioctl(fd, DMX_START);
    if (ret < 0) {
        MLOG("dmx start failed!");
        ::close(fd);
        return -1;
    }

    {
        std::unique_lock<std::mutex> _l(mLock);
        mChannelFds.emplace(pid, fd);
    }

    return fd;
}

//int HwTsParser::getDemuxFilterPrivateData(int pid)
//{
    //std::unique_lock<std::mutex> _l(mLock);
    //auto it = mChannelFds.find(pid);
    //return it != mChannelFds.end() ? it->second : -1;
//}

void HwTsParser::removeDemuxFilter(int pid)
{
    int fd = -1;
    {
        std::unique_lock<std::mutex> _l(mLock);
        auto it = mChannelFds.find(pid);
        if (it != mChannelFds.end()) {
            fd = it->second;
            mChannelFds.erase(it);
        }
    }

    if (fd < 0) {
        MLOG("mFd is invalid");
        return;
    }

    int ret = ioctl(fd, DMX_STOP);
    if (ret < 0) {
        MLOG("dmx stop failed!");
        return;
    }

    ::close(fd);
}

void HwTsParser::reset()
{

}

int HwTsParser::handleEvent(int fd, int events, void* data)
{
    sptr<AmlMpBuffer>& buffer = mCallbackBuffer;
    if (mCallbackBuffer == nullptr) {
        mCallbackBuffer = new AmlMpBuffer(kPesFilterBufferSize);
    }

    if (events & Looper::EVENT_INPUT) {
        int len = ::read(fd, buffer->base(), buffer->capacity());
        if (len < 0) {
            MLOGE("read failed, %s", strerror(errno));
            return 0;
        }

        buffer->setRange(0, len);
    }

    int version = -1;

    AmlHwDemux::FilterParams* filterParams = (AmlHwDemux::FilterParams*)data;
    if (filterParams->params.type == AML_MP_DEMUX_FILTER_PSI) {
        version = buffer->data()[5]>>1 & 0x1F;
    }
    int pid = filterParams->pid;
    MLOGV("fd:%d, pid:%d, size:%zu", fd, pid, buffer->size());
    if (mFilterCallback) {
        mFilterCallback(pid, buffer, version);
    }

    return 1;
}

}
