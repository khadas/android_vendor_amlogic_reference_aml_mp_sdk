#ifndef AUDIOTRACK_WRAPPER_H_
#define AUDIOTRACK_WRAPPER_H_

#include <media/AudioTrack.h>
#include <utils/AmlMpUtils.h>
#include <utils/AmlMpRefBase.h>
#include "Aml_MP/Common.h"

using android::AudioTrack;
using android::sp;
using android::Mutex;
namespace aml_mp {

class AudioTrackWrapper : public AmlMpRefBase {
public:
    AudioTrackWrapper();
    ~AudioTrackWrapper();
    void configureAudioTrack(int sampleRate, int channelCount, audio_format_t aFormat, int filterId, int hwAvSyncId);
    int configure(const Aml_MP_AudioParams& audioParams, int filterId, int hwAvSyncId);
    int start();
    int pause();
    int resume();
    int flush();
    int stop();
    void release();
private:
    char mName[50];
    pthread_t mThread;
    sp<AudioTrack> mAudioTrack = nullptr;
    bool mAudioTrackInited = false;
    bool mAudioStarted = false;
    static void *ThreadWrapper(void *me);
    int threadFunc();
    mutable Mutex mMutex;
};

} // namespace aml_mp

#endif /* AUDIOTRACK_WRAPPER_H_ */