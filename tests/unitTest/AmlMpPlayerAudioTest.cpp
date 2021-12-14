#define LOG_TAG "AmlMpPlayerAudioTest"
#include "AmlMpPlayerVideoTest.h"
#include <utils/AmlMpLog.h>
#include <utils/AmlMpUtils.h>
#include <gtest/gtest.h>
#include "TestUrlList.h"
#include "../amlMpTestSupporter/TestModule.h"
#include <AmlMpTestSupporter.h>
#include <pthread.h>
#include <getopt.h>
#include "source/Source.h"
#include <Aml_MP/Aml_MP.h>
#include "string.h"

using namespace aml_mp;

bool AmlMpPlayerBase::waitAudioChangedEvent(int timeoutMs)
{
#ifdef ANDROID
    std::unique_lock <std::mutex> _l(mLock);
#endif
    return mCond.wait_for(_l, std::chrono::milliseconds(timeoutMs), [this] () { return mAudioChanged; });
}

template<typename T1, typename T2>
void AmlMpPlayerBase::ParameterTest(const std::string & url, T1 key, T2 parameter)
{
    MLOGI("----------AudioTest START----------\n");
    startPlaying(url);
    checkHasVideo(url);
    EXPECT_FALSE(waitPlayingErrors());
    if (mRet == 0)
    {
        void *player = getPlayer();
        EXPECT_EQ(Aml_MP_Player_SetParameter(player, key, &parameter), Aml_MP_Player_GetParameter(player, key, &parameter));
        EXPECT_FALSE(waitPlayingErrors());
    }
    MLOGI("----------AudioTest END----------\n");
    stopPlaying();
}

void AmlMpPlayerBase::setAudioVideoParam(const std::string & url)
{
    createMpTestSupporter();
    mpTestSupporter->setDataSource(url);
    sptr<ProgramInfo> mProgramInfo = mpTestSupporter->getProgramInfo();
    if (mProgramInfo == nullptr)
    {
        printf("Format for this stream is not ts.");
        return;
    }
    mPlayMode = AmlMpTestSupporter::START_VIDEO_START_AUDIO;
    mpTestSupporter->startPlay(mPlayMode, false);
    void *player = getPlayer();
    //set videoParams
    Aml_MP_VideoParams videoParams;
    memset(&videoParams, 0, sizeof(videoParams));
    videoParams.videoCodec = mProgramInfo->videoCodec;
    videoParams.pid = mProgramInfo->videoPid;
    EXPECT_EQ(Aml_MP_Player_SetVideoParams(player, &videoParams), AML_MP_OK);
    //start decoding
    EXPECT_EQ(Aml_MP_Player_StartVideoDecoding(player), AML_MP_OK);
    //audioParams
    Aml_MP_AudioParams audioParams;
    memset(&audioParams, 0, sizeof(audioParams));
    audioParams.audioCodec = mProgramInfo->audioCodec;
    audioParams.pid = 601;
    EXPECT_EQ(Aml_MP_Player_SetAudioParams(player, &audioParams), AML_MP_OK);
    EXPECT_EQ(Aml_MP_Player_StartAudioDecoding(player), AML_MP_OK);
    audioParams.pid = 602;
    EXPECT_EQ(Aml_MP_Player_SetADParams(player, &audioParams), AML_MP_OK);
    EXPECT_EQ(Aml_MP_Player_StartADDecoding(player), AML_MP_OK);
    EXPECT_FALSE(waitPlayingErrors());
}

void AmlMpPlayerBase::SetGetVolume(const std::string & url, float volume)
{
    MLOGI("----------SetVolumeTest START----------\n");
    startPlaying(url);
    checkHasVideo(url);
    if (mRet == 0)
    {
        void *player = getPlayer();
        float volume2;
        EXPECT_EQ(Aml_MP_Player_SetVolume(player, volume), AML_MP_OK);
        EXPECT_EQ(Aml_MP_Player_GetVolume(player, &volume2), AML_MP_OK);
        printf("Set volume: %f, get volume: %f \n", volume, volume2);
        EXPECT_EQ(volume, volume2);
        EXPECT_FALSE(waitPlayingErrors());
    }
    MLOGI("----------SetVolumeTest END----------\n");
    stopPlaying();
}

TEST_F(AmlMpPlayerTest, SwitchAudioTrackTest)
{
    std::string url;
    for (auto &url: mUrls)
    {
        MLOGI("----------SwitchAudioTrackTest START----------\n");
        createMpTestSupporter();
        mpTestSupporter->setDataSource(url);
        sptr<ProgramInfo> mProgramInfo = mpTestSupporter->getProgramInfo();
        if (mProgramInfo == nullptr)
        {
            printf("Format for this stream is not ts.");
            continue;
        }
        std::vector<StreamInfo> audioStreams = mProgramInfo->audioStreams;
        mpTestSupporter->startPlay();
        void *player = getPlayer();
        //audioParams
        Aml_MP_AudioParams audioParams{};
        memset(&audioParams, 0, sizeof(audioParams));
        for (int i = 0; i < audioStreams.size(); i++)
        {
            audioParams.audioCodec = audioStreams.at(i).codecId;
            audioParams.pid = audioStreams.at(i).pid;
            EXPECT_EQ(Aml_MP_Player_SwitchAudioTrack(player, &audioParams), AML_MP_OK);
            printf("audioParams.pid is %d \n", audioParams.pid);
        }
        EXPECT_FALSE(waitPlayingErrors());
        stopPlaying();
        MLOGI("----------SwitchAudioTrackTest END----------\n");
    }
}

TEST_F(AmlMpPlayerTest, SetGetVolumeTest)
{
    std::string url;
    for (auto &url: mUrls)
    {
        SetGetVolume(url, 0);
        SetGetVolume(url, 20);
        SetGetVolume(url, 50);
        SetGetVolume(url, 100);
    }
}

TEST_F(AmlMpPlayerTest, AudioBalanceTest)
{
    std::string url;
    for (auto &url: mUrls)
    {
        for (Aml_MP_AudioBalance audiobalanceEnum = AML_MP_AUDIO_BALANCE_STEREO; audiobalanceEnum <= AML_MP_AUDIO_BALANCE_LRMIX;
            audiobalanceEnum = (Aml_MP_AudioBalance) (audiobalanceEnum + 1))
            {
                MLOGI("audiobalanceEnum is %d \n", audiobalanceEnum);
                ParameterTest(url, AML_MP_PLAYER_PARAMETER_AUDIO_BALANCE, audiobalanceEnum);
            }
    }
}

TEST_F(AmlMpPlayerTest, AudioOutputModeTest)
{
    std::string url;
    for (auto &url: mUrls)
    {
        for (Aml_MP_AudioOutputMode audiooutputmodeEnum = AML_MP_AUDIO_OUTPUT_PCM; audiooutputmodeEnum <= AML_MP_AUDIO_OUTPUT_AUTO;
            audiooutputmodeEnum = (Aml_MP_AudioOutputMode) (audiooutputmodeEnum + 1))
            {
                MLOGI("audiooutputEnum is %d \n", audiooutputmodeEnum);
                ParameterTest(url, AML_MP_PLAYER_PARAMETER_AUDIO_OUTPUT_MODE, audiooutputmodeEnum);
            }
    }
}

TEST_F(AmlMpPlayerTest, AudioOutputDeviceTest)
{
    std::string url;
    for (auto &url: mUrls)
    {
        for (Aml_MP_AudioOutputDevice audiooutputdeviceEnum = AML_MP_AUDIO_OUTPUT_DEVICE_NORMAL; audiooutputdeviceEnum <= AML_MP_AUDIO_OUTPUT_DEVICE_BT;
            audiooutputdeviceEnum = (Aml_MP_AudioOutputDevice) (audiooutputdeviceEnum + 1))
            {
                MLOGI("audiooutputdeviceEnum is %d \n", audiooutputdeviceEnum);
                ParameterTest(url, AML_MP_PLAYER_PARAMETER_AUDIO_OUTPUT_DEVICE, audiooutputdeviceEnum);
            }
    }
}

TEST_F(AmlMpPlayerTest, AudioMuteTest)
{
    for (auto &url: mUrls)
    {
        ParameterTest(url, AML_MP_PLAYER_PARAMETER_AUDIO_MUTE, true);
        ParameterTest(url, AML_MP_PLAYER_PARAMETER_AUDIO_MUTE, false);
    }
}

TEST_F(AmlMpPlayerTest, ADdecodingTest)
{
    std::string url;
    for (auto &url: mUrls)
    {
        MLOGI("----------ADdecodingTest START----------\n");
        setAudioVideoParam(url);
        void *player = getPlayer();
        EXPECT_EQ(Aml_MP_Player_StopADDecoding(player), AML_MP_OK);
        EXPECT_FALSE(waitPlayingErrors());
        stopPlaying();
        MLOGI("----------ADdecodingTest END----------\n");
    }
}

TEST_F(AmlMpPlayerTest, ADMix_master_Test)
{
    std::string url;
    for (auto &url: mUrls)
    {
        MLOGI("----------ADMix_master_Test START----------\n");
        setAudioVideoParam(url);
        void *player = getPlayer();
        Aml_MP_ADVolume adVolume = {AML_MP_MASTER_VOLUME, AML_MP_SLAVE_VOLUME};
        EXPECT_EQ(Aml_MP_Player_SetParameter(player, AML_MP_PLAYER_PARAMETER_AD_MIX_LEVEL, &adVolume), Aml_MP_Player_GetParameter(player, AML_MP_PLAYER_PARAMETER_AD_MIX_LEVEL, &adVolume));
        EXPECT_FALSE(waitPlayingErrors());
        stopPlaying();
    }
}

TEST_F(AmlMpPlayerTest, ADMix_slave_Test)
{
    std::string url;
    for (auto &url: mUrls)
    {
        MLOGI("----------ADMix_slave_Test START----------\n");
        setAudioVideoParam(url);
        void *player = getPlayer();
        #define AML_MP_MASTER_VOLUME 30
        #define AML_MP_SLAVE_VOLUME 70
        Aml_MP_ADVolume adVolume = {AML_MP_MASTER_VOLUME, AML_MP_SLAVE_VOLUME};
        EXPECT_EQ(Aml_MP_Player_SetParameter(player, AML_MP_PLAYER_PARAMETER_AD_MIX_LEVEL, &adVolume), Aml_MP_Player_GetParameter(player, AML_MP_PLAYER_PARAMETER_AD_MIX_LEVEL, &adVolume));
        EXPECT_FALSE(waitPlayingErrors());
        stopPlaying();
    }
}
