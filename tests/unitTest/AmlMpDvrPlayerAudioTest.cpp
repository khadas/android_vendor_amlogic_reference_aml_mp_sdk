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

void AmlMpBase::DVRPlayback_SetGetVolume(std::string url __unused, float volume)
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
    char recordPathInfo[100];
    for (auto &url: mUrls)
    {
        int ret = 0;
        MLOGI("----------DVRPlaybackSetGetVolumeTest START----------\n");
        //TODO: AUT Need change test code as follow
        //mpTestSupporter need call as this order:
        //setDataSource(url) -> prepare(cryptoMode_ifNeed) -> (getProgramInfo) -> start
        createMpTestSupporter(false);
        mpTestSupporter->setDataSource(url);
        mpTestSupporter->prepare();
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
        sprintf(recordPathInfo, "%s?demuxid=%d&sourceid=%d", AML_MP_RECORD_PATH, 1, 1);
        std::string finalUrl = recordPathInfo;
        mpTestSupporter2->setDataSource(finalUrl);
        mpTestSupporter2->prepare(CryptoMode);
        mpTestSupporter2->startDVRPlayback();
        DVRPlayback_SetGetVolume(AML_MP_RECORD_PATH, 0);
        DVRPlayback_SetGetVolume(AML_MP_RECORD_PATH, 20);
        DVRPlayback_SetGetVolume(AML_MP_RECORD_PATH, 50);
        DVRPlayback_SetGetVolume(AML_MP_RECORD_PATH, 100);

        stopPlaying();
        MLOGI("----------DVRPlaybackSetGetVolumeTest END----------\n");
    }
}


