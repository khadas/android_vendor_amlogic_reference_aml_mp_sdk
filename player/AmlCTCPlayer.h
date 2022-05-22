/*
 * Copyright (c) 2020 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#ifndef _AML_CTC_PLAYER_H_
#define _AML_CTC_PLAYER_H_

#include "AmlPlayerBase.h"
//#include <CTC_MediaProcessor.h>

struct ANativeWindow;

namespace aml_mp {
typedef void * AML_HANDLE;
typedef unsigned int AML_U32;
typedef int AML_S32;
typedef void AML_VOID;
typedef int32_t AML_HADECODE_OPENPARAM_S;
typedef uint8_t AML_DMX_DATA_S;

#define AML_SUCCESS     (0)
#define AML_FAILURE     (-1)
#define AML_TRUE        (1)
#define AML_FALSE       (0)
#define AUDIO_MAX_TRACKS (10) //keep sync with CTC

////////////////////////////////////////////////////////////////////////////////
typedef enum AML_AVPLAY_EVENT_E {
    EVENT_VIDEO_STREAM_INFO_CHANGED,
    EVENT_AUDIO_STREAM_INFO_CHANGED,

    EVENT_FIRST_IFRAME_DISPLAYED,
    EVENT_FRAME_DISPLAYED,  //  event->data is HalPtsInfo

    EVENT_VIDEO_END_OF_STREAM,
    EVENT_AUDIO_END_OF_STREAM,

    EVENT_SUBTITLE_DATA,

    EVENT_DECODE_ERROR,

    // used in SKB
    EVENT_AUDIO_RECONFIG,
    //AML_AVPLAY_EVENT_EOS,

    // added in SKB
    EVENT_AUDIO_PCM_DATA,
    EVENT_AV_SYNC_LOCKED,
    EVENT_AUDIO_FIRST_DECODED,
    EVENT_USERDATA_READY,
    EVENT_PLAY_COMPLETED,

    EVENT_BAD_DECODING,
    EVENT_BUFFER_FULL,

} AML_AVPLAY_EVENT_E;


typedef void (*AML_TVS_EVENT)(void* handler, AML_AVPLAY_EVENT_E evt, unsigned long param1, int param2);

typedef int (*AML_SECTION_FILTER_CB)(unsigned int u32AcquiredNum, unsigned char* pstBuf, void* pUserData);

typedef enum
{
    NORMAL          = 0,
    BT              = 1,
    AUDIOMIRROR     = 2,
    REMOTE_SUBMIX   = 3,
    UNKNOWN         = 4,
} AUDIO_OUTPUT_TYPE;

/////////////////////////////////////////////////////////////
// copy from AmlCommon.h
typedef enum AMLOGIC_VIDEO_CODEC_TYPE_e {
    AMLOGIC_VCODEC_TYPE_UNKNOWN = 0,
    AMLOGIC_VCODEC_TYPE_MPEG2 = 1,
    AMLOGIC_VCODEC_TYPE_MPEG4,
    AMLOGIC_VCODEC_TYPE_H263,
    AMLOGIC_VCODEC_TYPE_H264,
    AMLOGIC_VCODEC_TYPE_HEVC,
    AMLOGIC_VCODEC_TYPE_VC1,
    AMLOGIC_VCODEC_TYPE_AVS,
    AMLOGIC_VCODEC_TYPE_AVS2,
} AMLOGIC_VIDEO_CODEC_TYPE;

typedef enum AMLOGIC_AUDIO_CODEC_TYPE_e {
    AMLOGIC_ACODEC_TYPE_UNKNOWN = 0,
    AMLOGIC_ACODEC_TYPE_MP2,
    AMLOGIC_ACODEC_TYPE_AAC,        //      0x0F, 0x11
    AMLOGIC_ACODEC_TYPE_ADPCM,      //      AMLOGIC_ACODEC_TYPE_BLYRAYLPCM?
    AMLOGIC_ACODEC_TYPE_DOLBY_AC3, // ??
    AMLOGIC_ACODEC_TYPE_DOLBY_TRUEHD,
    AMLOGIC_ACODEC_TYPE_DOLBY_AC3PLUS,
    AMLOGIC_ACODEC_TYPE_DTSHD,
} AMLOGIC_AUDIO_CODEC_TYPE;

typedef struct{
    uint32_t u32AudBufSize;
    uint32_t u32VidBufSize;
} AML_AVPLAY_BUF_SIZE_S;

typedef struct{
    uint32_t u32DemuxId;
    AML_AVPLAY_BUF_SIZE_S stStreamAttr;
} AML_AVPLAY_ATTR_S;

typedef enum {
    AML_AVPLAY_STREAM_TYPE_NONE = -1,
    AML_AVPLAY_STREAM_TYPE_TS,
    AML_AVPLAY_STREAM_TYPE_ES,
} AML_AVPLAY_STREAM_TYPE_E;

////////////////////////////////////////
//add for audio or video change
typedef enum {
    AML_AVPLAY_ATTR_ID_NONE,
    AML_AVPLAY_ATTR_ID_SYNC,
    AML_AVPLAY_ATTR_ID_PCR_PID,
} AML_AVPLAY_ATTR_ID_E;

//add for av sync
typedef enum {
    AML_UNF_SYNC_REF_NONE,
    AML_UNF_SYNC_REF_AUDIO,
    AML_UNF_SYNC_REF_VIDEO,
    AML_UNF_SYNC_REF_PCR,
} AML_AVPLAY_SYNC_MODE_E;

typedef struct{
    AML_AVPLAY_SYNC_MODE_E enSyncRef;
    AML_HANDLE mDemux;
    AML_U32 pcrPid;
} AML_UNF_SYNC_ATTR_S;

typedef union {
    AML_UNF_SYNC_ATTR_S syncAttr;
    AML_U32 pcrPid;
} AML_AVPLAY_ATTR_U;

////////////////////////////////////////
//add for pause
typedef struct{
    uint32_t u32Reserved;
} AML_AVPLAY_PAUSE_OPT_S;

////////////////////////////////////////
typedef struct{
} AML_AVPLAY_RESUME_OPT_S;

////////////////////////////////////////
typedef struct{
} AML_AVPLAY_START_OPT_S;

////////////////////////////////////////
typedef struct{
} AML_AVPLAY_STOP_OPT_S;

////////////////////////////////////////
typedef struct{
} AML_AVPLAY_FLUSH_STREAM_OPT_S;

////////////////////////////////////////
//add for reset
typedef struct{
    uint32_t u32SeekPtsMs;
} AML_AVPLAY_RESET_OPT_S;

////////////////////////////////////////
// add for fast play
typedef enum {
    AML_AVPLAY_TPLAY_DIRECT_NONE,
    AML_AVPLAY_TPLAY_DIRECT_BACKWARD,
    AML_AVPLAY_TPLAY_DIRECT_FORWARD,
} AML_AVPLAY_TPLAY_DIRECT_E;

typedef struct{
    AML_AVPLAY_TPLAY_DIRECT_E enTplayDirect;
    uint32_t u32SpeedInteger;
    uint32_t u32SpeedDecimal;
} AML_AVPLAY_TPLAY_OPT_S;

////////////////////////////////////////
typedef enum {
    AML_VCODEC_MODE_NONE,
    AML_VCODEC_MODE_I,
} AML_AVPLAY_VCODEC_MODE_E;

////////////////////////////////////////
//add for get av info and avsync info
typedef struct {
    int64_t u32LastAudPts;
    int64_t u32LastVidPts;
} AML_AVPLAY_SYNC_STATUS_S;

typedef struct {
    uint32_t usedSize;
    uint32_t totalSize;
    int64_t  bufferedUs;
} AML_AVPLAY_BUFFER_STATUS_S;

typedef struct {
    AML_AVPLAY_SYNC_STATUS_S stSyncStatus;
    AML_AVPLAY_BUFFER_STATUS_S videoBufferStatus;
    AML_AVPLAY_BUFFER_STATUS_S audioBufferStatus;
} AML_AVPLAY_STATUS_INFO_S;

////////////////////////////////////////
//for amlvideo and amlaudio
typedef enum {
    AML_PLAYER_MEDIA_CHAN_NONE,
    AML_PLAYER_MEDIA_CHAN_VID,
    AML_PLAYER_MEDIA_CHAN_AUD,
    AML_PLAYER_MEDIA_CHAN_SUB,
} AML_PLAYER_MEDIA_CHAN_E;

typedef enum {
    AML_PLAYER_ATTR_ID_NONE,
    AML_PLAYER_ATTR_ID_VDEC,
    AML_PLAYER_ATTR_ID_AUD_PID,
    AML_PLAYER_ATTR_ID_VID_PID,
    AML_PLAYER_ATTR_ID_MULTIAUD,
    AML_PLAYER_ATTR_ID_EOSREACHED,
} AML_PLAYER_ATTR_ID_E;

typedef enum {
    AML_UNF_VCODEC_MODE_NORMAL,
} AML_PLAYER_VCODEC_MODE_E;

typedef struct {
    AMLOGIC_VIDEO_CODEC_TYPE enType;
    AML_PLAYER_VCODEC_MODE_E enMode;
    uint32_t u32ErrCover;
    uint32_t u32Priority;
    uint32_t u32Pid;
    uint32_t zorder;
    uint32_t width;
    uint32_t height;
    void* extraData;
    uint32_t extraDataSize;
} AML_UNF_VCODEC_ATTR_S;

typedef struct {
    AMLOGIC_AUDIO_CODEC_TYPE enType;
    AML_HADECODE_OPENPARAM_S stDecodeParam;
    uint32_t u32CurPid;
    uint16_t zorder;
    uint16_t nChannels;
    uint16_t nSampleRate;
    bool trackChanged;
    bool audioFocusedStart;
    void* extraData;
    uint32_t extraDataSize;
} AML_UNF_ACODEC_ATTR_S;

typedef struct {

} AML_UNF_SUB_ATTR_S;

typedef struct {
    AMLOGIC_AUDIO_CODEC_TYPE enType;
    AML_UNF_ACODEC_ATTR_S pstAcodecAttr[AUDIO_MAX_TRACKS];
    uint32_t u32PidNum;
    uint32_t u32CurPid;
    unsigned int pu32AudPid[AUDIO_MAX_TRACKS];
} AML_PLAY_MULTIAUD_ATTR_S;

typedef union {
    AML_UNF_VCODEC_ATTR_S vcodecAttr;
    AML_UNF_ACODEC_ATTR_S audioAttr;
    AML_U32 videoPid;
    AML_S32 eosReached;
    AML_PLAY_MULTIAUD_ATTR_S multiAudioAttr;
} AML_PLAYER_ATTR_U;

////////////////////////////////////////
typedef enum {
    AML_PLAYER_SCREEN_MODE_FIT,
    AML_PLAYER_SCREEN_MODE_STRETCH,
} AML_PLAYER_SCREEN_MODE_E;

typedef enum {
    AML_PLAYER_LAST_FRAME_MODE_BLACK,
    AML_PLAYER_LAST_FRAME_MODE_STILL,
} AML_PLAYER_LAST_FRAME_MODE_E;

typedef struct {
    AML_PLAYER_LAST_FRAME_MODE_E enMode;
    AML_PLAYER_SCREEN_MODE_E screenMode;
} AML_PLAYER_VIDEO_OPEN_OPT_S;

typedef struct {
    bool audioOnly;
} AML_PLAYER_AUDIO_OPEN_OPT_S;

typedef union {
    AML_PLAYER_VIDEO_OPEN_OPT_S videoOptions;
    AML_PLAYER_AUDIO_OPEN_OPT_S audioOptions;
} AML_PLAYER_OPEN_OPT_U;

////////////////////////////////////////
typedef struct {

} AML_PLAYER_VIDEO_START_OPT_S;

typedef struct {

} AML_PLAYER_AUDIO_START_OPT_S;

typedef union {
    AML_PLAYER_VIDEO_START_OPT_S videoOptions;
    AML_PLAYER_AUDIO_START_OPT_S audioOptions;
} AML_PLAYER_START_OPT_U;

////////////////////////////////////////
typedef struct {
    AML_PLAYER_LAST_FRAME_MODE_E enMode;
    uint32_t u32TimeoutMs;
} AML_PLAYER_VIDEO_STOP_OPT_S;

typedef struct {

} AML_PLAYER_AUDIO_STOP_OPT_S;

typedef union {
    AML_PLAYER_VIDEO_STOP_OPT_S videoOptions;
    AML_PLAYER_AUDIO_STOP_OPT_S audioOptions;
} AML_PLAYER_STOP_OPT_U;

////////////////////////////////////////
typedef struct {
    AML_PLAYER_LAST_FRAME_MODE_E enMode;
} AML_PLAYER_VIDEO_CLOSE_OPT_S;

typedef struct {

} AML_PLAYER_AUDIO_CLOSE_OPT_S;

typedef union {
    AML_PLAYER_VIDEO_CLOSE_OPT_S videoOptions;
    AML_PLAYER_AUDIO_CLOSE_OPT_S audioOptions;
} AML_PLAYER_CLOSE_OPT_U;

////////////////////////////////////////
typedef enum {
    AML_PLAYER_CONFIG_AUDIO_MUTE,
} AML_PLAYER_CONFIG_E;

typedef struct {
    bool mute;
} AML_PLAYER_CONFIG_AUDIO_MUTE_S;

typedef union {
    AML_PLAYER_CONFIG_AUDIO_MUTE_S configAudioMute;
} AML_PLAYER_CONFIG_OPT_U;


class AmlHalPlayer;

class AmlCTCPlayer : public aml_mp::AmlPlayerBase
{
public:
    AmlCTCPlayer(Aml_MP_PlayerCreateParams* createParams, int instanceId);
    ~AmlCTCPlayer();

    int setANativeWindow(ANativeWindow* nativeWindow) override;

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
    int stopVideoDecoding() override;
    int pauseVideoDecoding() override;
    int resumeVideoDecoding() override;

    int startAudioDecoding() override;
    int stopAudioDecoding() override;
    int pauseAudioDecoding() override;
    int resumeAudioDecoding() override;

    int setADParams(const Aml_MP_AudioParams* params, bool enableMix) override;
    int setADVolume(float volume) override;
    int getADVolume(float* volume) override;

private:
    void eventCb(AML_AVPLAY_EVENT_E event, unsigned long param1, int param2);

    char mName[50];
    sptr<AmlHalPlayer> mHalPlayer;

    bool mVideoParaSeted = false;
    bool mAudioParaSeted = false;
    AML_UNF_VCODEC_ATTR_S mVideoFormat;
    AML_UNF_ACODEC_ATTR_S mAudioFormat;

    int mZorder = 0;

    AML_HANDLE mPlayer = nullptr;
    AML_HANDLE mDemux = nullptr;

    bool mRequestKeepLastFrame = true;
    float mVolume = 100.0f;

    AML_AVPLAY_SYNC_MODE_E mAvSyncMode = AML_UNF_SYNC_REF_AUDIO;
    int mPcrPid = AML_MP_INVALID_PID;
    AML_AVPLAY_STREAM_TYPE_E mStreamType = AML_AVPLAY_STREAM_TYPE_TS;

    ANativeWindow* mSurface = nullptr;
    int mVideoTunnelId = -1;

    AUDIO_OUTPUT_TYPE mAudioOutputType = AUDIO_OUTPUT_TYPE::NORMAL;

private:
    AmlCTCPlayer(const AmlCTCPlayer&) = delete;
    AmlCTCPlayer& operator= (const AmlCTCPlayer&) = delete;
};

}

#endif
