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

void AmlMpBase::DVRPlayback_SetGetVolume(std::string url, float volume)
{
    void* dvrplayer = getDVRPlayer();
    float volume2;
    EXPECT_EQ(Aml_MP_DVRPlayer_SetVolume(dvrplayer, volume), AML_MP_OK);
    EXPECT_EQ(Aml_MP_DVRPlayer_GetVolume(dvrplayer, &volume2), AML_MP_OK);
    printf("Set volume: %f, get volume: %f \n", volume, volume2);
    EXPECT_EQ(volume, volume2);
    EXPECT_FALSE(waitPlayingErrors(5 * 1000ll));
}

TEST_F(AmlMpTest, DVRPlaybackSetGetVolumeTest)
{
    std::string url;
    for (auto &url: mUrls)
    {
        int ret = 0;
        MLOGI("----------DVRPlaybackSetGetVolumeTest START----------\n");
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
        DVRPlayback_SetGetVolume(AML_MP_RECORD_PATH, 0);
        DVRPlayback_SetGetVolume(AML_MP_RECORD_PATH, 20);
        DVRPlayback_SetGetVolume(AML_MP_RECORD_PATH, 50);
        DVRPlayback_SetGetVolume(AML_MP_RECORD_PATH, 100);

        stopPlaying();
        MLOGI("----------DVRPlaybackSetGetVolumeTest END----------\n");
    }
}


