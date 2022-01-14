#define LOG_TAG "AmlMpDvrPlayerVideoTest"
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

TEST_F(AmlMpTest, DVRPlaybackShowHideVideoTest)
{
    std::string url;
    for (auto &url: mUrls)
    {
        int ret = 0;
        MLOGI("----------DVRPlaybackShowHideVideoTest START----------\n");
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
        EXPECT_EQ(Aml_MP_DVRPlayer_HideVideo(dvrplayer), AML_MP_OK);
        EXPECT_TRUE(waitPlayingErrors(10* 1000ll));
        EXPECT_EQ(Aml_MP_DVRPlayer_ShowVideo(dvrplayer), AML_MP_OK);
        EXPECT_FALSE(waitPlayingErrors(10* 1000ll));

        stopPlaying();
        MLOGI("----------DVRPlaybackShowHideVideoTest END----------\n");
    }
}

TEST_F(AmlMpTest, DVRPlaybackSetVideoWindowTest)
{
    std::string url;
    for (auto &url: mUrls)
    {
        int ret = 0;
        MLOGI("----------DVRPlaybackSetVideoWindowTest START----------\n");
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

        AmlMpTestSupporter::DisplayParam displayParam;
        displayParam.x = AML_MP_VIDEO_WINDOW_X;
        displayParam.y = AML_MP_VIDEO_WINDOW_Y;
        displayParam.width = AML_MP_VIDEO_WINDOW_WIDTH;
        displayParam.height = AML_MP_VIDEO_WINDOW_HEIGHT;
        displayParam.zorder = AML_MP_VIDEO_WINDOW_ZORDER_1;
        displayParam.videoMode = AML_MP_VIDEO_MODE_1;
        mpTestSupporter2->setDisplayParam(displayParam);

        mpTestSupporter2->startDVRPlayback();
        void* dvrplayer = getDVRPlayer();
        EXPECT_EQ(Aml_MP_DVRPlayer_SetVideoWindow(dvrplayer, displayParam.x, displayParam.y, displayParam.width, displayParam.height), AML_MP_OK);
        EXPECT_FALSE(waitPlayingErrors(15* 1000ll));

        stopPlaying();
        MLOGI("----------DVRPlaybackSetVideoWindowTest END----------\n");
    }
}

