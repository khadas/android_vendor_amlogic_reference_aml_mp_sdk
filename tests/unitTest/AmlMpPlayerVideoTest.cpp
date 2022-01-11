/*
 * Copyright (c) 2020 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */
#define LOG_TAG "AmlMpPlayerVideoTest"
#include "AmlMpTest.h"
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
#include <unistd.h>
#include <fcntl.h>
#include <Aml_MP/Dvr.h>

using namespace aml_mp;

template<typename T1, typename T2>
void AmlMpBase::ParameterTest(const std::string & url, T1 key, T2 parameter)
{
    MLOGI("ParameterTest START: %d \n", parameter);
    runTest(url, false);
    if (mRet == 0)
    {
        void *player = getPlayer();
        EXPECT_EQ(Aml_MP_Player_SetParameter(player, key, &parameter), Aml_MP_Player_GetParameter(player, key, &parameter));
        EXPECT_FALSE(waitPlayingErrors());
    }
    MLOGI("----------ParameterTest END----------\n");
    stopPlaying();
}

TEST_F(AmlMpTest, startPlay_StartAll_StopAll_Test)
{
    std::string url;
    for (auto &url: mUrls)
    {
        mPlayMode = AmlMpTestSupporter::START_ALL_STOP_ALL;
        runTest(url);
    }
}

TEST_F(AmlMpTest, startPlay_StartAll_StopSeparately_Test)
{
    std::string url;
    for (auto &url: mUrls)
    {
        mPlayMode = AmlMpTestSupporter::START_ALL_STOP_SEPARATELY;
        runTest(url);
    }
}

TEST_F(AmlMpTest, startPlay_startSeparately_StopAll_Test)
{
    std::string url;
    for (auto &url: mUrls)
    {
        mPlayMode = AmlMpTestSupporter::START_SEPARATELY_STOP_ALL;
        runTest(url);
    }
}

TEST_F(AmlMpTest, startPlay_StartSeparately_StopSeparately_Test)
{
    std::string url;
    for (auto &url: mUrls)
    {
        mPlayMode = AmlMpTestSupporter::START_SEPARATELY_STOP_SEPARATELY;
        runTest(url);
    }
}

TEST_F(AmlMpTest, startPlay_StartSeparately_StopSeparately_V2_Test)
{
    std::string url;
    for (auto &url: mUrls)
    {
        mPlayMode = AmlMpTestSupporter::START_SEPARATELY_STOP_SEPARATELY_V2;
        runTest(url);
    }
}

TEST_F(AmlMpTest, startPlay_StartAudio_Video_Test)
{
    std::string url;
    for (auto &url: mUrls)
    {
        mPlayMode = AmlMpTestSupporter::START_AUDIO_START_VIDEO;
        runTest(url);
    }
}

TEST_F(AmlMpTest, startPlay_StartVideo_AudioTest)
{
    std::string url;
    for (auto &url: mUrls)
    {
        mPlayMode = AmlMpTestSupporter::START_VIDEO_START_AUDIO;
        runTest(url);
    }
}

TEST_F(AmlMpTest, startPlay_ModeMax_Test)
{
    std::string url;
    for (auto &url: mUrls)
    {
        mPlayMode = AmlMpTestSupporter::PLAY_MODE_MAX;
        runTest(url);
    }
}

TEST_F(AmlMpTest, clearLastFrame)
{
    std::string url;
    for (auto &url: mUrls)
    {
        startPlaying(url);
        checkHasVideo(url);
        if (mRet != 0)
        {
            continue;
        }
        EXPECT_FALSE(waitPlayingErrors());
        void *player = getPlayer();
        int blackOut = 1;
        Aml_MP_Player_SetParameter(player, AML_MP_PLAYER_PARAMETER_BLACK_OUT, &blackOut);
        EXPECT_FALSE(waitPlayingErrors());
        stopPlaying();
    }
}

TEST_F(AmlMpTest, ZappingTest)
{
    std::string url;
    for (auto &url: mUrls)
    {
        runTest(url);
    }
}

TEST_F(AmlMpTest, showHideVideoTest)
{
    std::string url;
    for (auto &url: mUrls)
    {
        startPlaying(url);
        if (mRet == 0)
        {
            checkHasVideo(url);
            EXPECT_FALSE(waitPlayingErrors());
            void *player = getPlayer();
            EXPECT_EQ(Aml_MP_Player_HideVideo(player), AML_MP_OK);
            EXPECT_FALSE(waitPlayingErrors());
            EXPECT_EQ(Aml_MP_Player_ShowVideo(player), AML_MP_OK);
            EXPECT_FALSE(waitPlayingErrors());
        }
        stopPlaying();
    }
}

#ifdef ANDROID
TEST_F(AmlMpTest, setANativeWindowTest)
{
    std::string url;
    for (auto &url: mUrls)
    {
        MLOGI("----------SetANativeWindow START----------");
        startPlaying(url);
        if (mRet == 0)
        {
            checkHasVideo(url);
            EXPECT_FALSE(waitPlayingErrors());
            void *player = getPlayer();
            sptr <NativeUI> mNativeUI = mpTestSupporter->createNativeUI();
            sp <ANativeWindow> mSurface = mNativeUI->getNativeWindow();
            EXPECT_EQ(Aml_MP_Player_SetANativeWindow(player, mSurface.get()), AML_MP_OK);
            MLOGI("----------SetANativeWindow END----------");
        }
        stopPlaying();
    }
}
#endif

TEST_F(AmlMpTest, surfaceControlTest)
{
    std::string url;
    createMpTestSupporter();
    //set surface
    AmlMpTestSupporter::DisplayParam displayParam;
    displayParam.x      = AML_MP_VIDEO_WINDOW_X;
    displayParam.y      = AML_MP_VIDEO_WINDOW_Y;
    displayParam.width  = AML_MP_VIDEO_WINDOW_WIDTH;
    displayParam.height = AML_MP_VIDEO_WINDOW_HEIGHT;
    displayParam.zorder = AML_MP_VIDEO_WINDOW_ZORDER_1;
    displayParam.videoMode = AML_MP_VIDEO_MODE_0;
    mpTestSupporter->setDisplayParam(displayParam);
    for (auto &url: mUrls)
    {
        MLOGI("----------surfaceControlTest START----------");
        runTest(url);
        MLOGI("----------surfaceControlTest END----------");
    }
}

TEST_F(AmlMpTest, setVideoMode_1_Test)
{
    std::string url;
    createMpTestSupporter();
    AmlMpTestSupporter::DisplayParam displayParam;
    displayParam.x = AML_MP_VIDEO_WINDOW_X;
    displayParam.y = AML_MP_VIDEO_WINDOW_Y;
    displayParam.width = AML_MP_VIDEO_WINDOW_WIDTH;
    displayParam.height = AML_MP_VIDEO_WINDOW_HEIGHT;
    displayParam.zorder = AML_MP_VIDEO_WINDOW_ZORDER_1;
    displayParam.videoMode = AML_MP_VIDEO_MODE_1;
    mpTestSupporter->setDisplayParam(displayParam);
    for (auto &url: mUrls)
    {
        MLOGI("----------setVideoMode_1_Test START----------");
        SetVideoWindow(url, displayParam.x, displayParam.y, displayParam.width, displayParam.height);
        MLOGI("----------setVideoMode_1_Test END----------");
    }
}

TEST_F(AmlMpTest, setVideoMode_2_Test)
{
    std::string url;
    createMpTestSupporter();
    AmlMpTestSupporter::DisplayParam displayParam;
    displayParam.x      = AML_MP_VIDEO_WINDOW_X;
    displayParam.y      = AML_MP_VIDEO_WINDOW_Y;
    displayParam.width  = AML_MP_VIDEO_WINDOW_WIDTH;
    displayParam.height = AML_MP_VIDEO_WINDOW_HEIGHT;
    displayParam.zorder = AML_MP_VIDEO_WINDOW_ZORDER_2;
    displayParam.videoMode = AML_MP_VIDEO_MODE_2;
    mpTestSupporter->setDisplayParam(displayParam);
    for (auto &url: mUrls)
    {
        MLOGI("----------setVideoMode_2_Test START----------");
        SetVideoWindow(url, displayParam.x, displayParam.y, displayParam.width, displayParam.height);
        MLOGI("----------setVideoMode_2_Test END----------");
    }
}

TEST_F(AmlMpTest, videoTunnelID_Test)
{
    std::string url;
    createMpTestSupporter();
    //set Surface
    AmlMpTestSupporter::DisplayParam displayParam;
    displayParam.x      = AML_MP_VIDEO_WINDOW_X;
    displayParam.y      = AML_MP_VIDEO_WINDOW_Y;
    displayParam.width  = AML_MP_VIDEO_WINDOW_WIDTH;
    displayParam.height = AML_MP_VIDEO_WINDOW_HEIGHT;
    mpTestSupporter->setDisplayParam(displayParam);
    for (auto &url: mUrls)
    {
        MLOGI("----------videoTunnelIDTest START----------");
        startPlaying(url);
        checkHasVideo(url);
        if (mRet != 0)
        {
            continue;
        }
        EXPECT_FALSE(waitPlayingErrors());
        void *player = getPlayer();
        EXPECT_EQ(Aml_MP_Player_SetParameter(player, AML_MP_PLAYER_PARAMETER_VIDEO_TUNNEL_ID, &displayParam),Aml_MP_Player_GetParameter(player, AML_MP_PLAYER_PARAMETER_VIDEO_TUNNEL_ID, &displayParam));
        EXPECT_FALSE(waitPlayingErrors());
        MLOGI("----------videoTunnelIDTest END----------");
        stopPlaying();
    }
}

TEST_F(AmlMpTest, displayModeTest)
{
    std::string url;
    for (auto &url: mUrls)
    {
        for (Aml_MP_VideoDisplayMode displayModeEnum = AML_MP_VIDEO_DISPLAY_MODE_NORMAL; displayModeEnum <= AML_MP_VIDEO_DISPLAY_MODE_CUSTOM;
            displayModeEnum = (Aml_MP_VideoDisplayMode) (displayModeEnum + 1))
            {
                MLOGI("displayModeEnum is %d \n", displayModeEnum);
                ParameterTest(url, AML_MP_PLAYER_PARAMETER_VIDEO_DISPLAY_MODE, displayModeEnum);
            }
    }
}

TEST_F(AmlMpTest, videoDecodeModeTest)
{
    std::string url;
    for (auto &url: mUrls)
    {
        for (Aml_MP_VideoDecodeMode videodecodeModeEnum = AML_MP_VIDEO_DECODE_MODE_NONE; videodecodeModeEnum <= AML_MP_VIDEO_DECODE_MODE_IONLY;
            videodecodeModeEnum = (Aml_MP_VideoDecodeMode) (videodecodeModeEnum + 1))
            {
                MLOGI("videodecodeModeEnum is %d \n", videodecodeModeEnum);
                ParameterTest(url, AML_MP_PLAYER_PARAMETER_VIDEO_DECODE_MODE, videodecodeModeEnum);
            }
    }
}

///////////////////////////////////////////////////////////////////////////////
static void usage()
{
    printf("Usage: AmlMpTest <test_source_dir>\n"
    "   try --help for more details.\n");
}

int main(int argc, char** argv)
{
    if (argc == 1)
    {
        usage();
        return 0;
    }
    std::string testpath;
    for (size_t i = 1; i < argc; i++)
    {
        if (strcmp(argv[1], "--url"))
        {
            testpath = argv[i];
            for (int j = i; j < argc - 1; ++j)
            {
                argv[j] = argv[j + 1];
            }
            argc--;
        }
        else
        {
            testpath = argv[i+1];
            for (int j = i; j < argc - 2; ++j)
            {
                argv[j] = argv[j + 2];
            }
            argc-=2;
        }
    }
    testing::InitGoogleTest(&argc, argv);

    if (!testpath.empty())
    {
        if (testpath.find("://") != std::string::npos) {
            printf("url testpath:%s, argc:%d\n", testpath.c_str(), argc);
            TestUrlList::instance().initSourceUrl(testpath);
        } else {
            printf("dir testpath2:%s, argc:%d\n", testpath.c_str(), argc);
            TestUrlList::instance().initSourceDir(testpath);
        }
    }

    return RUN_ALL_TESTS();
}
