/*
 * Copyright (c) 2021 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#ifndef _AML_TV_PLAYER_H_
#define _AML_TV_PLAYER_H_

#include "AmlPlayerBase.h"
#ifdef ANDROID
namespace android {
class NativeHandle;
}
#endif
struct ANativeWindow;

#include "tunerhal/TunerService.h"
#include "tunerhal/MediaCodecWrapper.h"
#include "tunerhal/AudioTrackWrapper.h"

namespace aml_mp {
#ifdef ANDROID
using android::NativeHandle;
#endif

class AmlTvPlayer : public aml_mp::AmlPlayerBase
{
public:
    AmlTvPlayer(Aml_MP_PlayerCreateParams* createParams, int instanceId);
    ~AmlTvPlayer();
    int setANativeWindow(ANativeWindow* nativeWindow);

    int initCheck() const override;
    int setVideoParams(const Aml_MP_VideoParams* params) override;
    int setAudioParams(const Aml_MP_AudioParams* params) override;
    int start() override;
    int stop() override;
    int pause() override;
    int resume() override;
    int flush() override;
    int setPlaybackRate(float rate) override;
    int getPlaybackRate(float* rate) override;
    int switchAudioTrack(const Aml_MP_AudioParams* params) override;
    int writeData(const uint8_t* buffer, size_t size) override;
    int writeEsData(Aml_MP_StreamType type, const uint8_t* buffer, size_t size, int64_t pts) override;
    int getCurrentPts(Aml_MP_StreamType type, int64_t* pts) override;
    int getFirstPts(Aml_MP_StreamType type, int64_t* pts) override;
    int getBufferStat(Aml_MP_BufferStat* bufferStat) override;
    int setVideoWindow(int x, int y, int width, int height) override;
    int setVolume(float volume) override;
    int getVolume(float* volume) override;
    int showVideo() override;
    int hideVideo() override;
    int setParameter(Aml_MP_PlayerParameterKey key, void* parameter) override;
    int getParameter(Aml_MP_PlayerParameterKey key, void* parameter) override;
    int setAVSyncSource(Aml_MP_AVSyncSource syncSource) override;
    int setPcrPid(int pid) override;

    int startVideoDecoding() override;
    int pauseVideoDecoding() override;
    int resumeVideoDecoding() override;
    int stopVideoDecoding() override;

    int startAudioDecoding() override;
    int pauseAudioDecoding() override;
    int resumeAudioDecoding() override;
    int stopAudioDecoding() override;
    int startADDecoding() override;
    int stopADDecoding() override;
    int setADParams(const Aml_MP_AudioParams* params, bool enableMix) override;

private:
    char mName[50];

    ANativeWindow* mNativewindow = nullptr;
    bool mVideoParaSeted = false;
    bool mAudioParaSeted = false;
    Aml_MP_VideoParams mVideoParams;
    Aml_MP_AudioParams mAudioParams;

    TunerService& mTunerService;
    sptr<TunerDvr> mTunerDvr = nullptr;
    sptr<TunerDemux> mTunerDemux = nullptr;
    sptr<TunerFilter> mVideoFilter = nullptr;
    sptr<TunerFilter> mAudioFilter = nullptr;

    sptr<MediaCodecWrapper> mVideoCodecWrapper = nullptr;
    sptr<AudioTrackWrapper> mAudioTrackWrapper = nullptr;
private:
    AmlTvPlayer(const AmlTvPlayer&) = delete;
    AmlTvPlayer& operator= (const AmlTvPlayer&) = delete;
};

}

#endif
