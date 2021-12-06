#ifndef TUNER_SERVICE_H_
#define TUNER_SERVICE_H_

#include "utils/AmlMpRefBase.h"
#include "TunerCommon.h"
#include "TunerDemux.h"
#include "TunerDvr.h"
#include "TunerFilter.h"

namespace aml_mp {
/**
 * Wrapper of ITuner.hal
*/
class TunerService : public AmlMpRefBase {
public:
    static TunerService& instance();
    /**
     * Create a new instance of Demux.
    */
    sptr<TunerDemux> openDemux();
    DemuxCapabilities getDemuxCaps();
    void checkAvailable();
private:
    static TunerService* sTunerService;
    sp<ITuner> mTuner = nullptr;
    DemuxCapabilities mDemuxCapabilities;

    TunerService();
    ~TunerService();

    TunerService(const TunerService&) = delete;
    TunerService& operator= (const TunerService&) = delete;
};

} // namespace aml_mp

#endif // TUNER_SERVICE_H_