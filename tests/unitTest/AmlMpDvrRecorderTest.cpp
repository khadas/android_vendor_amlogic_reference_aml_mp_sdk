#define LOG_TAG "AmlMpDvrRecorderTest"
#include "AmlMpTest.h"
#include <utils/AmlMpLog.h>
#include <utils/AmlMpUtils.h>
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

# if 0
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
#endif

TEST_F(AmlMpTest, NormalRecorderTest)
{
    std::string url;
    for (auto &url: mUrls)
    {
        int ret = 0;
        MLOGI("----------NormalRecorderTest START----------\n");
        createMpTestSupporter(false);
        mpTestSupporter->setDataSource(url);
        mpTestSupporter->prepare(CryptoMode);
        EXPECT_EQ(ret = mpTestSupporter->startRecord(), AML_MP_OK);
        void* recorder = getRecorder();
        EXPECT_EQ(Aml_MP_DVRRecorder_Pause(recorder), AML_MP_OK);
        waitPlaying(5 * 1000ll);
        EXPECT_EQ(Aml_MP_DVRRecorder_Resume(recorder), AML_MP_OK);
        waitPlaying(15 * 1000ll);
        Aml_MP_DVRRecorderStatus status;
        EXPECT_EQ(Aml_MP_DVRRecorder_GetStatus(recorder, &status), AML_MP_OK);
        stopPlaying();

        createMpTestSupporter2();
        mpTestSupporter2->getmUrl(AML_MP_RECORD_PATH);
        mpTestSupporter2->mDemuxId = AML_MP_HW_DEMUX_ID_1;
        mpTestSupporter2->setCrypto(CryptoMode);
        mpTestSupporter2->startDVRPlayback();
        waitPlaying(15 * 1000ll);
        DVRSegment(AML_MP_RECORD_PATH, true);
        stopPlaying();
        printf("----------NormalRecorderTest END----------\n");
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
        mpTestSupporter->prepare(CryptoMode);
        EXPECT_EQ(ret = mpTestSupporter->startRecord(true, true), AML_MP_OK);
        waitPlaying(20 * 1000ll);

        createMpTestSupporter2();
        mpTestSupporter2->getmUrl(AML_MP_RECORD_PATH);
        mpTestSupporter2->mDemuxId = AML_MP_HW_DEMUX_ID_1;
        mpTestSupporter2->setCrypto(CryptoMode);
        EXPECT_EQ(ret = mpTestSupporter2->startDVRPlayback(true, true), AML_MP_OK);
        waitPlaying(20 * 1000ll);
        DVRSegment(AML_MP_RECORD_PATH, true);
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
        mpTestSupporter->prepare(CryptoMode);

        if (mProgramInfo != nullptr)
        {
            mProgramInfo->videoPid = 220; //220;
            mProgramInfo->audioPid = 131;
            EXPECT_EQ(ret = mpTestSupporter->startRecord(false, false), AML_MP_OK);
            EXPECT_EQ(ret = mpTestSupporter->DVRRecorder_setStreams(), AML_MP_OK);
            EXPECT_EQ(ret = mpTestSupporter->startDVRRecorderAfterSetStreams(), AML_MP_OK);
            waitPlaying(20 * 1000ll);

            mProgramInfo->videoPid = 320; //320;
            mProgramInfo->audioPid = 132; //132;
            EXPECT_EQ(ret = mpTestSupporter->DVRRecorder_setStreams(), AML_MP_OK);
            waitPlaying(20 * 1000ll);
        }
        stopPlaying();

        createMpTestSupporter2();
        mpTestSupporter2->getmUrl(AML_MP_RECORD_PATH);
        mpTestSupporter2->mDemuxId = AML_MP_HW_DEMUX_ID_1;
        mpTestSupporter2->setCrypto(CryptoMode);
        EXPECT_EQ(ret = mpTestSupporter2->startDVRPlayback(), AML_MP_OK);
        waitPlaying(40 * 1000ll);
        //get segment list/info/delete
        DVRSegment(AML_MP_RECORD_PATH, true);

        stopPlaying();
        MLOGI("----------DynamicSetStreamTest END----------\n");
    }
}
