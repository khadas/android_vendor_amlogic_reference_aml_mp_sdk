#ifndef TUNER_COMMON_H_
#define TUNER_COMMON_H_

#include <android/hardware/tv/tuner/1.0/ITuner.h>
#include <android/hardware/tv/tuner/1.0/IDemux.h>
#include <android/hardware/tv/tuner/1.0/IFilter.h>
#include <android/hardware/tv/tuner/1.0/IFilterCallback.h>
#include <android/hardware/tv/tuner/1.0/IDvr.h>
#include <android/hardware/tv/tuner/1.0/IDvrCallback.h>
#include <fmq/MessageQueue.h>

using android::sp;
using android::hardware::Return;
using android::hardware::Void;
using android::hardware::MQDescriptorSync;
using android::hardware::EventFlag;
using android::hardware::MessageQueue;
using namespace android::hardware::tv::tuner::V1_0;

#endif /* TUNER_COMMON_H_ */