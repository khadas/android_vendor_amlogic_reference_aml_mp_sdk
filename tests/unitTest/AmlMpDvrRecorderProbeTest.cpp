#define LOG_TAG "AmlMpDvrRecorderProbeTest"
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

void AmlMpBase::getDVRSourceInfo(Aml_MP_DVRSourceInfo info)
{
    time_t              time = info.time;
    loff_t              size = info.size;
    uint32_t            pkts = info.pkts;
    printf("info.time: %ld, info.size: %lld, info.pkts: %d \n", time, size, pkts);
    EXPECT_GE(time, time_temp);
    EXPECT_GE(size, size_temp);
    time_temp = time;
    size_temp = size;
}

TEST_F(AmlMpTest, RecorderStatusTest)
{
    std::string url;
    for (auto &url: mUrls)
    {
        int ret = 0;
        MLOGI("----------RecorderStatusTest START----------\n");
        createMpTestSupporter(false);
        mpTestSupporter->setDataSource(url);
        mpTestSupporter->prepare(CryptoMode);
        EXPECT_EQ(ret = mpTestSupporter->startRecord(), AML_MP_OK);
        void* recorder = getRecorder();
        waitPlaying(15 * 1000ll);
        for (int i = 0; i <= 10; i++)
        {
            Aml_MP_DVRRecorderStatus status;
            EXPECT_EQ(Aml_MP_DVRRecorder_GetStatus(recorder, &status), AML_MP_OK);
            waitPlaying(2 * 1000ll);
            getDVRSourceInfo(status.info);
        }
        stopPlaying();
        MLOGI("----------RecorderStatusTest END----------\n");
    }
}

TEST_F(AmlMpTest, RecorderStatusEventTest)
{
    std::string url;
    for (auto &url: mUrls)
    {
        int ret = 0;
        MLOGI("----------RecorderStatusEventTest START----------\n");
        createMpTestSupporter(false);
        mpTestSupporter->setDataSource(url);
        mpTestSupporter->prepare(CryptoMode);
        EXPECT_EQ(ret = mpTestSupporter->startRecord(), AML_MP_OK);
        void* recorder = getRecorder();
        EXPECT_EQ(Aml_MP_DVRRecorder_Pause(recorder), AML_MP_OK);
        waitPlaying(5 * 1000ll);
        EXPECT_EQ(Aml_MP_DVRRecorder_Resume(recorder), AML_MP_OK);
        waitPlaying(5 * 1000ll);
        EXPECT_TRUE(waitDvrRecorderStatusEvent(5 * 1000ll));
        stopPlaying();
        MLOGI("----------RecorderStatusEventTest END----------\n");
    }
}

TEST_F(AmlMpTest, RecorderSyncEventTest)
{
    std::string url;
    for (auto &url: mUrls)
    {
        int ret = 0;
        MLOGI("----------RecorderSyncEventTest START----------\n");
        createMpTestSupporter(false);
        mpTestSupporter->setDataSource(url);
        mpTestSupporter->prepare(CryptoMode);
        EXPECT_EQ(ret = mpTestSupporter->startRecord(), AML_MP_OK);
        waitPlaying(5 * 1000ll);
        EXPECT_TRUE(waitAVSyncDoneEvent(5 * 1000ll));
        stopPlaying();
        MLOGI("----------RecorderSyncEventTest END----------\n");
    }
}

TEST_F(AmlMpTest, RecorderWriteErrorEventTest)
{
#if 0
    std::string url;
    for (auto &url: mUrls)
    {
        int ret = 0;
        MLOGI("----------RecorderWriteErrorEventTest START----------\n");
        createMpTestSupporter(false);
        mpTestSupporter->setDataSource(url);
        sptr<ProgramInfo> mProgramInfo = mpTestSupporter->getProgramInfo();
        if (mProgramInfo == nullptr)
        {
            printf("Format for this stream is not ts.");
            continue;
        }
        EXPECT_EQ(ret = mpTestSupporter->startRecord(), AML_MP_OK);
        waitPlaying(10 * 1000ll);

        char command[128];
        //strcpy(command, "adb root");
        //EXPECT_EQ(ret = system(command), AML_MP_OK);
        //strcpy(command, "adb remount");
        //EXPECT_EQ(ret = system(command), AML_MP_OK);
        //strcpy(command, "adb shell umount -l /mnt/user/0/7F5D-3C01");
        strcpy(command, "adb shell umount -l /storage/7F5D-3C01/");
        system(command);
        strcpy(command, "chmod 444 /storage/7F5D-3C01/amlMpRecordFile-0000.ts");
        system(command);
        //EXPECT_EQ(ret = system(command), AML_MP_OK);
        EXPECT_TRUE(waitDvrRecorderWriteErrorEvent(5 * 1000ll));
        strcpy(command, "adb shell mount /dev/block/vold/public:8,1 /storage/7F5D-3C01/");
        system(command);
        //EXPECT_EQ(ret = system(command), AML_MP_OK);
        stopPlaying();
        MLOGI("----------RecorderWriteErrorEventTest END----------\n");
    }
#endif
}


