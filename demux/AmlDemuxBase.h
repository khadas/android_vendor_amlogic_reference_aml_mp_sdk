/*
 * Copyright (c) 2020 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#ifndef _AML_MP_DEMUX_BASE_H_
#define _AML_MP_DEMUX_BASE_H_

#include <Aml_MP/Common.h>
#include <utils/AmlMpRefBase.h>
#include <mutex>
#include <map>

namespace aml_mp {
struct AmlMpBuffer;

///////////////////////////////////////////////////////////////////////////////
typedef int (*Aml_MP_Demux_FilterCb)(int pid, size_t size, const uint8_t* data, void* userData);

typedef enum {
    AML_MP_DEMUX_TYPE_HARDWARE,
    AML_MP_DEMUX_TYPE_SOFTWARE,
    AML_MP_DEMUX_TYPE_TUNERHAL,
} Aml_MP_DemuxType;

typedef enum {
    AML_MP_DEMUX_FILTER_PSI,
    AML_MP_DEMUX_FILTER_VIDEO,
    AML_MP_DEMUX_FILTER_AUDIO,
    AML_MP_DEMUX_FILTER_PCR,
} Aml_MP_DemuxFilterType;

typedef struct {
    Aml_MP_DemuxFilterType type;
    Aml_MP_CodecID codecType;
    uint32_t flags;

#define DMX_CHECK_CRC   1
#define DMX_ONESHOT     2
#define DMX_IMMEDIATE_START 4
#define DMX_KERNEL_CLIENT   0x8000

/*bit 16~23 for output */
#define DMX_ES_OUTPUT        (1 << 16)
/*set raw mode, it will send the struct dmx_sec_es_data, not es data*/
#define DMX_OUTPUT_RAW_MODE  (1 << 17)
#define DMX_OUTPUT_SECTION_MODE (1<<18)
} Aml_MP_DemuxFilterParams;

class AmlDemuxBase : public AmlMpRefBase
{
public:
    typedef void* CHANNEL;
    typedef void* FILTER;

    static sptr<AmlDemuxBase> create(Aml_MP_DemuxType demuxType);
    virtual ~AmlDemuxBase();

    virtual int open(bool isHardwareSource, Aml_MP_DemuxId demuxId = AML_MP_DEMUX_ID_DEFAULT, bool isSecureBuffer = false) = 0;
    virtual int close() = 0;
    virtual int start() = 0;
    virtual int stop() = 0;
    virtual int flush() = 0;
    virtual int feedTs(const uint8_t* buffer, size_t size) {
        (void)buffer;
        return size;
    }

    CHANNEL createChannel(int pid, const Aml_MP_DemuxFilterParams* params);
    int destroyChannel(CHANNEL channel);
    int openChannel(CHANNEL channel);
    int closeChannel(CHANNEL channel);
    FILTER createFilter(Aml_MP_Demux_FilterCb cb, void* userData);
    int destroyFilter(FILTER filter);
    int attachFilter(FILTER filter, CHANNEL channel);
    int detachFilter(FILTER filter, CHANNEL channel);

    struct ITsParser : virtual public AmlMpRefBase {
        using FilterCallback = void(int pid, const sptr<AmlMpBuffer>& data, int version);

        explicit ITsParser(const std::function<FilterCallback>& cb);
        virtual ~ITsParser() =default;
        virtual int feedTs(const uint8_t* buffer, size_t size) = 0;
        virtual void reset() = 0;
        virtual int addDemuxFilter(int pid, const Aml_MP_DemuxFilterParams* params) = 0;
        virtual void removeDemuxFilter(int pid) = 0;

    protected:
        std::function<FilterCallback> mFilterCallback;

    private:
        ITsParser(const ITsParser&) = delete;
        ITsParser& operator=(const ITsParser&) = delete;
    };

protected:
    struct Channel;
    struct Filter;

    AmlDemuxBase();
    virtual int addDemuxFilter(int pid, const Aml_MP_DemuxFilterParams* params) = 0;
    virtual int removeDemuxFilter(int pid) = 0;
    virtual bool isStopped() const = 0;

    void notifyData(int pid, const sptr<AmlMpBuffer>& data, int version);

    std::atomic<uint32_t> mFilterId{0};
    std::mutex mLock;
    std::map<int, sptr<Channel>> mChannels;

private:
    AmlDemuxBase(const AmlDemuxBase&) = delete;
    AmlDemuxBase& operator= (const AmlDemuxBase&) = delete;
};

}





#endif
