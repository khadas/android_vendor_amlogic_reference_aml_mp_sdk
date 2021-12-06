#include "TunerDemux.h"

namespace aml_mp {

TunerDemux::TunerDemux(sp<IDemux> demux)
: mDemux(demux) {
}

TunerDemux::~TunerDemux() {
    close();
}

sptr<TunerDvr> TunerDemux::openDvr(DvrType dvrType, int bufferSize, sptr<TunerDvr::TunerDvrCallback> tunerDvrCallback) {
    sptr<TunerDvr> tunerDvr = nullptr;
    Result res;
    sp<IDvrCallback> callback = new TunerDvr::DvrCallback(tunerDvrCallback);
    sp<IDvr> hidlDvr;
    mDemux->openDvr(dvrType, bufferSize, callback,
            [&](Result r, const sp<IDvr>& dvr) {
                hidlDvr = dvr;
                res = r;
            });
    if (res != Result::SUCCESS) {
        return nullptr;
    }
    tunerDvr = new TunerDvr(hidlDvr);
    return tunerDvr;
}

sptr<TunerFilter> TunerDemux::openFilter(DemuxFilterType filterType, int bufferSize, const sptr<TunerFilter::TunerFilterCallback> tunerFilterCallback) {
    sptr<TunerFilter> tunerFilter = nullptr;
    Result res;
    sp<IFilter> filterSp;
    sp<IFilterCallback> cbSp = new TunerFilter::FilterCallback(tunerFilterCallback);
    mDemux->openFilter(filterType, bufferSize, cbSp,
            [&](Result r, const sp<IFilter>& filter) {
                filterSp = filter;
                res = r;
            });
    if (res != Result::SUCCESS) {
        return nullptr;
    }
    tunerFilter = new TunerFilter(filterSp);
    return tunerFilter;
}

int TunerDemux::getAvSyncHwId(sptr<TunerFilter> tunerFilter) {
    uint32_t avSyncHwId = 0;
    Result res;
    sp<IFilter> halFilter = tunerFilter->getHalFilter();
    mDemux->getAvSyncHwId(halFilter,
            [&](Result r, uint32_t id) {
                res = r;
                avSyncHwId = id;
            });
    if (res != Result::SUCCESS) {
        return -1;
    }
    return avSyncHwId;
}

void TunerDemux::close() {
    if (mDemux) {
        mDemux->close();
        mDemux = nullptr;
    }
}

} // namespace aml_mp
