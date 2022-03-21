/*
 * Copyright (c) 2021 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#define LOG_TAG "AmlTvPlayer"
#include <utils/Log.h>
#include "AmlTvPlayer.h"
#include <utils/AmlMpUtils.h>
#include <media/AudioSystem.h>
#include "Aml_MP_PlayerImpl.h"

#ifdef ANDROID
#include <system/window.h>
#include <amlogic/am_gralloc_ext.h>
#ifndef __ANDROID_VNDK__
#include <gui/Surface.h>
using namespace android;
#endif
#endif


namespace aml_mp {

AmlTvPlayer::AmlTvPlayer(Aml_MP_PlayerCreateParams* createParams, int instanceId)
: aml_mp::AmlPlayerBase(createParams, instanceId)
, mTunerService(TunerService::instance()) {
    snprintf(mName, sizeof(mName), "%s_%d", LOG_TAG, instanceId);

    MLOGI("demuxId: %s", mpDemuxId2Str(createParams->demuxId));
    if (createParams->demuxId == AML_MP_DEMUX_ID_DEFAULT) {
        createParams->demuxId = AML_MP_HW_DEMUX_ID_0;
    }
    memset(&mVideoParams, 0, sizeof(mVideoParams));
    memset(&mAudioParams, 0, sizeof(mAudioParams));
}

int AmlTvPlayer::initCheck() const {
    return 0;
}

AmlTvPlayer::~AmlTvPlayer() {
    MLOGI("%s:%d", __FUNCTION__, __LINE__);
}

int AmlTvPlayer::setANativeWindow(ANativeWindow* nativeWindow) {
    MLOGI("AmlTvPlayer::setANativeWindow: %p", nativeWindow);
    mNativewindow = nativeWindow;
    return 0;
}

int AmlTvPlayer::setVideoParams(const Aml_MP_VideoParams* params) {
    if (params->videoCodec == AML_MP_CODEC_UNKNOWN || params->pid == AML_MP_INVALID_PID) {
        MLOGI("amtsplayer invalid video pid or codecid.\n");
        mVideoParaSeted = false;
        memset(&mVideoParams, 0, sizeof(mVideoParams));
    } else {
        MLOGI("amtsplayer video params seted.\n");
        mVideoParaSeted = true;
        memcpy(&mVideoParams, params, sizeof(mVideoParams));
    }
    return 0;
}

int AmlTvPlayer::setAudioParams(const Aml_MP_AudioParams* params) {
    if (params->audioCodec == AML_MP_CODEC_UNKNOWN || params->pid == AML_MP_INVALID_PID) {
        MLOGI("amtsplayer invalid audio pid or codecid.\n");
        mAudioParaSeted = false;
        memset(&mAudioParams, 0, sizeof(mAudioParams));
    } else {
        MLOGI("amtsplayer audio params seted.\n");
        mAudioParaSeted = true;
        memcpy(&mAudioParams, params, sizeof(mAudioParams));
    }
    return 0;
}

int AmlTvPlayer::start() {
    MLOGI("Call start");
    if (mVideoParaSeted || mAudioParaSeted) {
        // ITuner.OpenDemux and IDemux.openDvr to prepare to writeData
        mTunerDemux = mTunerService.openDemux();
        mTunerDvr = mTunerDemux->openDvr(DvrType::PLAYBACK);
        DvrSettings dvrSettings;
        mTunerDvr->getDefDvrSettings(dvrSettings, DvrType::PLAYBACK);
        mTunerDvr->configure(dvrSettings);
        mTunerDvr->start();
    }
    if (mVideoParaSeted) {
        // Demux.openFilter to get video FilterId
        // Demux.getAvSyncHwId to get video hw-av-sync-id
        DemuxFilterType filterType;
        memset(&filterType, 0, sizeof(filterType));
        filterType.mainType = DemuxFilterMainType::TS;
        filterType.subType.tsFilterType() = DemuxTsFilterType::VIDEO;
        mVideoFilter = mTunerDemux->openFilter(filterType);
        DemuxFilterSettings filterSettings;
        mVideoFilter->getDefTsAVFilterSettings(filterSettings, mVideoParams.pid, true);
        mVideoFilter->configure(filterSettings);
        int filterId = mVideoFilter->getId();
        int hwAvSyncId = mTunerDemux->getAvSyncHwId(mVideoFilter);
        MLOGI("video: filterId: %x, hwAvSyncId:%x", filterId, hwAvSyncId);
        MediaCodecParams mediaCodecParams {
            .codecId = mVideoParams.videoCodec,
            .nativewindow = mNativewindow,
            .audioHwSync = AudioSystem::getAudioHwSyncForSession((audio_session_t)AudioSystem::newAudioUniqueId(AUDIO_UNIQUE_ID_USE_SESSION)),
            .filterId = filterId,
            .hwAvSyncId = hwAvSyncId,
            .isPassthrough = true,
        };
        mVideoCodecWrapper = new MediaCodecWrapper();
        mVideoCodecWrapper->configure(mediaCodecParams);
        mVideoCodecWrapper->start();
    }
    if (mAudioParaSeted) {
        // Demux.openFilter to get audio FilterId
        // Demux.getAvSyncHwId to get audio hw-av-sync-id
        DemuxFilterType filterType;
        memset(&filterType, 0, sizeof(filterType));
        filterType.mainType = DemuxFilterMainType::TS;
        filterType.subType.tsFilterType() = DemuxTsFilterType::AUDIO;
        mAudioFilter = mTunerDemux->openFilter(filterType);
        DemuxFilterSettings filterSettings;
        mAudioFilter->getDefTsAVFilterSettings(filterSettings, mAudioParams.pid, true);
        mAudioFilter->configure(filterSettings);
        int filterId = mAudioFilter->getId();
        int hwAvSyncId = mTunerDemux->getAvSyncHwId(mAudioFilter);
        MLOGI("audio: filterId: %x, hwAvSyncId:%x", filterId, hwAvSyncId);
        mAudioTrackWrapper = new AudioTrackWrapper();
        mAudioTrackWrapper->configure(mAudioParams, filterId, hwAvSyncId);
        mAudioTrackWrapper->start();
    }
    return 0;
}

int AmlTvPlayer::stop() {
    MLOGI("Call stop");
    if (mVideoCodecWrapper) {
        mVideoCodecWrapper->stop();
        mVideoCodecWrapper->release();
        mVideoCodecWrapper = nullptr;
    }
    if (mAudioTrackWrapper) {
        mAudioTrackWrapper->stop();
        mAudioTrackWrapper = nullptr;
    }
    mVideoFilter->close();
    mVideoFilter = nullptr;
    mAudioFilter->close();
    mAudioFilter = nullptr;
    mTunerDvr->stop();
    mTunerDvr->close();
    mTunerDvr = nullptr;
    mTunerDemux->close();
    mTunerDemux = nullptr;
    return 0;
}

int AmlTvPlayer::writeData(const uint8_t* buffer, size_t size) {
    return mTunerDvr->feedData(buffer, size);
}

int AmlTvPlayer::pause() {
    MLOGI("Call pause");

    mAudioTrackWrapper->pause();

    return 0;
}

int AmlTvPlayer::resume() {
    MLOGI("Call resume");

    mAudioTrackWrapper->resume();

    return 0;
}

int AmlTvPlayer::flush() {
    MLOGI("Call flush");
    mTunerDvr->flush();
    mVideoFilter->flush();
    mAudioFilter->flush();
    mAudioTrackWrapper->flush();
    mVideoCodecWrapper->flush();
    return 0;
}

int AmlTvPlayer::setPlaybackRate(float rate) {
    MLOGI("setPlaybackRate, rate: %f", rate);
    return 0;
}

int AmlTvPlayer::getPlaybackRate(float* rate) {
    AML_MP_UNUSED(rate);
    MLOGI("getPlaybackRate not implemented");
    return AML_MP_ERROR_INVALID_OPERATION;
}

int AmlTvPlayer::switchAudioTrack(const Aml_MP_AudioParams* params) {
    AML_MP_UNUSED(params);
    return 0;
}

int AmlTvPlayer::writeEsData(Aml_MP_StreamType type, const uint8_t* buffer, size_t size, int64_t pts) {
    AML_MP_UNUSED(type);
    AML_MP_UNUSED(buffer);
    AML_MP_UNUSED(size);
    AML_MP_UNUSED(pts);
    return size;
}

int AmlTvPlayer::getCurrentPts(Aml_MP_StreamType type, int64_t* pts) {
    AML_MP_UNUSED(type);
    AML_MP_UNUSED(pts);
    return 0;
}

int AmlTvPlayer::getFirstPts(Aml_MP_StreamType type, int64_t* pts) {
    AML_MP_UNUSED(type);
    AML_MP_UNUSED(pts);
    return 0;
}

int AmlTvPlayer::getBufferStat(Aml_MP_BufferStat* bufferStat) {
    AML_MP_UNUSED(bufferStat);
    return 0;
}

int AmlTvPlayer::setVideoWindow(int x, int y, int width, int height) {
    MLOGI("setVideoWindow, x: %d, y: %d, width: %d, height: %d", x, y, width, height);
    return 0;
}

int AmlTvPlayer::setVolume(float volume) {
    MLOGI("setVolume, tsplayer_volume: %f", volume);
    return 0;
}

int AmlTvPlayer::getVolume(float* volume) {
    if (volume) {
        MLOGI("getVolume volume: %f", *volume);
    }
    return 0;
}

int AmlTvPlayer::showVideo() {
    return 0;
}

int AmlTvPlayer::hideVideo() {
    return 0;
}

int AmlTvPlayer::setParameter(Aml_MP_PlayerParameterKey key, void* parameter) {
    AML_MP_UNUSED(parameter);
    MLOGI("Call setParameter, key is %s", mpPlayerParameterKey2Str(key));
    return 0;
}

int AmlTvPlayer::getParameter(Aml_MP_PlayerParameterKey key, void* parameter) {
    AML_MP_UNUSED(parameter);
    MLOGD("Call getParameter, key is %s", mpPlayerParameterKey2Str(key));
    return 0;
}

int AmlTvPlayer::setAVSyncSource(Aml_MP_AVSyncSource syncSource) {
    MLOGI("setsyncmode, syncSource %s!!!", mpAVSyncSource2Str(syncSource));
    return 0;
}

int AmlTvPlayer::setPcrPid(int pid) {
    AML_MP_UNUSED(pid);
    return 0;
}

int AmlTvPlayer::startVideoDecoding() {
    return 0;
}

int AmlTvPlayer::pauseVideoDecoding() {
    return 0;
}

int AmlTvPlayer::resumeVideoDecoding() {
    return 0;
}

int AmlTvPlayer::stopVideoDecoding() {
    return 0;
}

int AmlTvPlayer::startAudioDecoding() {
    return 0;
}

int AmlTvPlayer::pauseAudioDecoding() {
    return 0;
}

int AmlTvPlayer::resumeAudioDecoding() {
    return 0;
}

int AmlTvPlayer::stopAudioDecoding() {
    return 0;
}

int AmlTvPlayer::startADDecoding() {
    return 0;
}

int AmlTvPlayer::stopADDecoding() {
    return 0;
}

int AmlTvPlayer::setADParams(const Aml_MP_AudioParams* params, bool enableMix) {
    AML_MP_UNUSED(params);
    AML_MP_UNUSED(enableMix);
    return 0;
}

int AmlTvPlayer::setADVolume(float volume) {
    AML_MP_UNUSED(volume);
    return 0;
}

int AmlTvPlayer::getADVolume(float* volume) {
    AML_MP_UNUSED(volume);
    return 0;
}

} // namespace aml_mp
