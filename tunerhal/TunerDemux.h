#ifndef TUNER_DEMUX_H_
#define TUNER_DEMUX_H_

#include "utils/AmlMpRefBase.h"
#include "TunerCommon.h"
#include "TunerDvr.h"
#include "TunerFilter.h"

namespace aml_mp {
/**
 * Wrapper of IDemux.hal
*/
class TunerDemux : public AmlMpRefBase {
public:
    TunerDemux(sp<IDemux> demux);
    ~TunerDemux();
    sptr<TunerDvr> openDvr(DvrType dvrType, int bufferSize = 16 * 188 * 1024, sptr<TunerDvr::TunerDvrCallback> tunerDvrCallback = nullptr);
    sptr<TunerFilter> openFilter(DemuxFilterType filterType, int bufferSize = 16 * 188 * 1024, const sptr<TunerFilter::TunerFilterCallback> tunerFilterCallback = nullptr);
    int getAvSyncHwId(sptr<TunerFilter> tunerFilter);
    void close();
private:
    sp<IDemux> mDemux = nullptr;
    DemuxCapabilities mDemuxCapabilities;
};

} // namespace aml_mp

#endif // TUNER_DEMUX_H_