/*
 * Copyright (c) 2021 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#ifndef _AML_MP_TUNERHAL_DEMUX_H_
#define _AML_MP_TUNERHAL_DEMUX_H_

#include "AmlDemuxBase.h"
#include "tunerhal/TunerService.h"

namespace aml_mp {
class AmlTunerHalDemux;

class TunerHalTsParser : public AmlDemuxBase::ITsParser
{
public:
    TunerHalTsParser(const std::function<FilterCallback>& cb);
    ~TunerHalTsParser();
    virtual int feedTs(const uint8_t* buffer, size_t size);
    virtual void reset();
    virtual int addDemuxFilter(int pid, const Aml_MP_DemuxFilterParams* params);
    virtual void removeDemuxFilter(int pid);

private:
    TunerHalTsParser(const TunerHalTsParser&) = delete;
    TunerHalTsParser& operator=(const TunerHalTsParser&) = delete;
};

class SectionFilterCallback : public TunerFilter::TunerFilterCallback {
public:
    SectionFilterCallback(){};
    ~SectionFilterCallback() {};
    virtual void onFilterEvent(const DemuxFilterEvent& filterEvent) override;
    virtual void onFilterStatus(const DemuxFilterStatus status) override;
    sptr<TunerFilter> mTunerFilter = nullptr;
    sptr<AmlTunerHalDemux> mAmlTunerHalDemux = nullptr;
    int mPid = 0;
};

class AmlTunerHalDemux : public AmlDemuxBase
{
public:
    AmlTunerHalDemux();
    ~AmlTunerHalDemux();
    virtual int open(bool isHardwareSource, Aml_MP_DemuxId demuxId, bool isSecureBuffer = false) override;
    virtual int start() override;
    virtual int feedTs(const uint8_t* buffer, size_t size) override;
    virtual int flush() override;
    virtual int stop() override;
    virtual int close() override;

    virtual int addDemuxFilter(int pid, const Aml_MP_DemuxFilterParams* params) override;
    virtual int removeDemuxFilter(int pid) override;
    virtual bool isStopped() const override;
    void notifyDataWrapper(int pid, const sptr<AmlMpBuffer>& data, int version);

private:
    TunerService& mTunerService;
    sptr<TunerHalTsParser> mTsParser = nullptr;
    sptr<TunerDemux> mTunerDemux = nullptr;
    sptr<TunerDvr> mTunerDvr = nullptr;

    std::map<int, sptr<TunerFilter>> mTunerFilterMap;

    AmlTunerHalDemux(const AmlTunerHalDemux&) = delete;
    AmlTunerHalDemux& operator= (const AmlTunerHalDemux&) = delete;
};

}

#endif // _AML_MP_TUNERHAL_DEMUX_H_
