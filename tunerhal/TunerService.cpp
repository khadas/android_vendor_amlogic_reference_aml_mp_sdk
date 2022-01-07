#include "TunerService.h"

namespace aml_mp {

TunerService* TunerService::sTunerService = nullptr;

TunerService::TunerService() {
    mTuner = ITuner::getService();
    memset(&mDemuxCapabilities, 0, sizeof(mDemuxCapabilities));
    mTuner->getDemuxCaps([&](Result r, DemuxCapabilities caps) {
        if (r == Result::SUCCESS) {
            mDemuxCapabilities = caps;
        }
    });
}

TunerService::~TunerService() {
}

TunerService& TunerService::instance() {
    static std::once_flag s_onceFlag;
    std::call_once(s_onceFlag, [] {
        sTunerService = new TunerService();
    });
    sTunerService->checkAvailable();
    return *sTunerService;
}

void TunerService::checkAvailable() {
    Result res;
    mTuner->getDemuxCaps([&](Result r, DemuxCapabilities caps __unused) {
        res = r;
    });
    if (res != Result::SUCCESS) {
        // TunerService may be crashed, retry to get TunerService
        mTuner = ITuner::getService();
        memset(&mDemuxCapabilities, 0, sizeof(mDemuxCapabilities));
        mTuner->getDemuxCaps([&](Result r, DemuxCapabilities caps) {
            if (r == Result::SUCCESS) {
                mDemuxCapabilities = caps;
            }
        });
    }
}

sptr<TunerDemux> TunerService::openDemux() {
    Result res;
    uint32_t id;
    sp<IDemux> demuxSp = nullptr;
    sptr<TunerDemux> tunerDemux = nullptr;
    mTuner->openDemux([&](Result r, uint32_t demuxId, const sp<IDemux>& demux) {
        demuxSp = demux;
        id = demuxId;
        res = r;
    });
    if (res != Result::SUCCESS) {
        return nullptr;
    }
    tunerDemux = new TunerDemux(demuxSp);
    return tunerDemux;
}

DemuxCapabilities TunerService::getDemuxCaps() {
    return mDemuxCapabilities;
}

} // namespace aml_mp
