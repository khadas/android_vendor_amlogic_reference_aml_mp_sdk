#ifndef TUNER_FILTER_H_
#define TUNER_FILTER_H_

#include "TunerCommon.h"
#include "utils/AmlMpRefBase.h"

namespace aml_mp {
/**
 * Wrapper of IFilter.hal
*/
class TunerFilter : public AmlMpRefBase {
public:
    TunerFilter(sp<IFilter> filter);
    ~TunerFilter();
    void getDefTsAVFilterSettings(DemuxFilterSettings& filterSettings, int pid, bool isPassthrough);
    void getDefTsSectionFilterSettings(DemuxFilterSettings& filterSettings, int pid, bool checkCRC = true);
    void configure(const DemuxFilterSettings& settings);
    void start();
    int getId();
    size_t read(uint8_t* data, size_t size);
    void flush();
    void stop();
    void close();
    sp<IFilter> getHalFilter();
    // for exported callback interface
    class TunerFilterCallback : public AmlMpRefBase {
    public:
        // virtual void onFilterEvent_1_1(const DemuxFilterEvent& filterEvent, const DemuxFilterEventExt& filterEventExt);
        virtual void onFilterEvent(const DemuxFilterEvent& filterEvent);
        virtual void onFilterStatus(const DemuxFilterStatus status);
    };
    class FilterCallback : public IFilterCallback {
    public:
        FilterCallback(sptr<TunerFilterCallback> tunerFilterCallback): mTunerFilterCallback(tunerFilterCallback){};
        ~FilterCallback(){};
        // virtual Return<void> onFilterEvent_1_1(const DemuxFilterEvent& filterEvent, const DemuxFilterEventExt& filterEventExt);
        virtual Return<void> onFilterEvent(const DemuxFilterEvent& filterEvent);
        virtual Return<void> onFilterStatus(const DemuxFilterStatus status);
        sptr<TunerFilterCallback> mTunerFilterCallback = nullptr;
    };
private:
    sp<IFilter> mFilter = nullptr;
    std::shared_ptr<MessageQueue<uint8_t, kSynchronizedReadWrite>> mMq = nullptr;
    EventFlag* mFilterMQEventFlag = nullptr;
};

} // namespace aml_mp

#endif // TUNER_FILTER_H_