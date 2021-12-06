#ifndef TUNER_DVR_H_
#define TUNER_DVR_H_

#include "utils/AmlMpRefBase.h"
#include "TunerCommon.h"
#include "TunerFilter.h"

namespace aml_mp {
/**
 * Wrapper of IDvr.hal
*/
class TunerDvr : public AmlMpRefBase {
public:
    TunerDvr(sp<IDvr> dvr);
    ~TunerDvr();
    void getDefDvrSettings(DvrSettings& dvrSettings, DvrType dvrType);
    void configure(const DvrSettings& settings);
    size_t feedData(const uint8_t* data, size_t size);
    void attachFilter(sptr<TunerFilter> tunerFilter);
    void detachFilter(sptr<TunerFilter> tunerFilter);
    void start();
    void flush();
    void stop();
    void close();
    // for exported callback interface
    class TunerDvrCallback : public AmlMpRefBase {
    public:
        virtual void onRecordStatus(const RecordStatus status);
        virtual void onPlaybackStatus(const PlaybackStatus status);
    };
    class DvrCallback : public IDvrCallback {
    public:
        DvrCallback(sptr<TunerDvrCallback> tunerDvrCallback): mTunerDvrCallback(tunerDvrCallback) {};
        ~DvrCallback(){};
        virtual Return<void> onRecordStatus(const RecordStatus status);
        virtual Return<void> onPlaybackStatus(const PlaybackStatus status);
    private:
        sptr<TunerDvrCallback> mTunerDvrCallback = nullptr;
    };
private:
    sp<IDvr> mDvr = nullptr;
    std::shared_ptr<MessageQueue<uint8_t, kSynchronizedReadWrite>> mMq = nullptr;
    EventFlag* mDvrMQEventFlag = nullptr;
};

} // namespace aml_mp

#endif // TUNER_DVR_H_