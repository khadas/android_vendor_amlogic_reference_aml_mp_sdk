/*
 * Copyright (c) 2020 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */
#define LOG_TAG "AmlCTCPlayer"
#include <utils/AmlMpLog.h>
#include "AmlCTCPlayer.h"
#include <utils/AmlMpUtils.h>
#include <system/window.h>
#include <dlfcn.h>

static const char* mName = LOG_TAG;

namespace aml_mp {
////////////////////////////////////////////////////////////////////////////////

#define FUNC_DEF(def, type) \
    typedef type; \
    pf##def* def;

#define FUNC_SYMBOL(v) FUNC_SYMBOL_RET(v, -1)

#define FUNC_SYMBOL_RET(v, r) \
    do { \
        if (v == nullptr) { \
            v = (decltype(v))dlsym(mHalPlayerHandle, #v); \
            if (v == nullptr) {\
                MLOGE("%s:%d dlsym(%s) failed: %s", __FUNCTION__, __LINE__, #v, dlerror()); \
                return r; \
            } \
        } \
    } while (0);

class AmlHalPlayer : public AmlMpRefBase
{
public:
    AmlHalPlayer();
    ~AmlHalPlayer();

    bool initCheck() {
        return mHalPlayerHandle != nullptr;
    }

    int getVersion(const char** versionString = nullptr) {
        FUNC_SYMBOL(AML_AVPLAY_GetVersion);
        return AML_AVPLAY_GetVersion(versionString);
    }

    int getCodecCapabilities(int type, std::vector<int>& codecs) {
        FUNC_SYMBOL(AML_AVPLAY_GetCodecCapabilities);
        return AML_AVPLAY_GetCodecCapabilities(type, codecs);
    }

    int getDefaultConfig(AML_AVPLAY_ATTR_S* attr, AML_AVPLAY_STREAM_TYPE_E stream_type) {
        FUNC_SYMBOL(AML_AVPLAY_GetDefaultConfig);
        return AML_AVPLAY_GetDefaultConfig(attr, stream_type);
    }

    int create(AML_AVPLAY_ATTR_S* attr, AML_HANDLE* mAVPlay, int zorder) {
        FUNC_SYMBOL(AML_AVPLAY_Create);
        return AML_AVPLAY_Create(attr, mAVPlay, zorder);
    }

    int setStreamType(AML_HANDLE mAVPlay, AML_AVPLAY_STREAM_TYPE_E stream_type) {
        FUNC_SYMBOL(AML_AVPLAY_SetStreamType);
        return AML_AVPLAY_SetStreamType(mAVPlay, stream_type);
    }

    int registerEvent64(AML_HANDLE mAVPlay, AML_TVS_EVENT event_func, AML_VOID* para) {
        FUNC_SYMBOL(AML_AVPLAY_RegisterEvent64);
        return AML_AVPLAY_RegisterEvent64(mAVPlay, event_func, para);
    }

    int unRegisterEvent(AML_HANDLE mAVPlay, AML_AVPLAY_EVENT_E event) {
        FUNC_SYMBOL(AML_AVPLAY_UnRegisterEvent);
        return AML_AVPLAY_UnRegisterEvent(mAVPlay, event);
    }

    int getAttr(AML_HANDLE mAVPlay, AML_AVPLAY_ATTR_ID_E attr_id, AML_AVPLAY_ATTR_U* attr) {
        FUNC_SYMBOL(AML_AVPLAY_GetAttr);
        return AML_AVPLAY_GetAttr(mAVPlay, attr_id, attr);
    }

    int setAttr(AML_HANDLE mAVPlay, AML_AVPLAY_ATTR_ID_E attr_id, AML_AVPLAY_ATTR_U* attr) {
        FUNC_SYMBOL(AML_AVPLAY_SetAttr);
        return AML_AVPLAY_SetAttr(mAVPlay, attr_id, attr);
    }

    int pause(AML_HANDLE mAVPlay, AML_AVPLAY_PAUSE_OPT_S* options) {
        FUNC_SYMBOL(AML_AVPLAY_Pause);
        return AML_AVPLAY_Pause(mAVPlay, options);
    }

    int resume(AML_HANDLE mAVPlay, AML_AVPLAY_RESUME_OPT_S* options) {
        FUNC_SYMBOL(AML_AVPLAY_Resume);
        return AML_AVPLAY_Resume(mAVPlay, options);
    }

    int start(AML_HANDLE mAVPlay, AML_AVPLAY_START_OPT_S* options) {
        FUNC_SYMBOL(AML_AVPLAY_Start);
        return AML_AVPLAY_Start(mAVPlay, options);
    }

    int stop(AML_HANDLE mAVPlay, AML_AVPLAY_STOP_OPT_S* options) {
        FUNC_SYMBOL(AML_AVPLAY_Stop);
        return AML_AVPLAY_Stop(mAVPlay, options);
    }

    int flushStream(AML_HANDLE mAVPlay, AML_AVPLAY_FLUSH_STREAM_OPT_S* options) {
        FUNC_SYMBOL(AML_AVPLAY_FlushStream);
        return AML_AVPLAY_FlushStream(mAVPlay, options);
    }

    int reset(AML_HANDLE mAVPlay, AML_AVPLAY_RESET_OPT_S* options) {
        FUNC_SYMBOL(AML_AVPLAY_Reset);
        return AML_AVPLAY_Reset(mAVPlay, options);
    }

    int destroy(AML_HANDLE mAVPlay, int zorder) {
        FUNC_SYMBOL(AML_AVPLAY_Destroy);
        return AML_AVPLAY_Destroy(mAVPlay, zorder);
    }

    int setDecodeMode(AML_HANDLE mAVPlay, AML_AVPLAY_VCODEC_MODE_E vcodec_mode) {
        FUNC_SYMBOL(AML_AVPLAY_SetDecodeMode);
        return AML_AVPLAY_SetDecodeMode(mAVPlay, vcodec_mode);
    }

    int getPlaySpeed(AML_HANDLE mAVPlay, float* speed) {
        FUNC_SYMBOL(AML_AVPLAY_GetPlaySpeed);
        return AML_AVPLAY_GetPlaySpeed(mAVPlay, speed);
    }

    int tplay(AML_HANDLE mAVPlay, AML_AVPLAY_TPLAY_OPT_S* options) {
        FUNC_SYMBOL(AML_AVPLAY_Tplay);
        return AML_AVPLAY_Tplay(mAVPlay, options);
    }

    int getStatusInfo(AML_HANDLE mAVPlay, AML_AVPLAY_STATUS_INFO_S* status) {
        FUNC_SYMBOL(AML_AVPLAY_GetStatusInfo);
        return AML_AVPLAY_GetStatusInfo(mAVPlay, status);
    }

    int setVideoSize(AML_HANDLE mAVPlay, int32_t x, int32_t y, int32_t w, int32_t h) {
        FUNC_SYMBOL(AML_AVPLAY_SetVideoSize);
        return AML_AVPLAY_SetVideoSize(mAVPlay, x, y, w, h);
    }

    native_handle_t* getSidebandHandle(AML_HANDLE mAVPlay) {
        FUNC_SYMBOL_RET(AML_PLAYER_Get_SideBandHandle, nullptr);
        return AML_PLAYER_Get_SideBandHandle(mAVPlay);
    }

    int setSurface(AML_HANDLE mAVPlay, ANativeWindow* surface) {
        FUNC_SYMBOL(AML_AVPLAY_SetSurface);
        return AML_AVPLAY_SetSurface(mAVPlay, surface);
    }

    int setVideoTunnelId(AML_HANDLE mAVPlay, int videoTunnelId) {
        FUNC_SYMBOL(AML_AVPLAY_SetVideoTunnelId);
        return AML_AVPLAY_SetVideoTunnelId(mAVPlay, videoTunnelId);
    }

    int setNetworkJitter(AML_HANDLE mAVPlay, AML_HANDLE demux, int delayMs) {
        FUNC_SYMBOL(AML_AVPLAY_SetNetworkJitter);
        return AML_AVPLAY_SetNetworkJitter(mAVPlay, demux, delayMs);
    }

    int setAudioOutputType(AML_HANDLE mAVPlay, AUDIO_OUTPUT_TYPE audioOutputType) {
        FUNC_SYMBOL(AML_AVPLAY_SetAudioOutputType);
        return AML_AVPLAY_SetAudioOutputType(mAVPlay, audioOutputType);
    }

    int setAudioPtsOffset(AML_HANDLE mAVPlay, int ptsOffset) {
        FUNC_SYMBOL(AML_AVPLAY_SetAudioPtsOffset);
        return AML_AVPLAY_SetAudioPtsOffset(mAVPlay, ptsOffset);
    }

    int setVideoPtsOffset(AML_HANDLE mAVPlay, int ptsOffset) {
        FUNC_SYMBOL(AML_AVPLAY_SetVideoPtsOffset);
        return AML_AVPLAY_SetVideoPtsOffset(mAVPlay, ptsOffset);
    }

    int setScreenMode(AML_HANDLE mAVPlay, int screenMode) {
        FUNC_SYMBOL(AML_AVPLAY_SetScreenMode);
        return AML_AVPLAY_SetScreenMode(mAVPlay, screenMode);
    }

    int pushEsBuffer(AML_HANDLE mAVPlay, AML_PLAYER_MEDIA_CHAN_E chn, const uint8_t* data, size_t nSize, int64_t us) {
        FUNC_SYMBOL(AML_AVPLAY_PushEsBuffer);
        return AML_AVPLAY_PushEsBuffer(mAVPlay, chn, data, nSize, us);
    }

    int pushEsBuffer90K(AML_HANDLE mAVPlay, AML_PLAYER_MEDIA_CHAN_E chn, const uint8_t* data, size_t nSize, int64_t pts) {
        FUNC_SYMBOL(AML_AVPLAY_PushEsBuffer90K);
        return AML_AVPLAY_PushEsBuffer90K(mAVPlay, chn, data, nSize, pts);
    }

    int setVolume(AML_HANDLE mAVPlay, float volume) {
        FUNC_SYMBOL(AML_AVPLAY_SetVolume);
        return AML_AVPLAY_SetVolume(mAVPlay, volume);
    }

    int chnOpen(AML_HANDLE mAVPlay, AML_PLAYER_MEDIA_CHAN_E change_type, AML_PLAYER_OPEN_OPT_U* options) {
        FUNC_SYMBOL(AML_PLAYER_ChnOpen);
        return AML_PLAYER_ChnOpen(mAVPlay, change_type, options);
    }

    int chnClose(AML_HANDLE mAVPlay, AML_PLAYER_MEDIA_CHAN_E change_type, AML_PLAYER_CLOSE_OPT_U* options) {
        FUNC_SYMBOL(AML_PLAYER_ChnClose);
        return AML_PLAYER_ChnClose(mAVPlay, change_type, options);
    }

    int getAttr(AML_HANDLE mAVPlay, AML_PLAYER_ATTR_ID_E attr, AML_PLAYER_ATTR_U* options) {
        FUNC_SYMBOL(AML_PLAYER_GetAttr);
        return AML_PLAYER_GetAttr(mAVPlay, attr, options);
    }

    int setAttr(AML_HANDLE mAVPlay, AML_PLAYER_ATTR_ID_E attr, AML_PLAYER_ATTR_U* options) {
        FUNC_SYMBOL(AML_PLAYER_SetAttr);
        return AML_PLAYER_SetAttr(mAVPlay, attr, options);
    }

    int start(AML_HANDLE mAVPlay, AML_PLAYER_MEDIA_CHAN_E change_type, AML_PLAYER_START_OPT_U* options) {
        FUNC_SYMBOL(AML_PLAYER_Start);
        return AML_PLAYER_Start(mAVPlay, change_type, options);
    }

    int stop(AML_HANDLE mAVPlay, AML_PLAYER_MEDIA_CHAN_E change_type, AML_PLAYER_STOP_OPT_U* options) {
        FUNC_SYMBOL(AML_PLAYER_Stop);
        return AML_PLAYER_Stop(mAVPlay, change_type, options);
    }

    int getConfig(AML_HANDLE mAVPlay, AML_PLAYER_MEDIA_CHAN_E change_type, AML_PLAYER_CONFIG_E config_type, AML_PLAYER_CONFIG_OPT_U* options) {
        FUNC_SYMBOL(AML_PLAYER_GetConfig);
        return AML_PLAYER_GetConfig(mAVPlay, change_type, config_type, options);
    }

    int setConfig(AML_HANDLE mAVPlay, AML_PLAYER_MEDIA_CHAN_E change_type, AML_PLAYER_CONFIG_E config_type, AML_PLAYER_CONFIG_OPT_U* options) {
        FUNC_SYMBOL(AML_PLAYER_SetConfig);
        return AML_PLAYER_SetConfig(mAVPlay, change_type, config_type, options);
    }

    int getCCEsData(AML_HANDLE mAVPlay, uint8_t* data, int nSize) {
        FUNC_SYMBOL(AML_PLAYER_GetCCEsData);
        return AML_PLAYER_GetCCEsData(mAVPlay, data, nSize);
    }

    AML_HANDLE openDemux(int zorder) {
        FUNC_SYMBOL_RET(AML_DMX_OpenDemux, nullptr);
        return AML_DMX_OpenDemux(zorder);
    }

    int closeDemux(AML_HANDLE demux) {
        FUNC_SYMBOL(AML_DMX_CloseDemux);
        return AML_DMX_CloseDemux(demux);
    }

    int startDemux(AML_HANDLE demux) {
        FUNC_SYMBOL(AML_DMX_StartDemux);
        return AML_DMX_StartDemux(demux);
    }

    int stopDemux(AML_HANDLE demux) {
        FUNC_SYMBOL(AML_DMX_StopDemux);
        return AML_DMX_StopDemux(demux);
    }

    int flushDemux(AML_HANDLE demux) {
        FUNC_SYMBOL(AML_DMX_FlushDemux);
        return AML_DMX_FlushDemux(demux);
    }

    AML_HANDLE createChannel(AML_HANDLE demux, int pid) {
        FUNC_SYMBOL_RET(AML_DMX_CreateChannel, nullptr);
        return AML_DMX_CreateChannel(demux, pid);
    }

    int destroyChannel(AML_HANDLE demux, AML_HANDLE channel) {
        FUNC_SYMBOL(AML_DMX_DestroyChannel);
        return AML_DMX_DestroyChannel(demux, channel);
    }

    int openChannel(AML_HANDLE demux, AML_HANDLE channel) {
        FUNC_SYMBOL(AML_DMX_OpenChannel);
        return AML_DMX_OpenChannel(demux, channel);
    }

    int closeChannel(AML_HANDLE demux, AML_HANDLE channel) {
        FUNC_SYMBOL(AML_DMX_CloseChannel);
        return AML_DMX_CloseChannel(demux, channel);
    }

    AML_HANDLE createFilter(AML_HANDLE demux, AML_SECTION_FILTER_CB cb, AML_VOID* pUserData, int id) {
        FUNC_SYMBOL_RET(AML_DMX_CreateFilter, nullptr);
        return AML_DMX_CreateFilter(demux, cb, pUserData, id);
    }

    int destroyFilter(AML_HANDLE demux, AML_HANDLE filter) {
        FUNC_SYMBOL(AML_DMX_DestroyFilter);
        return AML_DMX_DestroyFilter(demux, filter);
    }

    int attachFilter(AML_HANDLE demux, AML_HANDLE filter, AML_HANDLE channel) {
        FUNC_SYMBOL(AML_DMX_AttachFilter);
        return AML_DMX_AttachFilter(demux, filter, channel);
    }

    int detachFilter(AML_HANDLE demux, AML_HANDLE filter, AML_HANDLE channel) {
        FUNC_SYMBOL(AML_DMX_DetachFilter);
        return AML_DMX_DetachFilter(demux, filter, channel);
    }

    int pushTsBuffer(AML_HANDLE mAVPlay, AML_HANDLE demux, const uint8_t* data, size_t nSize) {
        FUNC_SYMBOL(AML_DMX_PushTsBuffer);
        return AML_DMX_PushTsBuffer(mAVPlay, demux, data, nSize);
    }

    int getParameters(const char* keys) {
        FUNC_SYMBOL(AML_AUDIOSYSTEM_GetParameters);
        return AML_AUDIOSYSTEM_GetParameters(keys);
    }

    int setParameters(const char* keyValuePairs) {
        FUNC_SYMBOL(AML_AUDIOSYSTEM_SetParameters);
        return AML_AUDIOSYSTEM_SetParameters(keyValuePairs);
    }

private:
    FUNC_DEF(AML_AVPLAY_GetVersion,
            int pfAML_AVPLAY_GetVersion(const char**));

    FUNC_DEF(AML_AVPLAY_GetCodecCapabilities,
            int pfAML_AVPLAY_GetCodecCapabilities(int type, std::vector<int>& codecs));

    FUNC_DEF(AML_AVPLAY_GetDefaultConfig,
            int pfAML_AVPLAY_GetDefaultConfig(AML_AVPLAY_ATTR_S* attr, AML_AVPLAY_STREAM_TYPE_E stream_type));

    FUNC_DEF(AML_AVPLAY_Create,
            int pfAML_AVPLAY_Create(AML_AVPLAY_ATTR_S* attr, AML_HANDLE* mAVPlay, int zorder));

    FUNC_DEF(AML_AVPLAY_SetStreamType,
            int pfAML_AVPLAY_SetStreamType(AML_HANDLE mAVPlay, AML_AVPLAY_STREAM_TYPE_E stream_type));

    FUNC_DEF(AML_AVPLAY_RegisterEvent64,
            int pfAML_AVPLAY_RegisterEvent64(AML_HANDLE mAVPlay, AML_TVS_EVENT event_func, AML_VOID* para));

    FUNC_DEF(AML_AVPLAY_UnRegisterEvent,
            int pfAML_AVPLAY_UnRegisterEvent(AML_HANDLE mAVPlay, AML_AVPLAY_EVENT_E event));

    FUNC_DEF(AML_AVPLAY_GetAttr,
            int pfAML_AVPLAY_GetAttr(AML_HANDLE mAVPlay, AML_AVPLAY_ATTR_ID_E attr_id, AML_AVPLAY_ATTR_U* attr));

    FUNC_DEF(AML_AVPLAY_SetAttr,
            int pfAML_AVPLAY_SetAttr(AML_HANDLE mAVPlay, AML_AVPLAY_ATTR_ID_E attr_id, AML_AVPLAY_ATTR_U* attr));

    FUNC_DEF(AML_AVPLAY_Pause,
            int pfAML_AVPLAY_Pause(AML_HANDLE mAVPlay, AML_AVPLAY_PAUSE_OPT_S* options));

    FUNC_DEF(AML_AVPLAY_Resume,
            int pfAML_AVPLAY_Resume(AML_HANDLE mAVPlay, AML_AVPLAY_RESUME_OPT_S* options));

    FUNC_DEF(AML_AVPLAY_Start,
            int pfAML_AVPLAY_Start(AML_HANDLE mAVPlay, AML_AVPLAY_START_OPT_S* options));

    FUNC_DEF(AML_AVPLAY_Stop,
            int pfAML_AVPLAY_Stop(AML_HANDLE mAVPlay, AML_AVPLAY_STOP_OPT_S *options));

    FUNC_DEF(AML_AVPLAY_FlushStream,
            int pfAML_AVPLAY_FlushStream(AML_HANDLE mAVPlay, AML_AVPLAY_FLUSH_STREAM_OPT_S *options));

    FUNC_DEF(AML_AVPLAY_Reset,
            int pfAML_AVPLAY_Reset(AML_HANDLE mAVPlay, AML_AVPLAY_RESET_OPT_S *options));

    FUNC_DEF(AML_AVPLAY_Destroy,
            int pfAML_AVPLAY_Destroy(AML_HANDLE mAVPlay, int zorder));

    FUNC_DEF(AML_AVPLAY_SetDecodeMode,
            int pfAML_AVPLAY_SetDecodeMode(AML_HANDLE mAVPlay, AML_AVPLAY_VCODEC_MODE_E vcodec_mode));

    FUNC_DEF(AML_AVPLAY_GetPlaySpeed,
            int pfAML_AVPLAY_GetPlaySpeed(AML_HANDLE mAVPlay, float* speed));

    FUNC_DEF(AML_AVPLAY_Tplay,
            int pfAML_AVPLAY_Tplay(AML_HANDLE mAVPlay, AML_AVPLAY_TPLAY_OPT_S *options));

    FUNC_DEF(AML_AVPLAY_GetStatusInfo,
            int pfAML_AVPLAY_GetStatusInfo(AML_HANDLE mAVPlay, AML_AVPLAY_STATUS_INFO_S *status));

    FUNC_DEF(AML_AVPLAY_SetVideoSize,
            int pfAML_AVPLAY_SetVideoSize(AML_HANDLE mAVPlay, int32_t x, int32_t y, int32_t w, int32_t h));

    FUNC_DEF(AML_PLAYER_Get_SideBandHandle,
            native_handle_t* pfAML_PLAYER_Get_SideBandHandle(AML_HANDLE mAVPlay)); // add for getsideband

    FUNC_DEF(AML_AVPLAY_SetSurface,
            int pfAML_AVPLAY_SetSurface(AML_HANDLE mAVPlay, ANativeWindow* surface));

    FUNC_DEF(AML_AVPLAY_SetVideoTunnelId,
            int pfAML_AVPLAY_SetVideoTunnelId(AML_HANDLE mAVPlay, int videoTunnelId));

    FUNC_DEF(AML_AVPLAY_SetNetworkJitter,
            int pfAML_AVPLAY_SetNetworkJitter(AML_HANDLE mAVPlay, AML_HANDLE demux, int delayMs));

    FUNC_DEF(AML_AVPLAY_SetAudioOutputType,
            int pfAML_AVPLAY_SetAudioOutputType(AML_HANDLE mAVPlay, AUDIO_OUTPUT_TYPE audioOutputType));

    FUNC_DEF(AML_AVPLAY_SetAudioPtsOffset,
            int pfAML_AVPLAY_SetAudioPtsOffset(AML_HANDLE mAVPlay, int ptsOffset));

    FUNC_DEF(AML_AVPLAY_SetVideoPtsOffset,
            int pfAML_AVPLAY_SetVideoPtsOffset(AML_HANDLE mAVPlay, int ptsOffset));

    FUNC_DEF(AML_AVPLAY_SetScreenMode,
            int pfAML_AVPLAY_SetScreenMode(AML_HANDLE mAVPlay, int screenMode));

    FUNC_DEF(AML_AVPLAY_PushEsBuffer,
            int pfAML_AVPLAY_PushEsBuffer(AML_HANDLE mAVPlay, AML_PLAYER_MEDIA_CHAN_E chn, const uint8_t* data, size_t nSize, int64_t us));

    FUNC_DEF(AML_AVPLAY_PushEsBuffer90K,
            int pfAML_AVPLAY_PushEsBuffer90K(AML_HANDLE mAVPlay, AML_PLAYER_MEDIA_CHAN_E chn, const uint8_t* data, size_t nSize, int64_t pts));

    FUNC_DEF(AML_AVPLAY_SetVolume,
            int pfAML_AVPLAY_SetVolume(AML_HANDLE mAVPlay, float volume));

    FUNC_DEF(AML_PLAYER_ChnOpen,
            int pfAML_PLAYER_ChnOpen(AML_HANDLE mAVPlay, AML_PLAYER_MEDIA_CHAN_E change_type, AML_PLAYER_OPEN_OPT_U *options));

    FUNC_DEF(AML_PLAYER_ChnClose,
            int pfAML_PLAYER_ChnClose(AML_HANDLE mAVPlay, AML_PLAYER_MEDIA_CHAN_E change_type, AML_PLAYER_CLOSE_OPT_U* options));

    FUNC_DEF(AML_PLAYER_GetAttr,
            int pfAML_PLAYER_GetAttr(AML_HANDLE mAVPlay, AML_PLAYER_ATTR_ID_E attr, AML_PLAYER_ATTR_U *options));

    FUNC_DEF(AML_PLAYER_SetAttr,
            int pfAML_PLAYER_SetAttr(AML_HANDLE mAVPlay, AML_PLAYER_ATTR_ID_E attr, AML_PLAYER_ATTR_U *options));

    FUNC_DEF(AML_PLAYER_Start,
            int pfAML_PLAYER_Start(AML_HANDLE mAVPlay, AML_PLAYER_MEDIA_CHAN_E change_type, AML_PLAYER_START_OPT_U *options));

    FUNC_DEF(AML_PLAYER_Stop,
            int pfAML_PLAYER_Stop(AML_HANDLE mAVPlay, AML_PLAYER_MEDIA_CHAN_E change_type, AML_PLAYER_STOP_OPT_U *options));

    FUNC_DEF(AML_PLAYER_GetConfig,
            int pfAML_PLAYER_GetConfig(AML_HANDLE mAVPlay, AML_PLAYER_MEDIA_CHAN_E change_type, AML_PLAYER_CONFIG_E config_type, AML_PLAYER_CONFIG_OPT_U* options));

    FUNC_DEF(AML_PLAYER_SetConfig,
            int pfAML_PLAYER_SetConfig(AML_HANDLE mAVPlay, AML_PLAYER_MEDIA_CHAN_E change_type, AML_PLAYER_CONFIG_E config_type, AML_PLAYER_CONFIG_OPT_U* options));

    FUNC_DEF(AML_PLAYER_GetCCEsData,
            int pfAML_PLAYER_GetCCEsData(AML_HANDLE mAVPlay, uint8_t *data, int nSize));

    FUNC_DEF(AML_DMX_OpenDemux,
            AML_HANDLE pfAML_DMX_OpenDemux(int zorder));

    FUNC_DEF(AML_DMX_CloseDemux,
            int pfAML_DMX_CloseDemux(AML_HANDLE demux));

    FUNC_DEF(AML_DMX_StartDemux,
            int pfAML_DMX_StartDemux(AML_HANDLE demux));

    FUNC_DEF(AML_DMX_StopDemux,
            int pfAML_DMX_StopDemux(AML_HANDLE demux));

    FUNC_DEF(AML_DMX_FlushDemux,
            int pfAML_DMX_FlushDemux(AML_HANDLE demux));

    FUNC_DEF(AML_DMX_CreateChannel,
            AML_HANDLE pfAML_DMX_CreateChannel(AML_HANDLE demux, int pid));

    FUNC_DEF(AML_DMX_DestroyChannel,
            int pfAML_DMX_DestroyChannel(AML_HANDLE demux, AML_HANDLE channel));

    FUNC_DEF(AML_DMX_OpenChannel,
            int pfAML_DMX_OpenChannel(AML_HANDLE demux, AML_HANDLE channel));

    FUNC_DEF(AML_DMX_CloseChannel,
            int pfAML_DMX_CloseChannel(AML_HANDLE demux, AML_HANDLE channel));

    FUNC_DEF(AML_DMX_CreateFilter,
            AML_HANDLE pfAML_DMX_CreateFilter(AML_HANDLE demux, AML_SECTION_FILTER_CB cb, AML_VOID* pUserData, int id));

    FUNC_DEF(AML_DMX_DestroyFilter,
            int pfAML_DMX_DestroyFilter(AML_HANDLE demux, AML_HANDLE filter));

    FUNC_DEF(AML_DMX_AttachFilter,
            int pfAML_DMX_AttachFilter(AML_HANDLE demux, AML_HANDLE filter, AML_HANDLE channel));

    FUNC_DEF(AML_DMX_DetachFilter,
            int pfAML_DMX_DetachFilter(AML_HANDLE demux, AML_HANDLE filter, AML_HANDLE channel));

    FUNC_DEF(AML_DMX_PushTsBuffer,
            int pfAML_DMX_PushTsBuffer(AML_HANDLE mAVPlay, AML_HANDLE demux, const uint8_t *data, size_t nSize));

    FUNC_DEF(AML_AUDIOSYSTEM_GetParameters,
            int pfAML_AUDIOSYSTEM_GetParameters(const char *keys));

    FUNC_DEF(AML_AUDIOSYSTEM_SetParameters,
            int pfAML_AUDIOSYSTEM_SetParameters(const char *keyValuePairs));

private:
    void* mHalPlayerHandle = nullptr;
};

AmlHalPlayer::AmlHalPlayer()
{
    MLOG();

    const char* libName = "libliveplayer.so";

    mHalPlayerHandle = dlopen(libName, RTLD_NOW);
    if (mHalPlayerHandle == nullptr) {
        MLOGE("dlopen %s failed!", libName);
    }
}

AmlHalPlayer::~AmlHalPlayer()
{
    if (mHalPlayerHandle != nullptr) {
        ::dlclose(mHalPlayerHandle);
        mHalPlayerHandle = nullptr;
    }

    MLOG();
}

////////////////////////////////////////////////////////////////////////////////
AMLOGIC_VIDEO_CODEC_TYPE convertToAmlVideoCodecType(Aml_MP_CodecID codecId)
{
    AMLOGIC_VIDEO_CODEC_TYPE amlVCodec = AMLOGIC_VCODEC_TYPE_UNKNOWN;

    switch (codecId) {
    case AML_MP_VIDEO_CODEC_MPEG12:
        amlVCodec = AMLOGIC_VCODEC_TYPE_MPEG2;
        break;

    case AML_MP_VIDEO_CODEC_MPEG4:
        amlVCodec = AMLOGIC_VCODEC_TYPE_MPEG4;
        break;

    case AML_MP_VIDEO_CODEC_H264:
        amlVCodec = AMLOGIC_VCODEC_TYPE_H264;
        break;

    case AML_MP_VIDEO_CODEC_HEVC:
        amlVCodec = AMLOGIC_VCODEC_TYPE_HEVC;
        break;

    case AML_MP_VIDEO_CODEC_VC1:
        amlVCodec = AMLOGIC_VCODEC_TYPE_VC1;
        break;

    case AML_MP_VIDEO_CODEC_AVS:
        amlVCodec = AMLOGIC_VCODEC_TYPE_AVS;
        break;

    case AML_MP_VIDEO_CODEC_AVS2:
        amlVCodec = AMLOGIC_VCODEC_TYPE_AVS2;
        break;

    default:
        MLOGE("unsupported codec id:%s", mpCodecId2Str(codecId));
        break;
    }

    return amlVCodec;
}

AMLOGIC_AUDIO_CODEC_TYPE convertToAmlAudioCodecType(Aml_MP_CodecID codecId)
{
    AMLOGIC_AUDIO_CODEC_TYPE amlACodec = AMLOGIC_ACODEC_TYPE_UNKNOWN;

    switch (codecId) {
    case AML_MP_AUDIO_CODEC_MP2:
    case AML_MP_AUDIO_CODEC_MP3:
        amlACodec = AMLOGIC_ACODEC_TYPE_MP2;
        break;

    case AML_MP_AUDIO_CODEC_AAC:
        amlACodec = AMLOGIC_ACODEC_TYPE_AAC;
        break;

    case AML_MP_AUDIO_CODEC_PCM:
        amlACodec = AMLOGIC_ACODEC_TYPE_ADPCM;
        break;

    case AML_MP_AUDIO_CODEC_AC3:
        amlACodec = AMLOGIC_ACODEC_TYPE_DOLBY_AC3;
        break;

    case AML_MP_AUDIO_CODEC_EAC3:
        amlACodec = AMLOGIC_ACODEC_TYPE_DOLBY_AC3PLUS;
        break;

    case AML_MP_AUDIO_CODEC_DTS:
        amlACodec = AMLOGIC_ACODEC_TYPE_DTSHD;
        break;

    default:
        MLOGE("unsupported codec id:%s", mpCodecId2Str(codecId));
        break;
    }

    return amlACodec;
}

AML_PLAYER_MEDIA_CHAN_E convertToAmlMediaChannelType(Aml_MP_StreamType streamType)
{
    AML_PLAYER_MEDIA_CHAN_E amlMediaChannelType = AML_PLAYER_MEDIA_CHAN_NONE;

    switch (streamType) {
    case AML_MP_STREAM_TYPE_VIDEO:
        amlMediaChannelType = AML_PLAYER_MEDIA_CHAN_VID;
        break;

    case AML_MP_STREAM_TYPE_AUDIO:
        amlMediaChannelType = AML_PLAYER_MEDIA_CHAN_AUD;
        break;

    case AML_MP_STREAM_TYPE_SUBTITLE:
        amlMediaChannelType = AML_PLAYER_MEDIA_CHAN_SUB;
        break;

    default:
        break;
    }

    return amlMediaChannelType;
}

////////////////////////////////////////////////////////////////////////////////
AmlCTCPlayer::AmlCTCPlayer(Aml_MP_PlayerCreateParams* createParams, int instanceId)
: aml_mp::AmlPlayerBase(createParams, instanceId)
{
    snprintf(mName, sizeof(mName), "%s_%d", LOG_TAG, instanceId);

    MLOGI("%s:%d instanceId=%d\n", __FUNCTION__, __LINE__, instanceId);

    mVideoParaSeted = false;
    mAudioParaSeted = false;
    memset(&mVideoFormat, 0, sizeof(mVideoFormat));
    mVideoFormat.u32Pid = AML_MP_INVALID_PID;
    memset(&mAudioFormat, 0, sizeof(mAudioFormat));
    mAudioFormat.u32CurPid = AML_MP_INVALID_PID;

    mZorder = instanceId;

    mHalPlayer = new AmlHalPlayer();
    if (mHalPlayer->initCheck()) {
        const char* versionString;
        mHalPlayer->getVersion(&versionString);
        MLOGI("%s", versionString);
    }
}

int AmlCTCPlayer::initCheck() const
{
    if (mHalPlayer && mHalPlayer->initCheck()) {
        return 0;
    }

    return -1;
}

AmlCTCPlayer::~AmlCTCPlayer()
{
    MLOGI("%s:%d\n", __FUNCTION__, __LINE__);

    stop();
}

int AmlCTCPlayer::setANativeWindow(ANativeWindow* pSurface) {
    RETURN_IF(-1, pSurface == nullptr);

    MLOGI("%s:%d ANativeWindow: %p\n", __FUNCTION__, __LINE__, pSurface);

    mSurface = pSurface;

    return 0;
}

int AmlCTCPlayer::setVideoParams(const Aml_MP_VideoParams* params) {
    RETURN_IF(-1, params == nullptr);

    MLOGI("%s:%d videoCodec: %s vpid: 0x%x width: %d height: %d\n", __FUNCTION__, __LINE__,
        mpCodecId2Str(params->videoCodec), params->pid, params->width, params->height);
    if (params->pid == AML_MP_INVALID_PID) {
        MLOGI("Video pid invalid, return -1\n");
        return -1;
    }

    mVideoFormat.enType = convertToAmlVideoCodecType(params->videoCodec);
    mVideoFormat.u32Pid = params->pid;
    mVideoFormat.zorder = mZorder;

    mVideoParaSeted = true;

    return 0;
}

int AmlCTCPlayer::setAudioParams(const Aml_MP_AudioParams* params) {
    RETURN_IF(-1, params == nullptr);

    MLOGI("%s:%d audioCodec: %s apid: 0x%x ch: %d samp: %d\n", __FUNCTION__, __LINE__,
        mpCodecId2Str(params->audioCodec), params->pid, params->nChannels, params->nSampleRate);
    if (params->pid == AML_MP_INVALID_PID) {
        MLOGI("Audio pid invalid, return -1\n");
        return -1;
    }

    mAudioFormat.enType = convertToAmlAudioCodecType(params->audioCodec);
    mAudioFormat.u32CurPid = params->pid;

    mAudioParaSeted = true;

    return 0;
}

int AmlCTCPlayer::start() {
    int ret = 0;
    MLOGI("%s:%d\n", __FUNCTION__, __LINE__);


    if (mAvSyncMode == AML_UNF_SYNC_REF_PCR) {
        MLOGI("pcr sync mode, create demux");
        if (mDemux == nullptr) {
            mDemux = mHalPlayer->openDemux(mZorder);
        }
    }

    //init player
    if (mPlayer == nullptr) {
        mHalPlayer->create(nullptr, &mPlayer, mZorder);
    }

    mHalPlayer->setStreamType(mPlayer, mStreamType);

    //set stc
    AML_AVPLAY_ATTR_U attr;
    attr.syncAttr.mDemux = mDemux;
    attr.syncAttr.pcrPid = mPcrPid;
    attr.syncAttr.enSyncRef = mAvSyncMode;
    mHalPlayer->setAttr(mPlayer, AML_AVPLAY_ATTR_ID_SYNC, &attr);
    MLOGI("mPcrPid:%d", mPcrPid);

    mHalPlayer->setAudioOutputType(mPlayer, mAudioOutputType);

    mHalPlayer->setNetworkJitter(mPlayer, mDemux, 300);

    if (mVideoTunnelId >= 0) {
        mHalPlayer->setVideoTunnelId(mPlayer, mVideoTunnelId);
    } else {
        mHalPlayer->setSurface(mPlayer, mSurface);
    }

    mHalPlayer->registerEvent64(mPlayer, [](void* handler, AML_AVPLAY_EVENT_E event, unsigned long param1, int param2) {
        AmlCTCPlayer* player = (AmlCTCPlayer*)handler;
        player->eventCb(event, param1, param2);
    }, this);

    mHalPlayer->setVolume(mPlayer, mVolume);

    AML_PLAYER_ATTR_U* pAttr = nullptr;

    AML_PLAYER_OPEN_OPT_U openOptions;

    //init video channel
    if (mVideoParaSeted) {
        openOptions.videoOptions.enMode = mRequestKeepLastFrame ? AML_PLAYER_LAST_FRAME_MODE_STILL : AML_PLAYER_LAST_FRAME_MODE_BLACK;
        openOptions.videoOptions.screenMode = AML_PLAYER_SCREEN_MODE_STRETCH;
        mHalPlayer->chnOpen(mPlayer, AML_PLAYER_MEDIA_CHAN_VID, &openOptions);

        pAttr = (AML_PLAYER_ATTR_U*)&mVideoFormat;
        mHalPlayer->setAttr(mPlayer, AML_PLAYER_ATTR_ID_VDEC, pAttr);

        int videoPid = mVideoFormat.u32Pid;
        pAttr = (AML_PLAYER_ATTR_U*)&videoPid;
        mHalPlayer->setAttr(mPlayer, AML_PLAYER_ATTR_ID_VID_PID, pAttr);
    } else {
        MLOGI("Video para not seted");
    }

    //init audio channel
    if (mAudioParaSeted) {
        openOptions.audioOptions.audioOnly = !mVideoParaSeted;
        mHalPlayer->chnOpen(mPlayer, AML_PLAYER_MEDIA_CHAN_AUD, &openOptions);

        pAttr = (AML_PLAYER_ATTR_U*)&mAudioFormat;
        mHalPlayer->setAttr(mPlayer, AML_PLAYER_ATTR_ID_AUD_PID, pAttr);

    } else {
        MLOGI("Audio para not seted");
    }

    //start demux
    if (mDemux != nullptr) {
        mHalPlayer->startDemux(mDemux);
    }

    //start video channel
    AML_PLAYER_START_OPT_U startOptions;
    mHalPlayer->start(mPlayer, AML_PLAYER_MEDIA_CHAN_VID, &startOptions);

    //start audio channel
    mHalPlayer->start(mPlayer, AML_PLAYER_MEDIA_CHAN_AUD, &startOptions);

    //start player
    ret = mHalPlayer->start(mPlayer, nullptr);

    //AmlPlayerBase::start();

    return ret;
}

int AmlCTCPlayer::stop() {
    int ret = 0;
    MLOGI("%s:%d\n", __FUNCTION__, __LINE__);
    //AmlPlayerBase::stop();

#if 0
    //stop video channel
    AML_PLAYER_STOP_OPT_U videoStopOption;
    videoStopOption.videoOptions.enMode = mRequestKeepLastFrame ? AML_PLAYER_LAST_FRAME_MODE_STILL : AML_PLAYER_LAST_FRAME_MODE_BLACK;
    mHalPlayer->stop(mPlayer, AML_PLAYER_MEDIA_CHAN_VID, &videoStopOption);

    //stop audio channel
    AML_PLAYER_STOP_OPT_U stopOptions{};
    mHalPlayer->stop(mPlayer, AML_PLAYER_MEDIA_CHAN_AUD, &stopOptions);
#endif

    //close video channel
    AML_PLAYER_CLOSE_OPT_U closeOptions;
    closeOptions.videoOptions.enMode = mRequestKeepLastFrame ? AML_PLAYER_LAST_FRAME_MODE_STILL : AML_PLAYER_LAST_FRAME_MODE_BLACK;
    mHalPlayer->chnClose(mPlayer, AML_PLAYER_MEDIA_CHAN_VID, &closeOptions);

    //close audio channel
    mHalPlayer->chnClose(mPlayer, AML_PLAYER_MEDIA_CHAN_AUD, nullptr);

    //stop demux
    if (mDemux != nullptr) {
        mHalPlayer->stopDemux(mDemux);
    }

    //stop player
    if (mPlayer != nullptr) {
        ret = mHalPlayer->stop(mPlayer, nullptr);
    }

    //close demux
    if (mDemux != nullptr) {
        mHalPlayer->closeDemux(mDemux);
        mDemux = nullptr;
    }

    //destroy player
    if (mPlayer != nullptr) {
        mHalPlayer->destroy(mPlayer, mZorder);
        mPlayer = nullptr;
    }

    return ret;
}

int AmlCTCPlayer::pause() {

    MLOGI("%s:%d\n", __FUNCTION__, __LINE__);

    AML_AVPLAY_PAUSE_OPT_S options;
    options.u32Reserved = 0;

    int ret = mHalPlayer->pause(mPlayer, &options);

    return ret;
}

int AmlCTCPlayer::resume() {
    MLOGI("%s:%d\n", __FUNCTION__, __LINE__);

    int ret = mHalPlayer->resume(mPlayer, nullptr);

    return ret;
}

int AmlCTCPlayer::flush() {
    MLOG();

    int ret = mHalPlayer->flushStream(mPlayer, nullptr);

    //AmlPlayerBase::flush();
    return ret;
}

int AmlCTCPlayer::setPlaybackRate(float rate) {
    MLOGI("%s:%d rate=%f\n", __FUNCTION__, __LINE__, rate);

    AML_AVPLAY_TPLAY_OPT_S options;
    options.enTplayDirect = AML_AVPLAY_TPLAY_DIRECT_NONE;
    options.u32SpeedInteger = rate;
    options.u32SpeedDecimal = (uint32_t)rate * 1000 % 1000;

    int ret = mHalPlayer->tplay(mPlayer, &options);

    return ret;
}

int AmlCTCPlayer::getPlaybackRate(float* rate) {
    int ret = mHalPlayer->getPlaySpeed(mPlayer, rate);

    return ret;
}

int AmlCTCPlayer::switchAudioTrack(const Aml_MP_AudioParams* params){
    RETURN_IF(-1, params == nullptr);

    return AML_MP_ERROR_DEAD_OBJECT;
}

int AmlCTCPlayer::writeData(const uint8_t* buffer, size_t size) {
    int ret = 0;

    ret = mHalPlayer->pushTsBuffer(mPlayer, mDemux, buffer, size);

    return ret;
}

int AmlCTCPlayer::writeEsData(Aml_MP_StreamType type, const uint8_t* buffer, size_t size, int64_t pts)
{
    AML_PLAYER_MEDIA_CHAN_E amlMediaChannelType = convertToAmlMediaChannelType(type);

    int ret = mHalPlayer->pushEsBuffer90K(mPlayer, amlMediaChannelType, buffer, size, pts);

    return ret;
}

int AmlCTCPlayer::getCurrentPts(Aml_MP_StreamType type, int64_t* pts) {
    AML_AVPLAY_STATUS_INFO_S status{};

    int ret = mHalPlayer->getStatusInfo(mPlayer, &status);
    if (ret < 0) {
        return ret;
    }

    int64_t currentPts = INT64_MIN;
    if (type == AML_MP_STREAM_TYPE_VIDEO) {
        currentPts = status.stSyncStatus.u32LastVidPts;
    } else if (type == AML_MP_STREAM_TYPE_AUDIO) {
        currentPts = status.stSyncStatus.u32LastAudPts;
    }

    *pts = currentPts;

    return 0;
}

int AmlCTCPlayer::getFirstPts(Aml_MP_StreamType type, int64_t* pts) {
    AML_MP_UNUSED(type);
    AML_MP_UNUSED(pts);

    MLOGI("TODO: AmlCTCPlayer::getFirstPts");

    return 0;
}

int AmlCTCPlayer::getBufferStat(Aml_MP_BufferStat* bufferStat) {

    MLOGI("%s:%d\n", __FUNCTION__, __LINE__);

    AML_AVPLAY_STATUS_INFO_S status{};

    int ret = mHalPlayer->getStatusInfo(mPlayer, &status);
    if (AML_SUCCESS == ret) {
        bufferStat->videoBuffer.dataLen = status.videoBufferStatus.usedSize;
        bufferStat->videoBuffer.size = status.videoBufferStatus.totalSize;
        bufferStat->videoBuffer.bufferedMs = status.videoBufferStatus.bufferedUs / 1000;

        bufferStat->audioBuffer.dataLen = status.audioBufferStatus.usedSize;
        bufferStat->audioBuffer.size = status.audioBufferStatus.totalSize;
        bufferStat->audioBuffer.bufferedMs = status.audioBufferStatus.bufferedUs / 1000;
    }

    return ret;
}

int AmlCTCPlayer::setVideoWindow(int x, int y, int width, int height) {

    MLOGI("%s:%d x: %d y: %d width: %d height: %d\n", __FUNCTION__, __LINE__, x, y, width, height);

    return mHalPlayer->setVideoSize(mPlayer, x, y, width, height);
}

int AmlCTCPlayer::setVolume(float volume) {
    MLOGI("%s:%d vol=%f\n", __FUNCTION__, __LINE__, volume);

    mVolume = volume;

    int ret = 0;
    if (mPlayer != nullptr) {
        ret = mHalPlayer->setVolume(mPlayer, volume);
    }

    return ret;
}

int AmlCTCPlayer::getVolume(float* volume) {

    *volume = mVolume;

    return 0;
}

int AmlCTCPlayer::showVideo() {

    MLOGI("%s:%d\n", __FUNCTION__, __LINE__);

    return 0;
}

int AmlCTCPlayer::hideVideo() {
    MLOGI("%s:%d\n", __FUNCTION__, __LINE__);

    return 0;
}

int AmlCTCPlayer::setParameter(Aml_MP_PlayerParameterKey key, void* parameter) {
    MLOGI("%s:%d %s\n", __FUNCTION__, __LINE__, mpPlayerParameterKey2Str(key));

    int ret = 0;

    switch (key) {
    case AML_MP_PLAYER_PARAMETER_VIDEO_DISPLAY_MODE:
    {
        Aml_MP_VideoDisplayMode displayMode = *(Aml_MP_VideoDisplayMode*)parameter;
        ret = mHalPlayer->setScreenMode(mPlayer, displayMode);
        break;
    }

    case AML_MP_PLAYER_PARAMETER_BLACK_OUT:
    {
        mRequestKeepLastFrame = !*(bool*)parameter;
        MLOGI("mRequestKeepLastFrame:%d", mRequestKeepLastFrame);
        break;
    }

    case AML_MP_PLAYER_PARAMETER_VIDEO_DECODE_MODE:
    {
        break;
    }

    case AML_MP_PLAYER_PARAMETER_VIDEO_PTS_OFFSET:
    {
        ret = mHalPlayer->setVideoPtsOffset(mPlayer, *(int*)parameter);
        break;
    }

    case AML_MP_PLAYER_PARAMETER_AUDIO_PTS_OFFSET:
    {
        ret = mHalPlayer->setAudioPtsOffset(mPlayer, *(int*)parameter);
        break;
    }

    case AML_MP_PLAYER_PARAMETER_AUDIO_OUTPUT_DEVICE:
    {
        Aml_MP_AudioOutputDevice device = *(Aml_MP_AudioOutputDevice*)parameter;
        if (device == AML_MP_AUDIO_OUTPUT_DEVICE_BT) {
            mAudioOutputType = AUDIO_OUTPUT_TYPE::BT;
        } else {
            mAudioOutputType = AUDIO_OUTPUT_TYPE::NORMAL;
        }

        ret = mHalPlayer->setAudioOutputType(mPlayer, mAudioOutputType);
        break;
    }

    case AML_MP_PLAYER_PARAMETER_AUDIO_MUTE:
    {
        AML_PLAYER_CONFIG_OPT_U configOptions{};
        configOptions.configAudioMute.mute = *(bool*)parameter;
        ret = mHalPlayer->setConfig(mPlayer, AML_PLAYER_MEDIA_CHAN_AUD, AML_PLAYER_CONFIG_AUDIO_MUTE, &configOptions);
        break;
    }

    case AML_MP_PLAYER_PARAMETER_VIDEO_TUNNEL_ID:
    {
        mVideoTunnelId = *(int*)parameter;
        MLOGI("mVideoTunnelId:%d", mVideoTunnelId);
        break;
    }

    //TODO: set EOS

    default:
        MLOGI("Not support parameter key: %s", mpPlayerParameterKey2Str(key));
        break;
    }

    return ret;
}

int AmlCTCPlayer::getParameter(Aml_MP_PlayerParameterKey key, void* parameter) {
    MLOGI("%s:%d\n", __FUNCTION__, __LINE__);
    MLOGD("Not support parameter key: %s", mpPlayerParameterKey2Str(key));
    AML_MP_UNUSED(key);
    AML_MP_UNUSED(parameter);
    return 0;
}

int AmlCTCPlayer::setAVSyncSource(Aml_MP_AVSyncSource syncSource) {
    MLOGI("%s:%d syncmode:%s\n", __FUNCTION__, __LINE__, mpAVSyncSource2Str(syncSource));

    if (syncSource == AML_MP_AVSYNC_SOURCE_PCR) {
        mAvSyncMode = AML_UNF_SYNC_REF_PCR;
    } else if (syncSource == AML_MP_AVSYNC_SOURCE_AUDIO) {
        mAvSyncMode = AML_UNF_SYNC_REF_AUDIO;
    } else if (syncSource == AML_MP_AVSYNC_SOURCE_VIDEO) {
        mAvSyncMode = AML_UNF_SYNC_REF_VIDEO;
    }

    return 0;
}

int AmlCTCPlayer::setPcrPid(int pid) {

    MLOGI("%s:%d\n", __FUNCTION__, __LINE__);
    mPcrPid = pid;

    return 0;
}

int AmlCTCPlayer::startVideoDecoding() {
    MLOGI("%s:%d\n", __FUNCTION__, __LINE__);

    int ret = mHalPlayer->start(mPlayer, AML_PLAYER_MEDIA_CHAN_VID, nullptr);

    return ret;
}

int AmlCTCPlayer::stopVideoDecoding() {
    MLOGI("%s:%d\n", __FUNCTION__, __LINE__);

    AML_PLAYER_STOP_OPT_U stopOptions{};
    stopOptions.videoOptions.enMode = mRequestKeepLastFrame ? AML_PLAYER_LAST_FRAME_MODE_STILL : AML_PLAYER_LAST_FRAME_MODE_BLACK;
    int ret = mHalPlayer->stop(mPlayer, AML_PLAYER_MEDIA_CHAN_VID, &stopOptions);

    return ret;
}

int AmlCTCPlayer::pauseVideoDecoding() {
    MLOGI("%s:%d\n", __FUNCTION__, __LINE__);

    return 0;
}

int AmlCTCPlayer::resumeVideoDecoding() {
    MLOGI("%s:%d\n", __FUNCTION__, __LINE__);

    return 0;
}

int AmlCTCPlayer::startAudioDecoding() {

    MLOGI("%s:%d\n", __FUNCTION__, __LINE__);

    int ret = mHalPlayer->start(mPlayer, AML_PLAYER_MEDIA_CHAN_AUD, nullptr);

    return ret;
}

int AmlCTCPlayer::stopAudioDecoding() {
    MLOGI("%s:%d\n", __FUNCTION__, __LINE__);

    int ret = mHalPlayer->stop(mPlayer, AML_PLAYER_MEDIA_CHAN_AUD, nullptr);

    return ret;
}

int AmlCTCPlayer::pauseAudioDecoding() {
    MLOGI("%s:%d\n", __FUNCTION__, __LINE__);

    return 0;
}

int AmlCTCPlayer::resumeAudioDecoding() {
    MLOGI("%s:%d\n", __FUNCTION__, __LINE__);

    return 0;
}

int AmlCTCPlayer::setADParams(const Aml_MP_AudioParams* params, bool enableMix) {
    AML_MP_UNUSED(params);
    AML_MP_UNUSED(enableMix);
    MLOGI("%s:%d\n", __FUNCTION__, __LINE__);
    return -1;
}

int AmlCTCPlayer::setADVolume(float volume) {
    AML_MP_UNUSED(volume);
    MLOGI("%s:%d\n", __FUNCTION__, __LINE__);
    return -1;
}

int AmlCTCPlayer::getADVolume(float* volume) {
    AML_MP_UNUSED(volume);
    MLOGI("%s:%d\n", __FUNCTION__, __LINE__);
    return -1;
}

void AmlCTCPlayer::eventCb(AML_AVPLAY_EVENT_E event, unsigned long param1, int param2)
{
    AML_MP_UNUSED(param1);
    AML_MP_UNUSED(param2);

    Aml_MP_PlayerEventType mpEvent = AML_MP_EVENT_UNKNOWN;
    int64_t param = 0;

    switch (event) {
    case AML_AVPLAY_EVENT_E::EVENT_FIRST_IFRAME_DISPLAYED:
        mpEvent = AML_MP_PLAYER_EVENT_FIRST_FRAME;
        break;

    case AML_AVPLAY_EVENT_E::EVENT_AUDIO_FIRST_DECODED:
        mpEvent = AML_MP_PLAYER_EVENT_AUDIO_DECODE_FIRST_FRAME;
        break;

    case AML_AVPLAY_EVENT_E::EVENT_PLAY_COMPLETED:
        break;

    default:
        break;
    }

    if (mpEvent != AML_MP_EVENT_UNKNOWN) {
        notifyListener(mpEvent, param);
    }
}

}
