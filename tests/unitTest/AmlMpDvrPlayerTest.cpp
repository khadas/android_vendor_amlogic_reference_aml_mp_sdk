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

AML_MP_DVRPLAYER AmlMpBase::getDVRPlayer()
{
    sptr <TestModule> testModule = mpTestSupporter2->getDVRPlayer();
    printf("testModule %p \n", testModule.get());
    AML_MP_DVRPLAYER dvrplayer = testModule->getCommandHandle();
    return dvrplayer;
}

TEST_F(AmlMpTest, DVRPlayback_SetStreams_Test)
{
    std::string url;
    for (auto &url: mUrls)
    {
        int ret = 0;
        MLOGI("----------DVRPlayback_SetStreams_Test START----------\n");
        createMpTestSupporter(false);
        mpTestSupporter->setDataSource(url);
        sptr<ProgramInfo> mProgramInfo = mpTestSupporter->getProgramInfo();
        if (mProgramInfo == nullptr)
        {
            printf("Format for this stream is not ts.");
            continue;
        }
        EXPECT_EQ(ret = mpTestSupporter->startRecord(), AML_MP_OK);
        void* recorder = getRecorder();
        waitPlaying(20 * 1000ll);

        createMpTestSupporter2();
        mpTestSupporter2->getmUrl(AML_MP_RECORD_PATH);
        mpTestSupporter2->mDemuxId = AML_MP_HW_DEMUX_ID_1;

        mpTestSupporter2->startDVRPlayback();
        void* dvrplayer = getDVRPlayer();
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

            case AML_MP_STREAM_TYPE_AUDIO:
                streams.streams[AML_MP_DVR_AUDIO_INDEX].type = AML_MP_STREAM_TYPE_AUDIO;
                streams.streams[AML_MP_DVR_AUDIO_INDEX].codecId = status.pids.streams[i].codecId;
                streams.streams[AML_MP_DVR_AUDIO_INDEX].pid = status.pids.streams[i].pid;
            EXPECT_EQ(Aml_MP_DVRPlayer_SetStreams(dvrplayer, &streams), AML_MP_OK);
            mpTestSupporter2->startDVRPlaybackAfterSetStreams();
            EXPECT_FALSE(waitPlaying(20* 1000ll));
            }
        }
        stopPlaying();
        MLOGI("----------DVRPlayback_SetStreams_Test END----------\n");
    }
}

TEST_F(AmlMpTest, DVRPlaybackPauseResumeTest)
{
    std::string url;
    for (auto &url: mUrls)
    {
        int ret = 0;
        MLOGI("----------DVRPlaybackPauseResumeTest START----------\n");
        createMpTestSupporter(false);
        mpTestSupporter->setDataSource(url);
        sptr<ProgramInfo> mProgramInfo = mpTestSupporter->getProgramInfo();
        if (mProgramInfo == nullptr)
        {
            printf("Format for this stream is not ts.");
            continue;
        }
        EXPECT_EQ(ret = mpTestSupporter->startRecord(), AML_MP_OK);
        void* recorder = getRecorder();
        waitPlaying(20 * 1000ll);

        createMpTestSupporter2();
        mpTestSupporter2->getmUrl(AML_MP_RECORD_PATH);
        mpTestSupporter2->mDemuxId = AML_MP_HW_DEMUX_ID_1;

        mpTestSupporter2->startDVRPlayback();
        void* dvrplayer = getDVRPlayer();
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

        stopPlaying();
        MLOGI("----------DVRPlaybackPauseResumeTest END----------\n");
    }
}

TEST_F(AmlMpTest, DVRPlaybackSeekTest)
{
    std::string url;
    for (auto &url: mUrls)
    {
        int ret = 0;
        MLOGI("----------DVRPlaybackSeekTest START----------\n");
        createMpTestSupporter(false);
        mpTestSupporter->setDataSource(url);
        sptr<ProgramInfo> mProgramInfo = mpTestSupporter->getProgramInfo();
        if (mProgramInfo == nullptr)
        {
            printf("Format for this stream is not ts.");
            continue;
        }
        EXPECT_EQ(ret = mpTestSupporter->startRecord(), AML_MP_OK);
        void* recorder = getRecorder();
        waitPlaying(20 * 1000ll);

        createMpTestSupporter2();
        mpTestSupporter2->getmUrl(AML_MP_RECORD_PATH);
        mpTestSupporter2->mDemuxId = AML_MP_HW_DEMUX_ID_1;

        mpTestSupporter2->startDVRPlayback();
        void* dvrplayer = getDVRPlayer();
        waitPlaying(5 * 1000ll);
        int timeOffset = 5 * 1000ll;
        EXPECT_EQ(Aml_MP_DVRPlayer_Seek(dvrplayer, timeOffset), AML_MP_OK);
        EXPECT_FALSE(waitPlayingErrors(15 * 1000ll));

        stopPlaying();
        MLOGI("----------DVRPlaybackSeekTest END----------\n");
    }
}

TEST_F(AmlMpTest, DVRPlaybackRate_Normal_Test)
{
    std::string url;
    float rate = 1.0;
    for (auto &url: mUrls)
    {
        int ret = 0;
        MLOGI("----------DVRPlaybackRate_Normal_Test START----------\n");
        createMpTestSupporter(false);
        mpTestSupporter->setDataSource(url);
        sptr<ProgramInfo> mProgramInfo = mpTestSupporter->getProgramInfo();
        if (mProgramInfo == nullptr)
        {
            printf("Format for this stream is not ts.");
            continue;
        }
        EXPECT_EQ(ret = mpTestSupporter->startRecord(), AML_MP_OK);
        void* recorder = getRecorder();
        waitPlaying(20 * 1000ll);

        createMpTestSupporter2();
        mpTestSupporter2->getmUrl(AML_MP_RECORD_PATH);
        mpTestSupporter2->mDemuxId = AML_MP_HW_DEMUX_ID_1;

        mpTestSupporter2->startDVRPlayback();
        void* dvrplayer = getDVRPlayer();
        EXPECT_EQ(Aml_MP_DVRPlayer_SetPlaybackRate(dvrplayer, rate), AML_MP_OK);
        EXPECT_FALSE(waitPlayingErrors(20 * 1000ll));
        stopPlaying();
        MLOGI("----------DVRPlaybackRate_Normal_Test END----------\n");
    }
}

TEST_F(AmlMpTest, DVRPlaybackRate_MicroSpeed_Test)
{
    std::string url;
    float rate[] = {0.125, 0.25, 0.5};
    for (auto &url: mUrls)
    {
        int ret = 0;
        MLOGI("----------DVRPlaybackRate_MicroSpeed_Test START----------\n");
        createMpTestSupporter(false);
        mpTestSupporter->setDataSource(url);
        sptr<ProgramInfo> mProgramInfo = mpTestSupporter->getProgramInfo();
        if (mProgramInfo == nullptr)
        {
            printf("Format for this stream is not ts.");
            continue;
        }
        EXPECT_EQ(ret = mpTestSupporter->startRecord(), AML_MP_OK);
        void* recorder = getRecorder();
        waitPlaying(30 * 1000ll);

        createMpTestSupporter2();
        mpTestSupporter2->getmUrl(AML_MP_RECORD_PATH);
        mpTestSupporter2->mDemuxId = AML_MP_HW_DEMUX_ID_1;

        mpTestSupporter2->startDVRPlayback();
        void* dvrplayer = getDVRPlayer();
        for (int i = 0; i < sizeof(rate)/sizeof(rate[0]); i++)
        {
            EXPECT_EQ(Aml_MP_DVRPlayer_SetPlaybackRate(dvrplayer, rate[i]), AML_MP_OK);
            EXPECT_FALSE(waitPlayingErrors(10 * 1000ll));
        }
        stopPlaying();
        MLOGI("----------DVRPlaybackRate_MicroSpeed_Test END----------\n");
    }
}

TEST_F(AmlMpTest, DVRPlaybackRate_FastForward_Test)
{
    std::string url;
    float rate[] = {2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 12.0, 16.0, 32.0, 48.0, 64.0, 128.0};
    for (auto &url: mUrls)
    {
        int ret = 0;
        MLOGI("----------DVRPlaybackRate_FastForward_Test START----------\n");
        createMpTestSupporter(false);
        mpTestSupporter->setDataSource(url);
        sptr<ProgramInfo> mProgramInfo = mpTestSupporter->getProgramInfo();
        if (mProgramInfo == nullptr)
        {
            printf("Format for this stream is not ts.");
            continue;
        }
        EXPECT_EQ(ret = mpTestSupporter->startRecord(), AML_MP_OK);
        void* recorder = getRecorder();
        waitPlaying(130 * 1000ll);

        createMpTestSupporter2();
        mpTestSupporter2->getmUrl(AML_MP_RECORD_PATH);
        mpTestSupporter2->mDemuxId = AML_MP_HW_DEMUX_ID_1;

        mpTestSupporter2->startDVRPlayback();
        void* dvrplayer = getDVRPlayer();
        for (int i = 0; i < sizeof(rate)/sizeof(rate[0]); i++)
        {
            EXPECT_EQ(Aml_MP_DVRPlayer_SetPlaybackRate(dvrplayer, rate[i]), AML_MP_OK);
            EXPECT_FALSE(waitPlayingErrors(10 * 1000ll));
        }
        stopPlaying();
        MLOGI("----------DVRPlaybackRate_FastForward_Test END----------\n");
    }
}

TEST_F(AmlMpTest, DVRPlaybackRate_FastBackward_Test)
{
    std::string url;
    float rate[] = {-1.0, -2.0, -4.0, -8.0, -12.0, -16.0, -32.0, -48.0, -64.0, -128.0};
    for (auto &url: mUrls)
    {
        int ret = 0;
        MLOGI("----------DVRPlaybackRate_FastBackward_Test START----------\n");
        createMpTestSupporter(false);
        mpTestSupporter->setDataSource(url);
        sptr<ProgramInfo> mProgramInfo = mpTestSupporter->getProgramInfo();
        if (mProgramInfo == nullptr)
        {
            printf("Format for this stream is not ts.");
            continue;
        }
        EXPECT_EQ(ret = mpTestSupporter->startRecord(), AML_MP_OK);
        void* recorder = getRecorder();
        waitPlaying(100 * 1000ll);

        createMpTestSupporter2();
        mpTestSupporter2->getmUrl(AML_MP_RECORD_PATH);
        mpTestSupporter2->mDemuxId = AML_MP_HW_DEMUX_ID_1;

        mpTestSupporter2->startDVRPlayback();
        void* dvrplayer = getDVRPlayer();
        for (int i = 0; i < sizeof(rate)/sizeof(rate[0]); i++)
        {
            EXPECT_EQ(Aml_MP_DVRPlayer_SetPlaybackRate(dvrplayer, rate[i]), AML_MP_OK);
            EXPECT_FALSE(waitPlayingErrors(10 * 1000ll));
        }
        stopPlaying();
        MLOGI("----------DVRPlaybackRate_FastBackward_Test END----------\n");
    }
}



