#define LOG_TAG "AmlMpDvrRecorderTest"
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

AML_MP_DVRRECORDER AmlMpBase::getRecorder()
{
    sptr <TestModule> testModule = mpTestSupporter->getRecorder();
    AML_MP_DVRRECORDER recorder = testModule->getCommandHandle();
    return recorder;
}

void AmlMpBase::DVRSegment(std::string url)
{
    uint32_t segmentNums;
    uint64_t* segmentIds = nullptr;
    int segmentIndex = 0;
    Aml_MP_DVRSegmentInfo segmentInfo;
    url = mpTestSupporter2->stripUrlIfNeeded(url);
    EXPECT_EQ(Aml_MP_DVRRecorder_GetSegmentList(url.c_str(), &segmentNums, &segmentIds), AML_MP_OK);
    EXPECT_EQ(Aml_MP_DVRRecorder_GetSegmentInfo(url.c_str(), segmentIds[segmentIndex], &segmentInfo), AML_MP_OK);
    EXPECT_EQ(Aml_MP_DVRRecorder_DeleteSegment(url.c_str(), segmentIds[segmentIndex]), AML_MP_OK);
}

TEST_F(AmlMpTest, NormalRecorderTest)
{
    std::string url;
    for (auto &url: mUrls)
    {
        int ret = 0;
        MLOGI("----------NormalRecorderTest START----------\n");
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
        EXPECT_EQ(Aml_MP_DVRRecorder_Pause(recorder), AML_MP_OK);
        waitPlaying(5 * 1000ll);
        EXPECT_EQ(Aml_MP_DVRRecorder_Resume(recorder), AML_MP_OK);
        waitPlaying(5 * 1000ll);
        Aml_MP_DVRRecorderStatus status;
        EXPECT_EQ(Aml_MP_DVRRecorder_GetStatus(recorder, &status), AML_MP_OK);


        createMpTestSupporter2();
        url = "dvr://storage/7F5D-3C01/amlMpRecordFile";
        mpTestSupporter2->getmUrl(url);
        mpTestSupporter2->mDemuxId = AML_MP_HW_DEMUX_ID_1;

        mpTestSupporter2->startDVRPlayback();
        waitPlaying(20 * 1000ll);
        DVRSegment(url);
        stopPlaying();
        MLOGI("----------NormalRecorderTest END----------\n");
    }
}

TEST_F(AmlMpTest, TimeshiftRecordTest)
{
    std::string url;
    for (auto &url: mUrls)
    {
        int ret = 0;
        MLOGI("----------TimeshiftRecordTest START----------\n");
        createMpTestSupporter(false);
        mpTestSupporter->setDataSource(url);
        sptr<ProgramInfo> mProgramInfo = mpTestSupporter->getProgramInfo();
        if (mProgramInfo != nullptr)
        {
            EXPECT_EQ(ret = mpTestSupporter->startRecord(true, true), AML_MP_OK);
            waitPlaying(20 * 1000ll);
        }

        createMpTestSupporter2();
        url = "dvr://data/amlMpRecordFile";
        mpTestSupporter2->getmUrl(url);
        mpTestSupporter2->mDemuxId = AML_MP_HW_DEMUX_ID_1;
        EXPECT_EQ(ret = mpTestSupporter2->startDVRPlayback(true), AML_MP_OK);
        waitPlaying(20 * 1000ll);
        DVRSegment(url);
        stopPlaying();
        MLOGI("----------TimeshiftRecordTest END----------\n");
    }
}

TEST_F(AmlMpTest, DynamicSetStreamTest)
{
    std::string url;
    for (auto &url: mUrls)
    {
        int ret = 0;
        MLOGI("----------DynamicSetStreamTest START----------\n");
        createMpTestSupporter(false);
        mpTestSupporter->setDataSource(url);
        sptr<ProgramInfo> mProgramInfo = mpTestSupporter->getProgramInfo();

        if (mProgramInfo != nullptr)
        {
            mProgramInfo->videoPid = 220;
            mProgramInfo->audioPid = 131;
            EXPECT_EQ(ret = mpTestSupporter->startRecord(false, false), AML_MP_OK);
            EXPECT_EQ(ret = mpTestSupporter->setStreams(), AML_MP_OK);
            EXPECT_EQ(ret = mpTestSupporter->startAftersetStreams(), AML_MP_OK);
            waitPlaying(20 * 1000ll);

            mProgramInfo->videoPid = 320;
            mProgramInfo->audioPid = 132;
            EXPECT_EQ(ret = mpTestSupporter->setStreams(), AML_MP_OK);
            waitPlaying(20 * 1000ll);
        }
        stopPlaying();

        createMpTestSupporter2();
        url = "dvr://data/amlMpRecordFile";
        mpTestSupporter2->getmUrl(url);
        mpTestSupporter2->mDemuxId = AML_MP_HW_DEMUX_ID_1;
        EXPECT_EQ(ret = mpTestSupporter2->startDVRPlayback(), AML_MP_OK);
        waitPlaying(40 * 1000ll);
        //get segment list/info/delete
        //DVRSegment(url);

        stopPlaying();
        MLOGI("----------DynamicSetStreamTest END----------\n");
    }
}
