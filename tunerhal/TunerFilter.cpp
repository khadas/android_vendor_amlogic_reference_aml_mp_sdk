#include "TunerFilter.h"
#include "TunerService.h"

namespace aml_mp {

TunerFilter::TunerFilter(sp<IFilter> filter)
: mFilter(filter) {
    MQDescriptorSync<uint8_t> filterMQDesc;
    Result res;
    mFilter->getQueueDesc(
            [&](Result r, const MQDescriptorSync<uint8_t>& mqDesc) {
                filterMQDesc = mqDesc;
                res = r;
            });
    if (res == Result::SUCCESS) {
        mMq.reset(new MessageQueue<uint8_t, kSynchronizedReadWrite>(filterMQDesc));
        EventFlag::createEventFlag(mMq->getEventFlagWord(), &mFilterMQEventFlag);
    }
}

TunerFilter::~TunerFilter() {
    close();
}

void TunerFilter::getDefTsAVFilterSettings(DemuxFilterSettings& filterSettings, int pid, bool isPassthrough) {
    memset(&filterSettings, 0, sizeof(filterSettings));
    filterSettings.ts().tpid = pid;
    filterSettings.ts().filterSettings.av({
        .isPassthrough = isPassthrough,
    });
}

void TunerFilter::getDefTsSectionFilterSettings(DemuxFilterSettings& filterSettings, int pid, bool checkCRC) {
    memset(&filterSettings, 0, sizeof(filterSettings));
    DemuxCapabilities demuxCaps = TunerService::instance().getDemuxCaps();
    filterSettings.ts().tpid = pid;
    filterSettings.ts().filterSettings.section({
        .isCheckCrc = checkCRC,
        .isRepeat = false,
        .isRaw = false,
    });

    std::vector<uint8_t> filter(demuxCaps.numBytesInSectionFilter);
    std::vector<uint8_t> mask(demuxCaps.numBytesInSectionFilter);
    std::vector<uint8_t> mode(demuxCaps.numBytesInSectionFilter);

    filterSettings.ts().filterSettings.section().condition.sectionBits({
        .filter = filter,
        .mask = mask,
        .mode = mode,
    });
}

void TunerFilter::configure(const DemuxFilterSettings& settings) {
    mFilter->configure(settings);
}

void TunerFilter::start() {
    mFilter->start();
}

int TunerFilter::getId() {
    int filterId = -1;
    Result res;
    mFilter->getId([&](Result r, const int& id) {
        res = r;
        if (res == Result::SUCCESS) {
            filterId = id;
        }
    });

    if (res != Result::SUCCESS) {
        return -1;
    }
    return filterId;
}

size_t TunerFilter::read(uint8_t* data, size_t size) {
    if (mMq->read(data, size)) {
        return size;
    }
    return 0;
}

void TunerFilter::flush() {
    mFilter->flush();
}

void TunerFilter::stop() {
    mFilter->stop();
}

void TunerFilter::close() {
    if (mFilterMQEventFlag) {
        EventFlag::deleteEventFlag(&mFilterMQEventFlag);
        mFilterMQEventFlag = nullptr;
    }
    mMq = nullptr;
    if (mFilter) {
        mFilter->close();
        mFilter = nullptr;
    }
}

sp<IFilter> TunerFilter::getHalFilter() {
    return mFilter;
}

// Return<void> TunerDemux::FilterCallback::onFilterEvent_1_1(const DemuxFilterEvent& filterEvent, const DemuxFilterEventExt& filterEventExt) {
//     if (mFilterEventCallback) {
//         mFilterEventCallback->onFilterEvent_1_1(filterEvent, filterEventExt);
//     }
//     return Void();
// }

Return<void> TunerFilter::FilterCallback::onFilterEvent(const DemuxFilterEvent& filterEvent) {
    if (mTunerFilterCallback) {
        mTunerFilterCallback->onFilterEvent(filterEvent);
    }
    return Void();
}

Return<void> TunerFilter::FilterCallback::onFilterStatus(const DemuxFilterStatus status) {
    if (mTunerFilterCallback) {
        mTunerFilterCallback->onFilterStatus(status);
    }
    return Void();
}

} // namespace aml_mp