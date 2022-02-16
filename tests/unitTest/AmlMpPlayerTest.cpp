#define LOG_TAG "AmlMpTest"
#include "AmlMpTest.h"
#include <utils/AmlMpLog.h>
#include <utils/AmlMpUtils.h>
#include "TestUrlList.h"
#include "../amlMpTestSupporter/TestModule.h"
#include <AmlMpTestSupporter.h>
#include <pthread.h>
#include <getopt.h>
#include "source/Source.h"
#include <Aml_MP/Aml_MP.h>
#include "string.h"
#include "../../player/Aml_MP_PlayerImpl.h"

using namespace aml_mp;

void AmlMpBase::videoDecoding(const std::string & url, bool mStart, bool mSourceReceiver)
{
    MLOGI("----------VideoDecodingTest START----------\n");
    createMpTestSupporter();
    mpTestSupporter->setDataSource(url);
    sptr<ProgramInfo> mProgramInfo = mpTestSupporter->getProgramInfo();
    if (mProgramInfo == nullptr)
    {
        printf("Format for this stream is not ts.");
        return;
    }
    mProgramInfo->scrambled=true;
    mPlayMode = AmlMpTestSupporter::START_VIDEO_START_AUDIO;
    mpTestSupporter->startPlay(mPlayMode, mStart, mSourceReceiver);
    void *player = getPlayer();
    //set videoParams
    Aml_MP_VideoParams videoParams;
    memset(&videoParams, 0, sizeof(videoParams));
    videoParams.videoCodec = mProgramInfo->videoCodec;
    videoParams.pid = mProgramInfo->videoPid;
    EXPECT_EQ(Aml_MP_Player_SetVideoParams(player, &videoParams), AML_MP_OK);
    //start decoding
    EXPECT_EQ(Aml_MP_Player_StartVideoDecoding(player), AML_MP_OK);
    if (!mSourceReceiver) {
        waitPlaying(1 * 1000ll);
    }else {
        EXPECT_FALSE(waitPlaying(1 * 1000ll));
        EXPECT_FALSE(waitPlayingErrors());
    }
    //stop decoding
    EXPECT_EQ(Aml_MP_Player_StopVideoDecoding(player), AML_MP_OK);
    stopPlaying();
    MLOGI("----------VideoDecodingTest END----------\n");
}

void AmlMpBase::audioDecoding(const std::string & url, bool mStart, bool mSourceReceiver)
{
    MLOGI("----------AudioDecodingTest START----------\n");
    createMpTestSupporter();
    mpTestSupporter->setDataSource(url);
    sptr<ProgramInfo> mProgramInfo = mpTestSupporter->getProgramInfo();
    if (mProgramInfo == nullptr)
    {
        printf("Format for this stream is not ts.");
        return;
    }
    mProgramInfo->scrambled=true;
    mPlayMode = AmlMpTestSupporter::START_AUDIO_START_VIDEO;
    mpTestSupporter->startPlay(mPlayMode, mStart, mSourceReceiver);
    void *player = getPlayer();
    //set audioParams
    Aml_MP_AudioParams audioParams;
    memset(&audioParams, 0, sizeof(audioParams));
    audioParams.audioCodec = mProgramInfo->audioCodec;
    audioParams.pid = mProgramInfo->audioPid;
    EXPECT_EQ(Aml_MP_Player_SetAudioParams(player, &audioParams), AML_MP_OK);
    //start decoding
    EXPECT_EQ(Aml_MP_Player_StartAudioDecoding(player), AML_MP_OK);
    if (!mSourceReceiver) {
        waitPlaying(1 * 1000ll);
    }else {
        EXPECT_FALSE(waitPlaying(1 * 1000ll));
        EXPECT_FALSE(waitPlayingErrors());
    }
    //stop decoding
    EXPECT_EQ(Aml_MP_Player_StopAudioDecoding(player), AML_MP_OK);
    stopPlaying();
    MLOGI("----------AudioDecodingTest END----------\n");
}

#ifdef ANDROID
void AmlMpBase::FCCAndPIPTest(const std::string & url, bool mPIP)
{
    MLOGI("----------FCCAndPIPTest START----------\n");
    createMpTestSupporter();
    char demuxInfo[30];
    sprintf(demuxInfo, "?demuxid=%d&sourceid=%d", 1, 1);
    std::string finalUrl = url + demuxInfo;
    mpTestSupporter->setDataSource(finalUrl);
    sptr<ProgramInfo> mProgramInfo = mpTestSupporter->getProgramInfo();
    if (mProgramInfo == nullptr)
    {
        printf("Format for this stream is not ts.");
        return;
    }
    mProgramInfo->scrambled=true;
    if (mPIP) {
        AmlMpTestSupporter::DisplayParam displayParam;
        displayParam.x      = AML_MP_VIDEO_WINDOW_X;
        displayParam.y      = AML_MP_VIDEO_WINDOW_Y;
        displayParam.width  = AML_MP_VIDEO_WINDOW_WIDTH;
        displayParam.height = AML_MP_VIDEO_WINDOW_HEIGHT;
        displayParam.zorder = AML_MP_VIDEO_WINDOW_ZORDER_1;
        displayParam.videoMode = AML_MP_VIDEO_MODE_1;
        mpTestSupporter->setDisplayParam(displayParam);
    }
    mProgramInfo->videoPid = 640;
    mProgramInfo->audioPid = 641;
    mpTestSupporter->mChannelId = AML_MP_CHANNEL_ID_MAIN;
    mpTestSupporter->startPlay();
    //void *player = getPlayer();
    if (mPIP) {
        EXPECT_FALSE(waitPlayingErrors());
    }

    createMpTestSupporter2();
    sprintf(demuxInfo, "?demuxid=%d&sourceid=%d", 0, 0);
    finalUrl = url + demuxInfo;
    mpTestSupporter2->setDataSource(finalUrl);
    sptr<ProgramInfo> mProgramInfo2 = mpTestSupporter2->getProgramInfo();
    if (mProgramInfo2 == nullptr)
    {
        printf("Format for this stream is not ts.");
        return;
    }
    mProgramInfo2->videoPid = 620;
    mProgramInfo2->audioPid = 621;
    mpTestSupporter2->mChannelId = AML_MP_CHANNEL_ID_PIP;
    if (!mPIP) {
        mpTestSupporter2->mWorkMode = AML_MP_PLAYER_MODE_CACHING_ONLY;
    }
    mpTestSupporter2->startPlay();
    if (mPIP) {
        EXPECT_FALSE(waitPlayingErrors());
    } else {
        waitPlaying();
    }
    if (!mPIP) {
        //cache->no surface
        mWorkMode = AML_MP_PLAYER_MODE_CACHING_ONLY;
        mpTestSupporter->setWindow(false);
        mpTestSupporter->setParameter(AML_MP_PLAYER_PARAMETER_WORK_MODE, &mWorkMode);
        //normal->surface
        mWorkMode = AML_MP_PLAYER_MODE_NORMAL;
        mpTestSupporter2->setParameter(AML_MP_PLAYER_PARAMETER_WORK_MODE, &mWorkMode);
        mpTestSupporter2->setWindow();
        waitPlaying(2 * 1000ll);
        EXPECT_FALSE(waitPlayingErrors());
    }
    stopPlaying();
    MLOGI("----------FCCAndPIPTest END----------\n");
}
#endif

TEST_F(AmlMpTest, PlayerStartStopTest)
{
    std::string url;
    for (auto &url: mUrls)
    {
        MLOGI("----------PlayerStartTest START----------\n");
        createMpTestSupporter();
        mpTestSupporter->setDataSource(url);
        sptr<ProgramInfo> mProgramInfo = mpTestSupporter->getProgramInfo();
        if (mProgramInfo == nullptr)
        {
            printf("Format for this stream is not ts.");
            continue;
        }
        mProgramInfo->scrambled=true;
        mpTestSupporter->startPlay();
        void *player = getPlayer();
        EXPECT_EQ(Aml_MP_Player_Start(player), -2);
        EXPECT_FALSE(waitPlayingErrors());
        EXPECT_EQ(Aml_MP_Player_Stop(player), AML_MP_OK);
        stopPlaying();
        MLOGI("----------PlayerStartTest END----------\n");
    }
}

TEST_F(AmlMpTest, VideoDecodingTest)
{
    std::string url;
    for (auto &url: mUrls)
    {
        videoDecoding(url, false, false);
    }
}

TEST_F(AmlMpTest, VideoDecodingTest1)
{
    std::string url;
    for (auto &url: mUrls)
    {
        videoDecoding(url, false, true);
    }
}

TEST_F(AmlMpTest, AudioDecodingTest)
{
    std::string url;
    for (auto &url: mUrls)
    {
        audioDecoding(url, false, false);
    }
}

TEST_F(AmlMpTest, AudioDecodingTest1)
{
    std::string url;
    for (auto &url: mUrls)
    {
        audioDecoding(url, false, true);
    }
}

TEST_F(AmlMpTest, SubtitleDecodingTest)
{
    std::string url;
    for (auto &url: mUrls)
    {
        MLOGI("----------SubtitleDecodingTest START----------\n");
        createMpTestSupporter();
        mpTestSupporter->setDataSource(url);
        sptr<ProgramInfo> mProgramInfo = mpTestSupporter->getProgramInfo();
        if (mProgramInfo == nullptr)
        {
            printf("Format for this stream is not ts.");
            continue;
        }
        mProgramInfo->scrambled=true;
        mPlayMode = AmlMpTestSupporter::START_ALL_STOP_ALL;
        mpTestSupporter->startPlay(mPlayMode, false);
        void *player = getPlayer();
        //set subtitleParams
        Aml_MP_SubtitleParams subtitleParams{};
        subtitleParams.subtitleCodec = mProgramInfo->subtitleCodec;
        switch (subtitleParams.subtitleCodec) {
            case AML_MP_SUBTITLE_CODEC_DVB:
                subtitleParams.pid = mProgramInfo->subtitlePid;
                subtitleParams.compositionPageId = mProgramInfo->compositionPageId;
                subtitleParams.ancillaryPageId = mProgramInfo->ancillaryPageId;
                break;
            case AML_MP_SUBTITLE_CODEC_TELETEXT:
                subtitleParams.pid = mProgramInfo->subtitlePid;
                break;
            case AML_MP_SUBTITLE_CODEC_SCTE27:
                subtitleParams.pid = mProgramInfo->subtitlePid;
                break;
            default:
                break;
        }
        EXPECT_EQ(Aml_MP_Player_SetSubtitleParams(player, &subtitleParams), AML_MP_OK);
        //start decoding
        EXPECT_EQ(Aml_MP_Player_StartSubtitleDecoding(player), AML_MP_OK);
        waitPlaying(1 * 1000ll);
        //stop decoding
        EXPECT_EQ(Aml_MP_Player_StopSubtitleDecoding(player), AML_MP_OK);
        stopPlaying();
        MLOGI("----------SubtitleDecodingTest END----------\n");
    }
}

TEST_F(AmlMpTest, PauseResumeTest)
{
    std::string url;
    for (auto &url: mUrls)
    {
        MLOGI("----------PauseResumeTest START----------\n");
        createMpTestSupporter();
        mpTestSupporter->setDataSource(url);
        sptr<ProgramInfo> mProgramInfo = mpTestSupporter->getProgramInfo();
        if (mProgramInfo == nullptr)
        {
            printf("Format for this stream is not ts.");
            continue;
        }
        mProgramInfo->scrambled=true;
        mpTestSupporter->startPlay();
        void *player = getPlayer();
        EXPECT_FALSE(waitPlayingErrors(5 * 1000ll));
        EXPECT_EQ(Aml_MP_Player_Pause(player), AML_MP_OK);
        EXPECT_FALSE(waitPlaying(2 * 1000ll));
        //check pause
        std::string frame_before = get_frame_count_checkpoint();
        EXPECT_FALSE(waitPlaying(5 * 1000ll));
        std::string frame_after = get_frame_count_checkpoint();
        EXPECT_EQ(frame_before, frame_after);
        EXPECT_EQ(Aml_MP_Player_Resume(player), AML_MP_OK);
        //check resume
        EXPECT_FALSE(waitPlayingErrors());
        stopPlaying();
        MLOGI("----------PauseResumeTest END----------\n");
    }
}

TEST_F(AmlMpTest, FlushTest)
{
    std::string url;
    for (auto &url: mUrls)
    {
        MLOGI("----------FlushTest START----------\n");
        createMpTestSupporter();
        mpTestSupporter->setDataSource(url);
        sptr<ProgramInfo> mProgramInfo = mpTestSupporter->getProgramInfo();
        if (mProgramInfo == nullptr)
        {
            printf("Format for this stream is not ts.");
            continue;
        }
        mProgramInfo->scrambled=true;
        mpTestSupporter->startPlay();
        void *player = getPlayer();
        EXPECT_EQ(Aml_MP_Player_Flush(player), AML_MP_OK);
        EXPECT_FALSE(waitPlayingErrors(5 * 1000ll));
        stopPlaying();
        MLOGI("----------FlushTest END----------\n");
    }
}

TEST_F(AmlMpTest, PlaybackRateTest)
{
    float rate[] = {-10.0, -2.0, 2.0, 5.0, 0.5, 0.9};
    std::string url;
    for (auto &url: mUrls)
    {
        MLOGI("----------PlaybackRateTest START----------\n");
        startPlaying(url);
        if (mRet == 0)
        {
            checkHasVideo(url);
            EXPECT_FALSE(waitPlayingErrors(5 * 1000ll));
            void *player = getPlayer();
            for (size_t i = 0; i < sizeof(rate)/sizeof(rate[0]); i++)
            {
                EXPECT_EQ(Aml_MP_Player_SetPlaybackRate(player, rate[i]), AML_MP_OK);
                EXPECT_TRUE(waitVideoChangedEvent(kWaitVideoChangedMs));
                EXPECT_FALSE(waitPlaying(3 * 1000ll));
            }
        }
        stopPlaying();
        MLOGI("----------PlaybackRateTest END----------\n");
    }
}

TEST_F(AmlMpTest, PlaybackRateTest1)
{
    float rate[] = {-1.5, -1.0, -0.5};
    std::string url;
    for (auto &url: mUrls)
    {
        MLOGI("----------PlaybackRateTest1 START----------\n");
        startPlaying(url);
        if (mRet == 0)
        {
            checkHasVideo(url);
            EXPECT_FALSE(waitPlayingErrors(5 * 1000ll));
            void *player = getPlayer();
            for (size_t i = 0; i < sizeof(rate)/sizeof(rate[0]); i++)
            {
                EXPECT_EQ(Aml_MP_Player_SetPlaybackRate(player, rate[i]), AML_MP_ERROR_BAD_VALUE);
                EXPECT_FALSE(waitPlayingErrors());
            }
        }
        stopPlaying();
        MLOGI("----------PlaybackRateTest1 END----------\n");
    }
}

TEST_F(AmlMpTest, PlaybackRateTest2)
{
    float rate = 0.0;
    std::string url;
    for (auto &url: mUrls)
    {
        MLOGI("----------PlaybackRateTest2 START----------\n");
        startPlaying(url);
        if (mRet == 0)
        {
            checkHasVideo(url);
            EXPECT_FALSE(waitPlayingErrors(5 * 1000ll));
            void *player = getPlayer();
            EXPECT_EQ(Aml_MP_Player_SetPlaybackRate(player, rate), AML_MP_OK);
            EXPECT_FALSE(waitPlaying(3 * 1000ll));
            std::string frame_before = get_frame_count_checkpoint();
            EXPECT_FALSE(waitPlaying(3 * 1000ll));
            std::string frame_after = get_frame_count_checkpoint();
            EXPECT_EQ(frame_before, frame_after);
        }
        stopPlaying();
        MLOGI("----------PlaybackRateTest2 END----------\n");
    }
}

TEST_F(AmlMpTest, AVSyncModeTest)
{
    std::string url;
    for (auto &url: mUrls)
    {
        MLOGI("----------AVSyncModeTest START----------\n");
        //enum
        for (Aml_MP_AVSyncSource avsyncsourceEnum = AML_MP_AVSYNC_SOURCE_DEFAULT; avsyncsourceEnum <= AML_MP_AVSYNC_SOURCE_PCR; avsyncsourceEnum = (Aml_MP_AVSyncSource) (avsyncsourceEnum + 1))
        {
            MLOGI("avsyncsourceEnum is %d \n", avsyncsourceEnum);
            createMpTestSupporter();
            mpTestSupporter->setDataSource(url);
            sptr<ProgramInfo> mProgramInfo = mpTestSupporter->getProgramInfo();
            if (mProgramInfo == nullptr)
            {
                printf("Format for this stream is not ts.");
                continue;
            }
            mProgramInfo->scrambled=true;
            mpTestSupporter->setAVSyncSource(avsyncsourceEnum);
            mpTestSupporter->startPlay();
            checkHasVideo(url);
            void *player = getPlayer();
            EXPECT_EQ(Aml_MP_Player_SetAVSyncSource(player, avsyncsourceEnum), AML_MP_OK);
            EXPECT_FALSE(waitPlayingErrors());
            stopPlaying();
        }
        MLOGI("----------AVSyncModeTest END----------\n");
    }
}

#ifdef ANDROID
TEST_F(AmlMpTest, PIPTest)
{
    std::string url;
    for (auto &url: mUrls)
    {
        FCCAndPIPTest(url, true);
    }
}

TEST_F(AmlMpTest, FCCTest)
{
//FCC: normal,cache(no surface)->cache,normal
    std::string url;
    for (auto &url: mUrls)
    {
        FCCAndPIPTest(url, false);
    }
}
#endif

