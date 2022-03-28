/*
 * Copyright (c) 2021 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#define LOG_NDEBUG 0
#define LOG_TAG "AmlTunerHalDemux"
#include <utils/AmlMpLog.h>
#include <utils/AmlMpUtils.h>
#include <Aml_MP/Aml_MP.h>
#include "AmlTunerHalDemux.h"
#include <utils/AmlMpHandle.h>
#include <utils/AmlMpBuffer.h>
#include <utils/AmlMpEventLooper.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <sstream>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdint.h>

static const char* mName = LOG_TAG;

namespace aml_mp {

AmlTunerHalDemux::AmlTunerHalDemux()
: mTunerService(TunerService::instance()) {
    MLOGI("AmlTunerHalDemux created");
}

AmlTunerHalDemux::~AmlTunerHalDemux() {
    close();
}

int AmlTunerHalDemux::open(bool isHardwareSource, Aml_MP_DemuxId demuxId, bool isSecureBuffer) {
    AML_MP_UNUSED(isHardwareSource);
    AML_MP_UNUSED(demuxId);
    AML_MP_UNUSED(isSecureBuffer);
    MLOGI("AmlTunerHalDemux::open");
    if (mTsParser == nullptr) {
        mTsParser = new TunerHalTsParser([this](int pid, const sptr<AmlMpBuffer>& data, int version) {
            return notifyData(pid, data, version);
        });
    }
    // open demux
    mTunerDemux = mTunerService.openDemux();
    // open dvr
    mTunerDvr = mTunerDemux->openDvr(DvrType::PLAYBACK);
    return 0;
}

int AmlTunerHalDemux::start() {
    // start dvr
    DvrSettings dvrSettings;
    mTunerDvr->getDefDvrSettings(dvrSettings, DvrType::PLAYBACK);
    mTunerDvr->configure(dvrSettings);
    mTunerDvr->start();
    return 0;
}

int AmlTunerHalDemux::feedTs(const uint8_t* buffer, size_t size) {
    return mTunerDvr->feedData(buffer, size);
}

int AmlTunerHalDemux::flush() {
    mTunerDvr->flush();
    return 0;
}

int AmlTunerHalDemux::stop() {
    mTunerDvr->stop();
    return 0;
}

int AmlTunerHalDemux::close() {
    if (mTunerDvr) {
        mTunerDvr->close();
        mTunerDvr = nullptr;
    }
    if (mTunerDemux) {
        mTunerDemux->close();
        mTunerDemux = nullptr;
    }
    return 0;
}

int AmlTunerHalDemux::addDemuxFilter(int pid, const Aml_MP_DemuxFilterParams* params) {
    MLOGI("AmlTunerHalDemux::addPSISection: pid: %d", pid);
    if (mTunerFilterMap.find(pid) != mTunerFilterMap.end()) {
        return 0;
    }

    if (params->type != AML_MP_DEMUX_FILTER_PSI) {
        return 0;
    }

    DemuxFilterType filterType;
    memset(&filterType, 0, sizeof(filterType));
    filterType.mainType = DemuxFilterMainType::TS;
    filterType.subType.tsFilterType() = DemuxTsFilterType::SECTION;

    sptr<SectionFilterCallback> sectionCallback = new SectionFilterCallback();
    sptr<TunerFilter> tunerFilter = mTunerDemux->openFilter(filterType, 4096, sectionCallback);
    sectionCallback->mTunerFilter = tunerFilter;
    sectionCallback->mAmlTunerHalDemux = this;
    sectionCallback->mPid = pid;

    DemuxFilterSettings filterSettings;
    bool checkCRC = params->flags & DMX_CHECK_CRC;
    tunerFilter->getDefTsSectionFilterSettings(filterSettings, pid, checkCRC);
    tunerFilter->configure(filterSettings);
    tunerFilter->start();

    mTunerFilterMap.emplace(pid, tunerFilter);
    return 0;
}

int AmlTunerHalDemux::removeDemuxFilter(int pid) {
    MLOGI("AmlTunerHalDemux::removePSISection: pid: %d", pid);
    auto it = mTunerFilterMap.find(pid);
    if (it != mTunerFilterMap.end()) {
        it->second->stop();
        it->second->close();
        mTunerFilterMap.erase(pid);
    }
    return 0;
}

bool AmlTunerHalDemux::isStopped() const {
    return false;
}

void AmlTunerHalDemux::notifyDataWrapper(int pid, const sptr<AmlMpBuffer>& data, int version) {
    notifyData(pid, data, version);
}

void SectionFilterCallback::onFilterEvent(const DemuxFilterEvent& filterEvent) {
    MLOGI("SectionFilterCallback::onFilterEvent enter");
    std::vector<DemuxFilterEvent::Event> events = filterEvent.events;
    for (size_t i = 0; i < events.size(); i++) {
        DemuxFilterEvent::Event event = events[i];
        DemuxFilterSectionEvent sectionEvent = event.section();
        size_t dataLength = sectionEvent.dataLength;

        sptr<AmlMpBuffer> data = new AmlMpBuffer(dataLength);
        size_t readLen = mTunerFilter->read(data->base(), dataLength);
        MLOGI("SectionFilterCallback::onFilterEvent event datalen: %d, readLen: %d", dataLength, readLen);
        mAmlTunerHalDemux->notifyDataWrapper(mPid, data, sectionEvent.version);
    }
    MLOGI("SectionFilterCallback::onFilterEvent exit");
}

void SectionFilterCallback::onFilterStatus(const DemuxFilterStatus filterEvent) {
    AML_MP_UNUSED(filterEvent);
}

/**
 * TunerHalTsParser
*/
TunerHalTsParser::TunerHalTsParser(const std::function<FilterCallback>& cb)
: ITsParser(cb) {

}

TunerHalTsParser::~TunerHalTsParser() {
    reset();
}

int TunerHalTsParser::feedTs(const uint8_t* buffer, size_t size) {
    AML_MP_UNUSED(buffer);
    return size;
}

void TunerHalTsParser::reset() {

}

int TunerHalTsParser::addDemuxFilter(int pid, const Aml_MP_DemuxFilterParams* params) {
    AML_MP_UNUSED(pid);
    AML_MP_UNUSED(params);
    return 0;
}

void TunerHalTsParser::removeDemuxFilter(int pid) {
    AML_MP_UNUSED(pid);

}

} // namespace aml_mp
