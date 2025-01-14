/*
 * Copyright (c) 2020 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#ifndef _AML_TS_PLAYER_H_
#define _AML_TS_PLAYER_H_

#include "AmlPlayerBase.h"
#include <AmTsPlayer.h>
#include <queue>
#include <utils/AmlMpUtils.h>
#include <utils/AmlMpBuffer.h>
#include <utils/AmlMpThread.h>
#include <utils/AmlMpRefBase.h>
#ifdef ANDROID
namespace android {
class NativeHandle;
}
#endif
struct ANativeWindow;

namespace aml_mp {
class AmlTsPlayer : public aml_mp::AmlPlayerBase
{
public:
    AmlTsPlayer(Aml_MP_PlayerCreateParams* createParams, int instanceId);
    ~AmlTsPlayer();
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
    int writeEsData_l(Aml_MP_StreamType type, const uint8_t* buffer, size_t size, int64_t pts);
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
    int stopVideoDecoding() override;
    int pauseVideoDecoding() override;
    int resumeVideoDecoding() override;

    int startAudioDecoding() override;
    int stopAudioDecoding() override;
    int startADDecoding() override;
    int stopADDecoding() override;
    int pauseAudioDecoding() override;
    int resumeAudioDecoding() override;
    int setADParams(const Aml_MP_AudioParams* params, bool enableMix) override;
    int setADVolume(float volume) override;
    int getADVolume(float* volume) override;
    int getMediaSyncId() override;
    int getPlayerId() override;
protected:
    am_tsplayer_handle mPlayer{0};
private:
    void eventCallback(am_tsplayer_event* event);

    char mName[50];
    am_tsplayer_init_params init_param = {TS_MEMORY, TS_INPUT_BUFFER_TYPE_NORMAL, 0, 0};
    const int kRwTimeout = 30000;
    int mVideoTunnelId = -1;
    ANativeWindow* mNativewindow = nullptr;
    int mBlackOut = 0;
    bool mVideoParaSeted;
    bool mAudioParaSeted;

    int32_t mApid = AML_MP_INVALID_PID;
    int mPacketsizefd = -1;
    sptr<AmlMpBuffer> mPacktsBuffer;
    unsigned mAudioContinuityCounter;
    int packetize(
            bool isAudio, const char *buffer_add,
            size_t buffer_size,
            sptr<AmlMpBuffer> *packets,
            uint32_t flags,
            const uint8_t *PES_private_data, size_t PES_private_data_len,
            size_t numStuffingBytes, int64_t timeUs);
    int incrementContinuityCounter(int isAudio);
    class AudioEsDataFeedThread : virtual public AmlMpThread {
    public:
        AudioEsDataFeedThread(sptr<AmlTsPlayer> player) : mPlayer(player) {
            snprintf(mName, sizeof(mName), "AudioEsDataFeedThread");
        };
        ~AudioEsDataFeedThread() {};
        bool threadLoop() override;
        void start();
        int writeEsData(const uint8_t* buffer, size_t size, int64_t pts);
        void pause();
        void flush();
        void resume();
        void stop();
        struct AudioEsBuffer {
            uint8_t *addr;
            size_t size;
            int64_t pts;
        };
        sptr<AmlTsPlayer> mPlayer;
        mutable std::mutex mLock;
        bool mPaused = false;
        std::queue<struct AudioEsBuffer> mAudioEsQueue;
        char mName[50];
    };

    sptr<AudioEsDataFeedThread> mAudioEsDataFeedThread;

private:
    AmlTsPlayer(const AmlTsPlayer&) = delete;
    AmlTsPlayer& operator= (const AmlTsPlayer&) = delete;
};

}

#endif
