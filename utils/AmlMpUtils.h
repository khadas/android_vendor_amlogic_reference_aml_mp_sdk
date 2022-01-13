/*
 * Copyright (c) 2020 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#ifndef _AML_MP_UTILS_H_
#define _AML_MP_UTILS_H_

#include "AmlMpLog.h"
#include <chrono>
#include <amports/vformat.h>
#include <amports/aformat.h>
#include <Aml_MP/Common.h>
#include <string>
#include <vector>
#ifdef HAVE_SUBTITLE
#include <SubtitleNativeAPI.h>
#endif
#include "AmlMpConfig.h"

#ifndef _LINUX_LIST_H
#define _LINUX_LIST_H
struct list_head {
    struct list_head *next, *prev;
};
#endif
#include <dvr_wrapper.h>
#include <dvb_utils.h>

#include <utils/RefBase.h>
#include <sys/syscall.h>
#include <unistd.h>
#ifdef ANDROID
#include <media/stagefright/foundation/ADebug.h>
namespace android {
class NativeHandle;
}
#else
#include <assert.h>
#define CHECK(condition)  assert(condition)
#endif

namespace aml_mp {
#ifndef LOG_ALWAYS_FATAL
  #define LOG_ALWAYS_FATAL(...) \
      ( (void)(android_printAssert(NULL, LOG_TAG, ## __VA_ARGS__)) )
  #endif
#define LITERAL_TO_STRING_INTERNAL(x)    #x
#define LITERAL_TO_STRING(x) LITERAL_TO_STRING_INTERNAL(x)

#define AML_MP_UNUSED(x) (void)(x)

///////////////////////////////////////////////////////////////////////////////
#define RETURN_IF(error, cond)                                                       \
    do {                                                                             \
        if (cond) {                                                                  \
            MLOGE("%s:%d return %s <- %s", __FUNCTION__, __LINE__, #error, #cond);   \
            return error;                                                            \
        }                                                                            \
    } while (0)

#define RETURN_VOID_IF(cond)                                                         \
    do {                                                                             \
        if (cond) {                                                                  \
            MLOGE("%s:%d return <- %s", __FUNCTION__, __LINE__, #cond);              \
            return;                                                                  \
        }                                                                            \
    } while (0)

///////////////////////////////////////////////////////////////////////////////
#define AML_MP_TRACE(thresholdMs) FunctionLogger __flogger##__LINE__(mName, __FILE__, __FUNCTION__, __LINE__, thresholdMs, AmlMpConfig::instance().mLogDebug)

struct FunctionLogger
{
public:
    FunctionLogger(const char * tag, const char * file, const char * func, const int line, int threshold, bool verbose)
    : m_tag(tag), m_file(file), m_func(func), m_line(line), mThreshold(threshold), mVerbose(verbose)
    {
        (void)m_file;
        (void)m_line;

        mBeginTime = std::chrono::steady_clock::now();
        if (mVerbose) {
            ALOG(LOG_DEBUG, "Aml_MP", "%s %s() >> begin", m_tag, m_func);
        }
    }

    ~FunctionLogger()
    {
        auto endTime = std::chrono::steady_clock::now();
        int64_t duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - mBeginTime).count();

        if (mVerbose) {
            ALOG(LOG_DEBUG, "Aml_MP", "%s %s() << end %" PRId64 " ms", m_tag, m_func, duration);
        }

        if(duration >= mThreshold)
            ALOG(LOG_WARN, "Aml_MP", "%s %s() takes %" PRId64 " ms, Seems slow, check!", m_tag, m_func, duration);
    }

private:
    const char * m_tag;
    const char * m_file;
    const char * m_func;
    const int m_line;
    int mThreshold;
    bool mVerbose;
    std::chrono::steady_clock::time_point mBeginTime;
};

const char codecMap[][20][30] = {
    //Video codec
    {
        "Not defined codec",
        "video/mpeg2",          //AML_MP_VIDEO_CODEC_MPEG12
        "video/mp4v-es",        //AML_MP_VIDEO_CODEC_MPEG4
        "video/avc",            //AML_MP_VIDEO_CODEC_H264
        "video/vc1",            //AML_MP_VIDEO_CODEC_VC1
        "video/avs",            //AML_MP_VIDEO_CODEC_AVS
        "video/hevc",           //AML_MP_VIDEO_CODEC_HEVC
        "video/x-vnd.on2.vp9",  //AML_MP_VIDEO_CODEC_VP9
        "video/avs2",           //AML_MP_VIDEO_CODEC_AVS2
        "video/x-motion-jpeg",  //AML_MP_VIDEO_CODEC_MJPEG
        "video/av01",           //AML_MP_VIDEO_CODEC_AV1
    },
    //Audio codec
    {
        "Not defined codec",
        "audio/mpeg-L2",    //AML_MP_AUDIO_CODEC_MP2
        "audio/mpeg",       //AML_MP_AUDIO_CODEC_MP3
        "audio/ac3",        //AML_MP_AUDIO_CODEC_AC3
        "audio/eac3",       //AML_MP_AUDIO_CODEC_EAC3
        "audio/dtshd",      //AML_MP_AUDIO_CODEC_DTS
        "audio/mp4a-latm",  //AML_MP_AUDIO_CODEC_AAC
        "audio/aac-latm",   //AML_MP_AUDIO_CODEC_LATM
        "audio/raw",        //AML_MP_AUDIO_CODEC_PCM
        "audio/ac4",        //AML_MP_AUDIO_CODEC_AC4
        "audio/flac",       //AML_MP_AUDIO_CODEC_FLAC
        "audio/vorbis",     //AML_MP_AUDIO_CODEC_VORBIS
        "audio/opus",       //AML_MP_AUDIO_CODEC_OPUS
    },
};

const char resolutionMap[][10] = {"1920*1080", "3840*2160", "7680*4320",};

///////////////////////////////////////////////////////////////////////////////
const char* mpCodecId2Str(Aml_MP_CodecID codecId);
const char* mpStreamType2Str(Aml_MP_StreamType streamType);
const char* mpDemuxId2Str(Aml_MP_DemuxId demuxId);
const char* mpPlayerParameterKey2Str(Aml_MP_PlayerParameterKey playerParamKey);
const char* mpAVSyncSource2Str(Aml_MP_AVSyncSource syncSource);
const char* mpPlayerEventType2Str(Aml_MP_PlayerEventType eventType);
const char* mpPlayerWorkMode2Str(Aml_MP_PlayerWorkMode workmode);
const char* mpInputStreamType2Str(Aml_MP_InputStreamType inputStreamType);
const char* mpInputSourceType2Str(Aml_MP_InputSourceType inputSourceType);
const char* mpCASType2Str(Aml_MP_CASType casType);
const char* mpVideoDecideMode2Str(Aml_MP_VideoDecodeMode decodeMode);
const char* mpCASServiceType2Str(Aml_MP_CASServiceType servciceType);
const char* mpVideoErrorRecoveryMode2Str(Aml_MP_VideoErrorRecoveryMode errorRecoveryMode);

vformat_t convertToVFormat(Aml_MP_CodecID videoCodec);
aformat_t convertToAForamt(Aml_MP_CodecID audioCodec);

am_tsplayer_video_codec convertToVideoCodec(Aml_MP_CodecID aml_MP_VideoCodec);
am_tsplayer_audio_codec convertToAudioCodec(Aml_MP_CodecID aml_MP_AudioCodec);
am_tsplayer_input_source_type convertToInputSourceType(Aml_MP_InputSourceType sourceType);
am_tsplayer_avsync_mode convertToAVSyncSourceType(Aml_MP_AVSyncSource avSyncSource);

Aml_MP_StreamType convertToMpStreamType(DVR_StreamType_t streamType);
DVR_StreamType_t convertToDVRStreamType(Aml_MP_StreamType streamType);

Aml_MP_CodecID convertToMpCodecId(DVR_VideoFormat_t fmt);
Aml_MP_CodecID convertToMpCodecId(DVR_AudioFormat_t fmt);
DVR_VideoFormat_t convertToDVRVideoFormat(Aml_MP_CodecID codecId);
DVR_AudioFormat_t convertToDVRAudioFormat(Aml_MP_CodecID codecId);

void convertToMpDVRStream(Aml_MP_DVRStream* mpDvrStream, DVR_StreamPid_t* dvrStream);
void convertToMpDVRStream(Aml_MP_DVRStream* mpDvrStream, DVR_StreamInfo_t* dvrStreamInfo);
void convertToMpDVRSourceInfo(Aml_MP_DVRSourceInfo* dest, DVR_WrapperInfo_t* source);

am_tsplayer_video_match_mode convertToTsPlayerVideoMatchMode(Aml_MP_VideoDisplayMode videoDisplayMode);
am_tsplayer_video_trick_mode convertToTsplayerVideoTrickMode(Aml_MP_VideoDecodeMode videoDecodeMode);
am_tsplayer_audio_out_mode convertToTsPlayerAudioOutMode(Aml_MP_AudioOutputMode audioOutputMode);
void convertToMpVideoInfo(Aml_MP_VideoInfo* mpVideoInfo, am_tsplayer_video_info* tsVideoInfo);
am_tsplayer_audio_stereo_mode convertToTsPlayerAudioStereoMode(Aml_MP_AudioBalance audioBalance);
void convertToMpPlayerEventAudioFormat(Aml_MP_PlayerEventAudioFormat* dest, am_tsplayer_audio_format_t* source);

DVB_DemuxSource_t convertToDVBDemuxSource(Aml_MP_DemuxSource source);
Aml_MP_DemuxSource convertToMpDemuxSource(DVB_DemuxSource_t source);
am_tsplayer_stream_type convertToTsplayerStreamType(Aml_MP_StreamType streamType);
Aml_MP_StreamType convertToAmlMPStreamType(am_tsplayer_stream_type streamType);
am_tsplayer_input_buffer_type inputStreamTypeConvert(Aml_MP_InputStreamType streamType);

int convertToCodecRecoveryMode(Aml_MP_VideoErrorRecoveryMode errorRecoveryMode);
Aml_MP_VideoErrorRecoveryMode convertToAmlMPErrorRecoveryMode(int codecRecoveryMode);

#ifdef HAVE_SUBTITLE
AmlTeletextCtrlParam convertToTeletextCtrlParam(AML_MP_TeletextCtrlParam* teletextCtrlParam);
AmlTeletextEvent convertToTeletextEvent(Aml_MP_TeletextEvent teletextEvent);
AML_MP_SubtitleDataType convertToMpSubtitleDataType(AmlSubDataType subDataType);
#endif

bool isSupportMultiHwDemux();

struct NativeWindowHelper
{
    NativeWindowHelper();
    ~NativeWindowHelper();

    int setSidebandTunnelMode(ANativeWindow* nativeWindow);
    int setSidebandNonTunnelMode(ANativeWindow* nativeWindow, int* videoTunnelId);
    void clearTunnelId();

private:
    #ifdef ANDROID
    ANativeWindow* mNativewindow = nullptr;
    android::sp<android::NativeHandle> mSidebandHandle;
    int mTunnelId = -1;
    int mMesonVtFd = -1;
    #endif
    NativeWindowHelper(const NativeWindowHelper&) = delete;
    NativeWindowHelper& operator= (const NativeWindowHelper&) = delete;
};


int pushBlankBuffersToNativeWindow(ANativeWindow *nativeWindow /* nonnull */);

static inline pid_t gettid()
{
    return syscall(__NR_gettid);
}

const char* convertToMIMEString(Aml_MP_CodecID codecId);
Aml_MP_CodecID convertToMpCodecId(std::string mimeStr);

void split(const std::string& s, std::vector<std::string>& tokens, const std::string& delimiters = " ");
std::string trim(std::string& s, const std::string& chars = " \n");
int setTSNSourceToLocal();
int setTSNSourceToDemod();
}
#endif
