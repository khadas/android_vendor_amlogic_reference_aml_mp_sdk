#define LOG_TAG "AudioTrackWrapper"
#include "AudioTrackWrapper.h"

namespace aml_mp {

static audio_format_t getAudioFormat(Aml_MP_CodecID aml_MP_AudioCodec)
{
    switch (aml_MP_AudioCodec) {
        case AML_MP_AUDIO_CODEC_MP2:
        case AML_MP_AUDIO_CODEC_MP3:
            return AUDIO_FORMAT_MP3;
        case AML_MP_AUDIO_CODEC_AC3:
            return AUDIO_FORMAT_AC3;
        case AML_MP_AUDIO_CODEC_EAC3:
            return AUDIO_FORMAT_E_AC3;
        case AML_MP_AUDIO_CODEC_DTS:
            return AUDIO_FORMAT_DTS;
        case AML_MP_AUDIO_CODEC_AAC:
        case AML_MP_AUDIO_CODEC_LATM:
            return AUDIO_FORMAT_AAC_HE_V2;
        case AML_MP_AUDIO_CODEC_PCM:
            return AUDIO_FORMAT_PCM_16_BIT;
        case AML_MP_AUDIO_CODEC_AC4:
            return AUDIO_FORMAT_AC4;
        case AML_MP_AUDIO_CODEC_BASE:
        case AML_MP_AUDIO_CODEC_FLAC:
        default:
            return AUDIO_FORMAT_INVALID;
    }
}

AudioTrackWrapper::AudioTrackWrapper() {
    snprintf(mName, sizeof(mName), "%s", LOG_TAG);
    mAudioTrack = new AudioTrack();
}

AudioTrackWrapper::~AudioTrackWrapper() {

}

void AudioTrackWrapper::configureAudioTrack(int sampleRate, int channelCount, audio_format_t aFormat, int filterId, int hwAvSyncId) {
    audio_offload_info_t offloadInfo;
    offloadInfo = AUDIO_INFO_INITIALIZER;
    offloadInfo.format = aFormat;
    offloadInfo.sample_rate = sampleRate;
    offloadInfo.channel_mask = audio_channel_out_mask_from_count(channelCount);
    offloadInfo.has_video = false;
    offloadInfo.stream_type = AUDIO_STREAM_MUSIC; //required for offload
    offloadInfo.content_id = filterId;
    offloadInfo.sync_id = hwAvSyncId;
    mAudioTrack->set(AUDIO_STREAM_MUSIC,
                    sampleRate,
                    aFormat,
                    audio_channel_out_mask_from_count(channelCount),
                    0,
                    AUDIO_OUTPUT_FLAG_NONE,
                    nullptr,
                    nullptr,
                    0,
                    0,
                    false,
                    AUDIO_SESSION_ALLOCATE,
                    android::AudioTrack::TRANSFER_DEFAULT,
                    &offloadInfo);
}

int AudioTrackWrapper::configure(const Aml_MP_AudioParams& audioParams, int filterId, int hwAvSyncId) {
    int defSampleRate = 48000;
    int defChannelCount = 2;
    audio_format_t aFormat = getAudioFormat(audioParams.audioCodec);
    int sampleRate = audioParams.nSampleRate > 0 ? audioParams.nSampleRate : defSampleRate;
    int channelCount = audioParams.nChannels > 0 ? audioParams.nChannels : defChannelCount;
    configureAudioTrack(sampleRate, channelCount, aFormat, filterId, hwAvSyncId);

    if (mAudioTrack->initCheck() != 0) {
        MLOGE("mAudioTrack->initCheck failed, retry legacy config");
        // retry legacy config, need fixed by SWPL-58253
        sampleRate = 4001;
        channelCount = 1;
        if (aFormat == AUDIO_FORMAT_AAC_HE_V2) {
            aFormat = AUDIO_FORMAT_AAC_LC;
        }
        configureAudioTrack(sampleRate, channelCount, aFormat, filterId, hwAvSyncId);
        if (mAudioTrack->initCheck() != 0) {
            MLOGE("mAudioTrack->initCheck failed");
            return -1;
        }
    }
    mAudioTrackInited = true;
    return 0;
}

int AudioTrackWrapper::start() {
    if (!mAudioTrackInited) {
        return -1;
    }
    mAudioTrack->start();
    return 0;
}

int AudioTrackWrapper::pause() {
    if (!mAudioTrackInited) {
        return -1;
    }
    mAudioTrack->pause();
    return 0;
}

int AudioTrackWrapper::resume() {
    if (!mAudioTrackInited) {
        return -1;
    }
    mAudioTrack->start();
    return 0;
}

int AudioTrackWrapper::flush() {
    if (!mAudioTrackInited) {
        return -1;
    }
    mAudioTrack->flush();
    return 0;
}

int AudioTrackWrapper::stop() {
    if (!mAudioTrackInited) {
        return -1;
    }
    mAudioTrack->stop();
    mAudioTrackInited = false;
    return 0;
}

void AudioTrackWrapper::release() {
    mAudioTrack = nullptr;
}

} // namespace aml_mp