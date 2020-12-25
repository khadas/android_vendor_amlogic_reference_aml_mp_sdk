/*
 * Copyright (c) 2020 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#define LOG_NDEBUG 0
#define KEEP_ALOGX
#define LOG_TAG "AmlMpPlayerImpl"
#include "Aml_MP_PlayerImpl.h"
#include <utils/AmlMpLog.h>
#include <utils/AmlMpUtils.h>
#include <utils/AmlMpConfig.h>
#include <sstream>
#include <mutex>
#include <condition_variable>
#include <media/stagefright/foundation/ADebug.h>
#include "AmlPlayerBase.h"
#ifndef __ANDROID_VNDK__
#include <gui/Surface.h>
#include <gui/SurfaceComposerClient.h>
#endif

namespace aml_mp {

///////////////////////////////////////////////////////////////////////////////
AmlMpPlayerImpl::AmlMpPlayerImpl(const Aml_MP_PlayerCreateParams* createParams)
: mInstanceId(AmlMpPlayerRoster::instance().registerPlayer(this))
, mCreateParams(*createParams)
{
    snprintf(mName, sizeof(mName), "%s_%d", LOG_TAG, mInstanceId);

    MLOG();

    memset(&mVideoParams, 0, sizeof(mVideoParams));
    mVideoParams.pid = AML_MP_INVALID_PID;
    memset(&mAudioParams, 0, sizeof(mAudioParams));
    mAudioParams.pid = AML_MP_INVALID_PID;
    memset(&mSubtitleParams, 0, sizeof(mSubtitleParams));
    mSubtitleParams.pid = AML_MP_INVALID_PID;

    mWorkmode = AML_MP_PLAYER_MODE_NORMAL;

    AmlMpConfig::instance().init();
}

AmlMpPlayerImpl::~AmlMpPlayerImpl()
{
    MLOG();

    CHECK(mState == STATE_IDLE);
    CHECK(mStreamState == ALL_STREAMS_STOPPED);

    AmlMpPlayerRoster::instance().unregisterPlayer(mInstanceId);
}

int AmlMpPlayerImpl::registerEventCallback(Aml_MP_PlayerEventCallback cb, void* userData)
{
    AML_MP_TRACE(10);

    if (mState != STATE_IDLE) {
        return -1;
    }

    mEventCb = cb;
    mUserData = userData;

    return 0;
}

int AmlMpPlayerImpl::setVideoParams(const Aml_MP_VideoParams* params)
{
    AML_MP_TRACE(10);

    if (mStreamState & VIDEO_STARTED) {
        ALOGE("video started already!");
        return -1;
    }

    mVideoParams.pid = params->pid;
    mVideoParams.videoCodec = params->videoCodec;
    mVideoParams.width = params->width;
    mVideoParams.height = params->height;
    mVideoParams.frameRate = params->frameRate;
    memcpy(mVideoParams.extraData, params->extraData, sizeof(mVideoParams.extraData));
    mVideoParams.extraDataSize = params->extraDataSize;

    return 0;
}

int AmlMpPlayerImpl::setAudioParams(const Aml_MP_AudioParams* params)
{
    AML_MP_TRACE(10);

    if (mStreamState & AUDIO_STARTED) {
        ALOGE("audio started already!");
        return -1;
    }

    mAudioParams.pid = params->pid;
    mAudioParams.audioCodec = params->audioCodec;
    mAudioParams.nChannels = params->nChannels;
    mAudioParams.nSampleRate = params->nSampleRate;
    memcpy(mAudioParams.extraData, params->extraData, sizeof(mAudioParams.extraData));
    mAudioParams.extraDataSize = params->extraDataSize;

    return 0;
}

int AmlMpPlayerImpl::setSubtitleParams(const Aml_MP_SubtitleParams* params)
{
    AML_MP_TRACE(10);

    if (mStreamState & SUBTITLE_STARTED) {
        ALOGE("subtitle started already!");
        return -1;
    }

    memcpy(&mSubtitleParams, params, sizeof(Aml_MP_SubtitleParams));

    return 0;
}

int AmlMpPlayerImpl::setCASParams(const Aml_MP_CASParams* params)
{
    AML_MP_TRACE(10);

    mCASParams = *params;

    return 0;
}

int AmlMpPlayerImpl::start()
{
    AML_MP_TRACE(10);

    if (mState == STATE_IDLE) {
        if (prepare() < 0) {
            ALOGE("prepare failed!");
            return -1;
        }
    }

    setParams();
    int ret = mPlayer->start();
    if (ret < 0) {
        ALOGE("%s failed!", __FUNCTION__);
    }

    setState(STATE_RUNNING);

    //CHECK: assume start always be success if param exist
    if (mAudioParams.pid != AML_MP_INVALID_PID) {
        mStreamState |= AUDIO_STARTED;
    }

    if (mVideoParams.pid != AML_MP_INVALID_PID) {
        mStreamState |= VIDEO_STARTED;
    }

    if (mSubtitleParams.pid != AML_MP_INVALID_PID) {
        mStreamState |= SUBTITLE_STARTED;
    }

    return ret;
}

int AmlMpPlayerImpl::stop()
{
    AML_MP_TRACE(10);

    if (mState == STATE_RUNNING || mState == STATE_PAUSED) {
        if (mPlayer) {
            mPlayer->stop();
        }

        mStreamState &= ~(AUDIO_STARTED | VIDEO_STARTED | SUBTITLE_STARTED);
    }

    return resetIfNeeded();
}

int AmlMpPlayerImpl::pause()
{
    AML_MP_TRACE(10);
    RETURN_IF(-1, mPlayer == nullptr);

    if (mState != STATE_RUNNING) {
        return 0;
    }

    if (mPlayer->pause() < 0) {
        return -1;
    }

    setState(STATE_PAUSED);
    return 0;
}

int AmlMpPlayerImpl::resume()
{
    AML_MP_TRACE(10);
    RETURN_IF(-1, mPlayer == nullptr);

    if (mState != STATE_PAUSED) {
        return 0;
    }

    if (mPlayer->resume() < 0) {
        return -1;
    }

    setState(STATE_RUNNING);
    return 0;
}

int AmlMpPlayerImpl::flush()
{
    AML_MP_TRACE(10);
    RETURN_IF(-1, mPlayer == nullptr);

    return mPlayer->flush();
}

int AmlMpPlayerImpl::setPlaybackRate(float rate)
{
    AML_MP_TRACE(10);
    mPlaybackRate = rate;
    RETURN_IF(-1, mPlayer == nullptr);

    return mPlayer->setPlaybackRate(rate);
}

int AmlMpPlayerImpl::switchAudioTrack(const Aml_MP_AudioParams* params)
{
    AML_MP_TRACE(10);
    RETURN_IF(-1, mPlayer == nullptr);

    return mPlayer->switchAudioTrack(params);
}

int AmlMpPlayerImpl::switchSubtitleTrack(const Aml_MP_SubtitleParams* params)
{
    AML_MP_TRACE(10);
    RETURN_IF(-1, mPlayer == nullptr);

    return mPlayer->switchSubtitleTrack(params);
}

int AmlMpPlayerImpl::writeData(const uint8_t* buffer, size_t size)
{
    RETURN_IF(-1, mPlayer == nullptr);

    if (mCasHandle != nullptr) {
        mCasHandle->processEcm(buffer, size);
    }

    return mPlayer->writeData(buffer, size);
}

int AmlMpPlayerImpl::writeEsData(Aml_MP_StreamType type, const uint8_t* buffer, size_t size, int64_t pts)
{
    RETURN_IF(-1, mPlayer == nullptr);

    return mPlayer->writeEsData(type, buffer, size, pts);
}

int AmlMpPlayerImpl::getCurrentPts(Aml_MP_StreamType streamType, int64_t* pts)
{
    RETURN_IF(-1, mPlayer == nullptr);

    return mPlayer->getCurrentPts(streamType, pts);
}

int AmlMpPlayerImpl::getBufferStat(Aml_MP_BufferStat* bufferStat)
{
    RETURN_IF(-1, mPlayer == nullptr);

    return mPlayer->getBufferStat(bufferStat);
}

int AmlMpPlayerImpl::setAnativeWindow(void* nativeWindow)
{
    AML_MP_TRACE(10);
    if (nativeWindow == nullptr) {
        mNativeWindow = nullptr;
    } else {
        mNativeWindow = static_cast<ANativeWindow*>(nativeWindow);
    }
    ALOGI("setAnativeWindow: %p, mNativewindow: %p", nativeWindow, mNativeWindow.get());

    if (mPlayer != nullptr) {
        mPlayer->setANativeWindow(mNativeWindow.get());
    }
    return 0;
}

int AmlMpPlayerImpl::setVideoWindow(int x, int y, int width, int height)
{
    AML_MP_TRACE(10);
#ifndef __ANDROID_VNDK__
    if (mNativeWindow == nullptr) {
        ALOGI("Nativewindow is null, create it");
        mComposerClient = new android::SurfaceComposerClient;
        mComposerClient->initCheck();

        mSurfaceControl = mComposerClient->createSurface(android::String8("AmlMpPlayer"), width, height, android::PIXEL_FORMAT_RGB_565, 0);
        mSurfaceControl->isValid();
        mSurface = mSurfaceControl->getSurface();
        android::SurfaceComposerClient::Transaction()
            .setFlags(mSurfaceControl, android::layer_state_t::eLayerOpaque, android::layer_state_t::eLayerOpaque)
            .show(mSurfaceControl)
            .apply();

        mNativeWindow = mSurface;
    }

    if (mSurfaceControl != nullptr) {
        ALOGI("Set video window size: x %d, y %d, width: %d, height: %d", x, y, width, height);
        auto transcation = android::SurfaceComposerClient::Transaction();
        if (x >= 0 && y >= 0) {
            transcation.setPosition(mSurfaceControl, x, y);
        }

        transcation.setSize(mSurfaceControl, width, height);
        transcation.setCrop_legacy(mSurfaceControl, android::Rect(width, height));
        transcation.setLayer(mSurfaceControl, mZorder);

        transcation.apply();
    }
#endif
    mVideoWindow = {x, y, width, height};
    RETURN_IF(-1, mPlayer == nullptr);

    return mPlayer->setVideoWindow(x, y, width, height);
}

int AmlMpPlayerImpl::setVolume(float volume)
{
    AML_MP_TRACE(10);
    mVolume = volume;
    RETURN_IF(-1, mPlayer == nullptr);

    return mPlayer->setVolume(volume);
}

int AmlMpPlayerImpl::getVolume(float* volume)
{
    AML_MP_TRACE(10);
    RETURN_IF(-1, mPlayer == nullptr);

    return mPlayer->getVolume(volume);
}

int AmlMpPlayerImpl::showVideo()
{
    AML_MP_TRACE(10);
    RETURN_IF(-1, mPlayer == nullptr);

    return mPlayer->showVideo();
}

int AmlMpPlayerImpl::hideVideo()
{
    AML_MP_TRACE(10);
    RETURN_IF(-1, mPlayer == nullptr);

    return mPlayer->hideVideo();
}

int AmlMpPlayerImpl::showSubtitle()
{
    AML_MP_TRACE(10);
    RETURN_IF(-1, mPlayer == nullptr);

    return mPlayer->showSubtitle();
}

int AmlMpPlayerImpl::hideSubtitle()
{
    AML_MP_TRACE(10);
    RETURN_IF(-1, mPlayer == nullptr);

    return mPlayer->hideSubtitle();
}

int AmlMpPlayerImpl::setParameter(Aml_MP_PlayerParameterKey key, void* parameter)
{
    AML_MP_TRACE(10);
    int ret = 0;

    switch (key) {
    case AML_MP_PLAYER_PARAMETER_AUDIO_BALANCE:
    {
        mAudioBalance = *(Aml_MP_AudioBalance*)parameter;
    }
    break;

    case AML_MP_PLAYER_PARAMETER_WORK_MODE:
    {
        mWorkmode = *(Aml_MP_PlayerWorkMode*)parameter;
        ALOGI("Set workmode: %d", mWorkmode);
    }
    break;

    case AML_MP_PLAYER_PARAMETER_VIDEO_WINDOW_ZORDER:
    {
        mZorder = *(int*)parameter;
        ALOGI("Set zorder: %d", mZorder);

#ifndef __ANDROID_VNDK__
        if (mSurfaceControl != nullptr) {
            auto transcation = android::SurfaceComposerClient::Transaction();

            transcation.setLayer(mSurfaceControl, mZorder);

            transcation.apply();
        }
#endif
        return 0;
    }
    break;

    default:
        break;
    }

    if (mPlayer != nullptr) {
        ret = mPlayer->setParameter(key, parameter);
    }

    return ret;
}

int AmlMpPlayerImpl::getParameter(Aml_MP_PlayerParameterKey key, void* parameter)
{
    AML_MP_TRACE(10);
    RETURN_IF(-1, mPlayer == nullptr);

    return mPlayer->getParameter(key, parameter);
}

int AmlMpPlayerImpl::setAVSyncSource(Aml_MP_AVSyncSource syncSource)
{
    AML_MP_TRACE(10);
    mSyncSource = syncSource;

    return 0;
}

int AmlMpPlayerImpl::setPcrPid(int pid)
{
    AML_MP_TRACE(10);
    mPcrPid = pid;

    return 0;
}

int AmlMpPlayerImpl::startVideoDecoding()
{
    AML_MP_TRACE(10);
    MLOG();

    if (mState == STATE_IDLE) {
        if (prepare() < 0) {
            ALOGE("prepare failed!");
            return -1;
        }
    }

    setParams();
    int ret = mPlayer->startVideoDecoding();
    if (ret < 0) {
        ALOGE("%s failed!", __FUNCTION__);
        return -1;
    }

    setState(STATE_RUNNING);
    if (mVideoParams.pid != AML_MP_INVALID_PID) {
        mStreamState |= VIDEO_STARTED;
    }

    return ret;
}

int AmlMpPlayerImpl::stopVideoDecoding()
{
    AML_MP_TRACE(10);
    RETURN_IF(-1, mPlayer == nullptr);

    if (mState == STATE_RUNNING || mState == STATE_PAUSED) {
        if (mPlayer) {
            mPlayer->stopVideoDecoding();
        }

        mStreamState &= ~VIDEO_STARTED;
    }

    return resetIfNeeded();
}

int AmlMpPlayerImpl::startAudioDecoding()
{
    AML_MP_TRACE(10);
    MLOG();

    if (mState == STATE_IDLE) {
        if (prepare() < 0) {
            ALOGE("prepare failed!");
            return -1;
        }
    }

    setParams();
    int ret = mPlayer->startAudioDecoding();
    if (ret < 0) {
        ALOGE("%s failed!", __FUNCTION__);
        return -1;
    }

    setState(STATE_RUNNING);
    if (mAudioParams.pid != AML_MP_INVALID_PID) {
        mStreamState |= AUDIO_STARTED;
    }

    return ret;
}

int AmlMpPlayerImpl::stopAudioDecoding()
{
    AML_MP_TRACE(10);
    RETURN_IF(-1, mPlayer == nullptr);

    if (mState == STATE_RUNNING || mState == STATE_PAUSED) {
        if (mPlayer) {
            mPlayer->stopAudioDecoding();
        }

        mStreamState &= ~AUDIO_STARTED;
    }

    return resetIfNeeded();
}

int AmlMpPlayerImpl::startSubtitleDecoding()
{
    AML_MP_TRACE(10);
    RETURN_IF(-1, mPlayer == nullptr);

    if (mState == STATE_IDLE) {
        if (prepare() < 0) {
            ALOGE("prepare failed!");
            return -1;
        }
    }

    setParams();
    int ret = mPlayer->startSubtitleDecoding();
    if (ret < 0) {
        ALOGE("%s failed!", __FUNCTION__);
        return -1;
    }

    setState(STATE_RUNNING);
    if (mSubtitleParams.pid != AML_MP_INVALID_PID) {
        mStreamState |= SUBTITLE_STARTED;
    }

    return ret;
}

int AmlMpPlayerImpl::stopSubtitleDecoding()
{
    AML_MP_TRACE(10);
    RETURN_IF(-1, mPlayer == nullptr);

    if (mState == STATE_RUNNING || mState == STATE_PAUSED) {
        if (mPlayer) {
            mPlayer->stopSubtitleDecoding();
        }

        mStreamState &= ~SUBTITLE_STARTED;
    }

    return resetIfNeeded();
}

int AmlMpPlayerImpl::setSubtitleWindow(int x, int y, int width, int height)
{
    AML_MP_TRACE(10);
    mSubtitleWindow = {x, y, width, height};
    RETURN_IF(-1, mPlayer == nullptr);

    return mPlayer->setSubtitleWindow(x, y, width, height);
}

int AmlMpPlayerImpl::startDescrambling()
{
    AML_MP_TRACE(10);

    ALOGI("encrypted stream!");
    if (mCASParams.type == AML_MP_CAS_UNKNOWN) {
        ALOGE("cas params is invalid!");
        return -1;
    }

    mCasHandle = AmlCasBase::create(mCreateParams.sourceType, &mCASParams);
    if (mCasHandle == nullptr) {
        ALOGE("create CAS handle failed!");
        return -1;
    }

    mCasHandle->openSession();

    return 0;
}

int AmlMpPlayerImpl::stopDescrambling()
{
    AML_MP_TRACE(10);

    if (mCasHandle) {
        mCasHandle->closeSession();
        mCasHandle.clear();
    }

    return 0;
}

int AmlMpPlayerImpl::setADParams(Aml_MP_AudioParams* params)
{
    AML_MP_TRACE(10);
    RETURN_IF(-1, mPlayer == nullptr);

    mADParams.pid = params->pid;
    mADParams.audioCodec = params->audioCodec;
    mADParams.nChannels = params->nChannels;
    mADParams.nSampleRate = params->nSampleRate;
    memcpy(mADParams.extraData, params->extraData, sizeof(mADParams.extraData));
    mADParams.extraDataSize = params->extraDataSize;

    return 0;
}

///////////////////////////////////////////////////////////////////////////////
const char* AmlMpPlayerImpl::stateString(State state)
{
    switch (state) {
    case STATE_IDLE:
        return "STATE_IDLE";
    case STATE_PREPARED:
        return "STATE_PREPARED";
    case STATE_RUNNING:
        return "STATE_RUNNING";
    case STATE_PAUSED:
        return "STATE_PAUSED";
    }
}

std::string AmlMpPlayerImpl::streamStateString(int streamState)
{
    std::stringstream ss;
    bool hasValue = false;

    if (streamState & AUDIO_STARTED) {
        if (hasValue) {
            ss << "|";
        }
        ss << "AUDIO_STARTED";
        hasValue = true;
    }

    if (streamState & VIDEO_STARTED) {
        if (hasValue) {
            ss << "|";
        }
        ss << "VIDEO_STARTED";
        hasValue = true;
    }

    if (streamState & SUBTITLE_STARTED) {
        if (hasValue) {
            ss << "|";
        }
        ss << "SUBTITLE_STARTED";
        hasValue = true;
    }

    if (!hasValue) {
        ss << "STREAM_STOPPED";
    }

    return ss.str();
}

void AmlMpPlayerImpl::setState(State state)
{
    if (mState != state) {
        ALOGI("%s -> %s", stateString(mState), stateString(state));
        mState = state;
    }
}

int AmlMpPlayerImpl::prepare()
{
    MLOG();

    if (mCreateParams.drmMode == AML_MP_INPUT_STREAM_ENCRYPTED) {
        startDescrambling();
    }

    if (mPlayer == nullptr) {
        mPlayer = AmlPlayerBase::create(&mCreateParams, mInstanceId);
    }

    if (mPlayer == nullptr) {
        ALOGE("AmlPlayerBase create failed!");
        return -1;
    }

    ALOGI("mWorkmode: %d", mWorkmode);
    mPlayer->setParameter(AML_MP_PLAYER_PARAMETER_WORK_MODE, &mWorkmode);

    mPlayer->registerEventCallback(mEventCb, mUserData);

    if ((mSubtitleWindow.width > 0) && (mSubtitleWindow.height > 0)) {
        mPlayer->setSubtitleWindow(mSubtitleWindow.x, mSubtitleWindow.y, mSubtitleWindow.width, mSubtitleWindow.height);
    }

    ALOGI("mNativeWindow:%p", mNativeWindow.get());
    mPlayer->setANativeWindow(mNativeWindow.get());

    if (mSyncSource == AML_MP_AVSYNC_SOURCE_DEFAULT) {
        mSyncSource = AML_MP_AVSYNC_SOURCE_PCR;
    }

    mPlayer->setAVSyncSource(mSyncSource);

    if (mSyncSource == AML_MP_AVSYNC_SOURCE_PCR && mPcrPid != AML_MP_INVALID_PID) {
        mPlayer->setPcrPid(mPcrPid);
    }

    setState(STATE_PREPARED);

    return 0;
}

void AmlMpPlayerImpl::setParams()
{
    if (mVideoParams.pid != AML_MP_INVALID_PID) {
        mPlayer->setVideoParams(&mVideoParams);
    }

    if (mAudioParams.pid != AML_MP_INVALID_PID) {
        mPlayer->setAudioParams(&mAudioParams);
    }

    if (mSubtitleParams.subtitleCodec != AML_MP_CODEC_UNKNOWN) {
        mPlayer->setSubtitleParams(&mSubtitleParams);
    }

}

int AmlMpPlayerImpl::resetIfNeeded()
{
    MLOG();

    if (mState == STATE_RUNNING || mState == STATE_PAUSED) {
        if (mStreamState == ALL_STREAMS_STOPPED) {
            setState(STATE_PREPARED);
        } else {
            ALOGE("current streamState:%s", streamStateString(mStreamState).c_str());
        }
    }

    if (mState == STATE_PREPARED) {
        reset();
    }

    return 0;
}

int AmlMpPlayerImpl::reset()
{
    MLOG();

    if (mCasHandle) {
        stopDescrambling();
    }

    mPlayer.clear();

    setState(STATE_IDLE);

    return 0;
}

///////////////////////////////////////////////////////////////////////////////
AmlMpPlayerRoster* AmlMpPlayerRoster::sAmlPlayerRoster = nullptr;

AmlMpPlayerRoster::AmlMpPlayerRoster()
: mPlayers{nullptr}
{

}

AmlMpPlayerRoster::~AmlMpPlayerRoster()
{

}

AmlMpPlayerRoster& AmlMpPlayerRoster::instance()
{
    static std::once_flag s_onceFlag;

    if (sAmlPlayerRoster == nullptr) {
        std::call_once(s_onceFlag, [] {
            sAmlPlayerRoster = new AmlMpPlayerRoster();
        });
    }

    return *sAmlPlayerRoster;
}

int AmlMpPlayerRoster::registerPlayer(void* player)
{
    if (player == nullptr) {
        return -1;
    }

    std::lock_guard<std::mutex> _l(mLock);
    for (size_t i = 0; i < kPlayerInstanceMax; ++i) {
        if (mPlayers[i] == nullptr) {
            mPlayers[i] = player;
            (void)++mPlayerNum;

            //ALOGI("register player id:%d(%p)", i, player);
            return i;
        }
    }

    return -1;
}

void AmlMpPlayerRoster::unregisterPlayer(int id)
{
    std::lock_guard<std::mutex> _l(mLock);

    CHECK(mPlayers[id]);
    mPlayers[id] = nullptr;
    (void)--mPlayerNum;
}

void AmlMpPlayerRoster::signalAmTsPlayerId(int id)
{
    std::lock_guard<std::mutex> _l(mLock);
    mAmtsPlayerId = id;
}

bool AmlMpPlayerRoster::isAmTsPlayerExist() const
{
    std::lock_guard<std::mutex> _l(mLock);
    return mAmtsPlayerId != -1;
}



}
