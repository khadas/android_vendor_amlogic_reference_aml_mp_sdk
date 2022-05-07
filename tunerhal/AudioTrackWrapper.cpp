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

int AudioTrackWrapper::threadFunc() {
    char *audioData = (char *)malloc(256);
    while (mAudioStarted) {
        if (!mAudioTrack)
            break;
        int written = mAudioTrack->write(audioData, 256,false);
        // MLOGI("threadFunc written=%d",written);
         if (written != 256 ) {
            usleep(100 *1000); //100ms
         }

    }
THREADOUT:
    ALOGI("[%s %d] thread out\n", __FUNCTION__, __LINE__);
    free(audioData);
    return 0;
}

void *AudioTrackWrapper::ThreadWrapper(void *me) {
    AudioTrackWrapper *Convertor = static_cast<AudioTrackWrapper *>(me);
    Convertor->threadFunc();
    return NULL;
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
    Mutex::Autolock lock(mMutex);
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
    Mutex::Autolock lock(mMutex);
    MLOGI("AudioTrackWrapper::start");
    if (!mAudioTrackInited) {
        return -1;
    }
    mAudioTrack->start();
    if (!mAudioStarted) {
        mAudioStarted =true;
        pthread_attr_t attr;
        pthread_attr_init(&attr);
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
        pthread_create(&mThread, &attr, ThreadWrapper, this);
        pthread_attr_destroy(&attr);
    }
    return 0;
}

int AudioTrackWrapper::pause() {
    Mutex::Autolock lock(mMutex);
     MLOGI("AudioTrackWrapper::pause");
    if (!mAudioTrackInited) {
        return -1;
    }
    mAudioTrack->pause();
    return 0;
}

int AudioTrackWrapper::resume() {
    Mutex::Autolock lock(mMutex);
    MLOGI("AudioTrackWrapper::resume");
    if (!mAudioTrackInited) {
        return -1;
    }
    mAudioTrack->start();
    return 0;
}

int AudioTrackWrapper::flush() {
    Mutex::Autolock lock(mMutex);
    MLOGI("AudioTrackWrapper::flush");
    if (!mAudioTrackInited) {
        return -1;
    }
    mAudioTrack->flush();
    return 0;
}

int AudioTrackWrapper::stop() {
    MLOGI("AudioTrackWrapper::stop");
    Mutex::Autolock lock(mMutex);
    if (!mAudioTrackInited) {
        return -1;
    }
    mAudioStarted =false;
    pthread_join(mThread,NULL);
    mAudioTrack->stop();
    mAudioTrackInited = false;
    return 0;
}

void AudioTrackWrapper::release() {
    MLOGI("AudioTrackWrapper::release");
    mAudioTrack = nullptr;
}

} // namespace aml_mp