#include "TunerDvr.h"

namespace aml_mp {

TunerDvr::TunerDvr(sp<IDvr> dvr)
: mDvr(dvr) {
    Result res;
    mDvr->getQueueDesc([&](Result r, const MQDescriptorSync<uint8_t>& mq) {
        res = r;
        if (res == Result::SUCCESS) {
            mMq.reset(new MessageQueue<uint8_t, kSynchronizedReadWrite>(mq));
            EventFlag::createEventFlag(mMq->getEventFlagWord(), &mDvrMQEventFlag);
        }
    });
}

TunerDvr::~TunerDvr() {
    close();
}

void TunerDvr::getDefDvrSettings(DvrSettings& dvrSettings, DvrType dvrType) {
    memset(&dvrSettings, 0, sizeof(dvrSettings));
    switch (dvrType) {
        case DvrType::PLAYBACK:
            dvrSettings.playback({
                .dataFormat = DataFormat::TS,
                .packetSize = 188,
            });
            break;
        case DvrType::RECORD:
            dvrSettings.record({
                .dataFormat = DataFormat::TS,
                .packetSize = 188,
            });
            break;
        default:
            break;
    }
}

void TunerDvr::configure(const DvrSettings& settings) {
    mDvr->configure(settings);
}

size_t TunerDvr::feedData(const uint8_t* data, size_t size) {
    size_t available = mMq->availableToWrite();
    size_t newSize = std::min(available, size);
    if (newSize != size) {
        return 0;
    }
    if (mMq->write(data, newSize)) {
        mDvrMQEventFlag->wake(static_cast<uint32_t>(DemuxQueueNotifyBits::DATA_READY));
    } else {
        newSize = 0;
    }
    return newSize;
}

void TunerDvr::attachFilter(sptr<TunerFilter> tunerFilter) {
    sp<IFilter> hidlFilter = tunerFilter->getHalFilter();
    mDvr->attachFilter(hidlFilter);
}

void TunerDvr::detachFilter(sptr<TunerFilter> tunerFilter) {
    sp<IFilter> hidlFilter = tunerFilter->getHalFilter();
    mDvr->detachFilter(hidlFilter);
}

void TunerDvr::start() {
    mDvr->start();
}

void TunerDvr::flush() {
    mDvr->flush();
}

void TunerDvr::stop() {
    mDvr->stop();
}

void TunerDvr::close() {
    if (mDvrMQEventFlag) {
        EventFlag::deleteEventFlag(&mDvrMQEventFlag);
        mDvrMQEventFlag = nullptr;
    }
    mMq = nullptr;
    if (mDvr) {
        mDvr->close();
        mDvr = nullptr;
    }
}

Return<void> TunerDvr::DvrCallback::onRecordStatus(const RecordStatus status) {
    if (mTunerDvrCallback) {
        mTunerDvrCallback->onRecordStatus(status);
    }
    return Void();
}

Return<void> TunerDvr::DvrCallback::onPlaybackStatus(const PlaybackStatus status) {
    if (mTunerDvrCallback) {
        mTunerDvrCallback->onPlaybackStatus(status);
    }
    return Void();
}

} // namespace aml_mp
