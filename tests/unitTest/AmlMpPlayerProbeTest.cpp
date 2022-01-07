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

TEST_F(AmlMpTest, BufferTest)
{
    std::string url;
    for (auto &url: mUrls)
    {
        MLOGI("----------BufferTest START----------\n");
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
        Aml_MP_BufferStat bufferstat;
        EXPECT_EQ(Aml_MP_Player_GetBufferStat(player, &bufferstat), AML_MP_OK);
        EXPECT_FALSE(waitPlayingErrors());
        stopPlaying();
        MLOGI("----------BufferTest END----------\n");
    }
}

TEST_F(AmlMpTest, PtsTest)
{
    std::string url;
    for (auto &url: mUrls)
    {
        MLOGI("----------PtsTest START----------\n");
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
        int64_t pts;
        EXPECT_EQ(Aml_MP_Player_GetCurrentPts(player, AML_MP_STREAM_TYPE_VIDEO, &pts), AML_MP_OK);
        EXPECT_FALSE(waitPlayingErrors());
        stopPlaying();
        MLOGI("----------PtsTest END----------\n");
    }
}

TEST_F(AmlMpTest, VideoInfoTest)
{
    std::string url;
    for (auto &url: mUrls)
    {
        MLOGI("----------VideoInfoTest START----------\n");
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
        waitPlaying(2 * 1000ll);
        void *player = getPlayer();
        Aml_MP_VideoInfo VideoInfo; //{720, 576, 25, 15, 16:9};
        EXPECT_EQ(Aml_MP_Player_GetParameter(player, AML_MP_PLAYER_PARAMETER_VIDEO_INFO, &VideoInfo), AML_MP_OK);
        printf("VideoInfo.bitrate:%d", VideoInfo.bitrate);
        EXPECT_EQ(VideoInfo.width, 720);
        EXPECT_EQ(VideoInfo.height, 576);
        EXPECT_EQ(VideoInfo.frameRate, 25);
        EXPECT_EQ(VideoInfo.bitrate, 15);
        EXPECT_EQ(VideoInfo.ratio64, 1);
        EXPECT_FALSE(waitPlayingErrors());
        stopPlaying();
        MLOGI("----------VideoInfoTest END----------\n");
    }
}

TEST_F(AmlMpTest, VideoDecodeStatTest)
{
    std::string url;
    for (auto &url: mUrls)
    {
        MLOGI("----------VideoDecodeStatTest START----------\n");
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
        waitPlaying(2 * 1000ll);
        void *player = getPlayer();
        Aml_MP_VdecStat vdec_stat;
        EXPECT_EQ(Aml_MP_Player_GetParameter(player, AML_MP_PLAYER_PARAMETER_VIDEO_DECODE_STAT, &vdec_stat), AML_MP_OK);
        printf("trace getParameter, AML_MP_PLAYER_PARAMETER_VIDEO_DECODE_STAT, bit_depth_luma: %d, bit_depth_chroma: %d, frame_dur: %d, status: %d, frame_count: %d, ratio_control: %d, double_write_mode: %d, decode_time_cost: %d, size: %d, vdec_stat.status: %d", vdec_stat.bit_depth_luma, vdec_stat.bit_depth_chroma, vdec_stat.frame_dur, vdec_stat.status, vdec_stat.frame_count, vdec_stat.ratio_control, vdec_stat.double_write_mode, vdec_stat.decode_time_cost, vdec_stat.qos.size, vdec_stat.status);
        EXPECT_EQ(vdec_stat.frame_width, 720);
        EXPECT_EQ(vdec_stat.frame_height, 576);
        EXPECT_EQ(vdec_stat.frame_rate, 25);
        EXPECT_EQ(vdec_stat.frame_dur, 3840);
        EXPECT_EQ(vdec_stat.status, 54);
        EXPECT_FALSE(waitPlayingErrors());
        stopPlaying();
        MLOGI("----------VideoDecodeStatTest END----------\n");
    }
}

TEST_F(AmlMpTest, AudioInfoTest)
{
    std::string url;
    for (auto &url: mUrls)
    {
        MLOGI("----------AudioInfoTest START----------\n");
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
        waitPlaying(2 * 1000ll);
        void *player = getPlayer();
        Aml_MP_AudioInfo AudioInfo; //{720, 576, 25, 15, 16:9};
        EXPECT_EQ(Aml_MP_Player_GetParameter(player, AML_MP_PLAYER_PARAMETER_AUDIO_INFO, &AudioInfo), AML_MP_OK);
        printf("AudioInfo.bitrate:%d, AudioInfo.sample_rate:%d, AudioInfo.channels:%d, AudioInfo.channel_mask:%d", AudioInfo.bitrate, AudioInfo.sample_rate, AudioInfo.channels, AudioInfo.channel_mask);
        EXPECT_EQ(AudioInfo.sample_rate, 48);
        EXPECT_EQ(AudioInfo.channels, 2);
        EXPECT_EQ(AudioInfo.channel_mask, 25);
        EXPECT_EQ(AudioInfo.bitrate, 15);
        EXPECT_FALSE(waitPlayingErrors());
        stopPlaying();
        MLOGI("----------AudioInfoTest END----------\n");
    }
}

TEST_F(AmlMpTest, AudioDecodeStatTest)
{
    std::string url;
    for (auto &url: mUrls)
    {
        MLOGI("----------AudioDecodeStatTest START----------\n");
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
        waitPlaying(2 * 1000ll);
        void *player = getPlayer();
        Aml_MP_AdecStat adec_stat;
        EXPECT_EQ(Aml_MP_Player_GetParameter(player, AML_MP_PLAYER_PARAMETER_AUDIO_DECODE_STAT, &adec_stat), AML_MP_OK);
        printf("trace getParameter, AML_MP_PLAYER_PARAMETER_AUDIO_DECODE_STAT, frame_count: %d, error_frame_count: %d, drop_frame_count: %d", adec_stat.frame_count, adec_stat.error_frame_count, adec_stat.drop_frame_count);
        EXPECT_EQ(adec_stat.frame_count, 720);
        EXPECT_EQ(adec_stat.error_frame_count, 576);
        EXPECT_EQ(adec_stat.drop_frame_count, 25);
        EXPECT_FALSE(waitPlayingErrors());
        stopPlaying();
        MLOGI("----------AudioDecodeStatTest END----------\n");
    }
}

TEST_F(AmlMpTest, AVSyncDoneEventTest)
{
    std::string url;
    for (auto &url: mUrls)
    {
        MLOGI("----------AVSyncDoneEventTest START----------\n");
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
        waitPlaying(2 * 1000ll);
        //void *player = getPlayer();
        EXPECT_TRUE(waitAVSyncDoneEvent(2 * 1000ll));
        stopPlaying();
        MLOGI("----------AVSyncDoneEventTest END----------\n");
    }
}

TEST_F(AmlMpTest, PidChangedEventTest)
{
    std::string url;
    for (auto &url: mUrls)
    {
//special stream
        printf("url is %s", url.c_str());
        if (strstr(url.c_str(), "video_pid_change") != NULL) {
            MLOGI("----------PidChangedEventTest START----------\n");
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
            EXPECT_FALSE(waitPlayingErrors(AML_MP_PID_CHANGED_DURATION));
            EXPECT_TRUE(waitDataLossEvent(AML_MP_DATA_LOSS_EVENT));
            EXPECT_TRUE(waitPidChangedEvent(AML_MP_PID_CHANGED_EVENT));
            stopPlaying();
            MLOGI("----------PidChangedEventTest END----------\n");
        } else {
            continue;
        }
    }
}
