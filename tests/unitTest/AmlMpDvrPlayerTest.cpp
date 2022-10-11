#define LOG_TAG "AmlMpDvrPlayerTest"
#include "AmlMpTest.h"
#include <utils/AmlMpLog.h>
#include <utils/AmlMpUtils.h>
#include <gtest/gtest.h>
#include "TestUrlList.h"
#include "../amlMpTestSupporter/TestModule.h"
#include <AmlMpTestSupporter.h>
#include <DVRRecord.h>
#include <pthread.h>
#include <getopt.h>
#include "source/Source.h"
#include <Aml_MP/Aml_MP.h>
#include "string.h"
#include <stdio.h>

using namespace aml_mp;

TEST_F(AmlMpTest, DVRPlayback_SetStreams_Test)
{
    std::string url;
    char recordPathInfo[100];
    for (auto &url: mUrls)
    {
        MLOGI("----------DVRPlayback_SetStreams_Test START----------\n");
        DVRRecordRecordStream(url);

        sptr<AmlMpTestSupporter> testDvrPlayer = nullptr;
        createMpTestSupporter(&testDvrPlayer);
        testDvrPlayer->setDataSource(AML_MP_RECORD_PATH);
        testDvrPlayer->prepare(CryptoMode);
        testDvrPlayer->startDVRPlayback();
        AML_MP_DVRPLAYER dvrplayer = getDVRPlayer(testDvrPlayer);
        Aml_MP_DVRPlayerStatus status;
        EXPECT_EQ(Aml_MP_DVRPlayer_GetStatus(dvrplayer, &status), AML_MP_OK);
        printf("nbstreams is: %d \n", status.pids.nbStreams);

        Aml_MP_DVRStreamArray streams;
        memset(&streams, 0, sizeof(streams));
        for (int i = 0; i < status.pids.nbStreams; ++i)
        {
            switch (status.pids.streams[i].type)
            {
            case AML_MP_STREAM_TYPE_VIDEO:
                streams.streams[AML_MP_DVR_VIDEO_INDEX].type = AML_MP_STREAM_TYPE_VIDEO;
                streams.streams[AML_MP_DVR_VIDEO_INDEX].codecId = status.pids.streams[i].codecId;
                streams.streams[AML_MP_DVR_VIDEO_INDEX].pid = status.pids.streams[i].pid;
                break;

            case AML_MP_STREAM_TYPE_AUDIO:
                streams.streams[AML_MP_DVR_AUDIO_INDEX].type = AML_MP_STREAM_TYPE_AUDIO;
                streams.streams[AML_MP_DVR_AUDIO_INDEX].codecId = status.pids.streams[i].codecId;
                streams.streams[AML_MP_DVR_AUDIO_INDEX].pid = status.pids.streams[i].pid;
                break;

            default:
                break;
            }
        }
        EXPECT_EQ(Aml_MP_DVRPlayer_SetStreams(dvrplayer, &streams), AML_MP_OK);
        testDvrPlayer->startDVRPlaybackAfterSetStreams();
        EXPECT_FALSE(waitPlaying(20* 1000ll));

        testDvrPlayer->stop();
        MLOGI("----------DVRPlayback_SetStreams_Test END----------\n");
    }
}

TEST_F(AmlMpTest, DVRPlaybackPauseResumeTest)
{
    std::string url;
    char recordPathInfo[100];
    for (auto &url: mUrls)
    {
        MLOGI("----------DVRPlaybackPauseResumeTest START----------\n");
        DVRRecordRecordStream(url);

        sptr<AmlMpTestSupporter> testDvrPlayer = nullptr;
        createMpTestSupporter(&testDvrPlayer);
        testDvrPlayer->setDataSource(AML_MP_RECORD_PATH);
        testDvrPlayer->prepare(CryptoMode);
        testDvrPlayer->startDVRPlayback();
        AML_MP_DVRPLAYER dvrplayer = getDVRPlayer(testDvrPlayer);
        waitPlaying(5 * 1000ll);
        EXPECT_EQ(Aml_MP_DVRPlayer_Pause(dvrplayer), AML_MP_OK);
        waitPlaying(1 * 1000ll);
        //check pause
        std::string frame_before = get_frame_count_checkpoint();
        EXPECT_FALSE(waitPlaying(4 * 1000ll));
        std::string frame_after = get_frame_count_checkpoint();
        EXPECT_EQ(frame_before, frame_after);
        //check resume
        EXPECT_EQ(Aml_MP_DVRPlayer_Resume(dvrplayer), AML_MP_OK);
        EXPECT_FALSE(waitPlayingErrors(15 * 1000ll));

        testDvrPlayer->stop();
        MLOGI("----------DVRPlaybackPauseResumeTest END----------\n");
    }
}

TEST_F(AmlMpTest, DVRPlaybackSeekTest)
{
    std::string url;
    char recordPathInfo[100];
    for (auto &url: mUrls)
    {
        MLOGI("----------DVRPlaybackSeekTest START----------\n");
        DVRRecordRecordStream(url);

        sptr<AmlMpTestSupporter> testDvrPlayer = nullptr;
        createMpTestSupporter(&testDvrPlayer);
        testDvrPlayer->setDataSource(AML_MP_RECORD_PATH);
        testDvrPlayer->prepare(CryptoMode);
        testDvrPlayer->startDVRPlayback();
        AML_MP_DVRPLAYER dvrplayer = getDVRPlayer(testDvrPlayer);
        waitPlaying(5 * 1000ll);
        int timeOffset = 5 * 1000ll;
        EXPECT_EQ(Aml_MP_DVRPlayer_Seek(dvrplayer, timeOffset), AML_MP_OK);
        EXPECT_FALSE(waitPlayingErrors(15 * 1000ll));

        testDvrPlayer->stop();
        MLOGI("----------DVRPlaybackSeekTest END----------\n");
    }
}

TEST_F(AmlMpTest, DVRPlaybackRate_Normal_Test)
{
    std::string url;
    float rate = 1.0;
    char recordPathInfo[100];
    for (auto &url: mUrls)
    {
        MLOGI("----------DVRPlaybackRate_Normal_Test START----------\n");
        DVRRecordRecordStream(url);

        sptr<AmlMpTestSupporter> testDvrPlayer = nullptr;
        createMpTestSupporter(&testDvrPlayer);
        testDvrPlayer->setDataSource(AML_MP_RECORD_PATH);
        testDvrPlayer->prepare(CryptoMode);
        testDvrPlayer->startDVRPlayback();
        AML_MP_DVRPLAYER dvrplayer = getDVRPlayer(testDvrPlayer);
        EXPECT_EQ(Aml_MP_DVRPlayer_SetPlaybackRate(dvrplayer, rate), AML_MP_OK);
        EXPECT_FALSE(waitPlayingErrors(20 * 1000ll));
        testDvrPlayer->stop();
        MLOGI("----------DVRPlaybackRate_Normal_Test END----------\n");
    }
}

TEST_F(AmlMpTest, DVRPlaybackRate_MicroSpeed_Test)
{
    std::string url;
    float rate[] = {0.125, 0.25, 0.5};
    char recordPathInfo[100];
    for (auto &url: mUrls)
    {
        MLOGI("----------DVRPlaybackRate_MicroSpeed_Test START----------\n");
        DVRRecordRecordStream(url);

        sptr<AmlMpTestSupporter> testDvrPlayer = nullptr;
        createMpTestSupporter(&testDvrPlayer);
        testDvrPlayer->setDataSource(AML_MP_RECORD_PATH);
        testDvrPlayer->prepare(CryptoMode);
        testDvrPlayer->startDVRPlayback();
        AML_MP_DVRPLAYER dvrplayer = getDVRPlayer(testDvrPlayer);
        for (size_t i = 0; i < sizeof(rate)/sizeof(rate[0]); i++)
        {
            EXPECT_EQ(Aml_MP_DVRPlayer_SetPlaybackRate(dvrplayer, rate[i]), AML_MP_OK);
            EXPECT_FALSE(waitPlayingErrors(20 * 1000ll));
        }
        testDvrPlayer->stop();
        MLOGI("----------DVRPlaybackRate_MicroSpeed_Test END----------\n");
    }
}

TEST_F(AmlMpTest, DVRPlaybackRate_FastForward_Test)
{
    std::string url;
    float rate[] = {2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 12.0, 16.0};
    char recordPathInfo[100];
    for (auto &url: mUrls)
    {
        MLOGI("----------DVRPlaybackRate_FastForward_Test START----------\n");
        DVRRecordRecordStream(url);

        sptr<AmlMpTestSupporter> testDvrPlayer = nullptr;
        createMpTestSupporter(&testDvrPlayer);
        testDvrPlayer->setDataSource(AML_MP_RECORD_PATH);
        testDvrPlayer->prepare(CryptoMode);
        testDvrPlayer->startDVRPlayback();
        AML_MP_DVRPLAYER dvrplayer = getDVRPlayer(testDvrPlayer);
        for (size_t i = 0; i < sizeof(rate)/sizeof(rate[0]); i++)
        {
            EXPECT_EQ(Aml_MP_DVRPlayer_SetPlaybackRate(dvrplayer, rate[i]), AML_MP_OK);
            EXPECT_FALSE(waitPlayingErrors(10 * 1000ll));
        }
        testDvrPlayer->stop();
        MLOGI("----------DVRPlaybackRate_FastForward_Test END----------\n");
    }
}

TEST_F(AmlMpTest, DVRPlaybackRate_FastBackward_Test)
{
    std::string url;
    float rate[] = {-1.0, -2.0, -4.0, -8.0, -12.0, -16.0, -32.0};
    char recordPathInfo[100];
    for (auto &url: mUrls)
    {
        MLOGI("----------DVRPlaybackRate_FastBackward_Test START----------\n");
        DVRRecordRecordStream(url);

        sptr<AmlMpTestSupporter> testDvrPlayer = nullptr;
        createMpTestSupporter(&testDvrPlayer);
        testDvrPlayer->setDataSource(AML_MP_RECORD_PATH);
        testDvrPlayer->prepare(CryptoMode);
        testDvrPlayer->startDVRPlayback();
        AML_MP_DVRPLAYER dvrplayer = getDVRPlayer(testDvrPlayer);
        for (size_t i = 0; i < sizeof(rate)/sizeof(rate[0]); i++)
        {
            EXPECT_EQ(Aml_MP_DVRPlayer_SetPlaybackRate(dvrplayer, rate[i]), AML_MP_OK);
            waitPlaying(10 * 1000ll);
            EXPECT_FALSE(waitPlayingErrors(10 * 1000ll));
        }
        testDvrPlayer->stop();
        MLOGI("----------DVRPlaybackRate_FastBackward_Test END----------\n");
     }
}
