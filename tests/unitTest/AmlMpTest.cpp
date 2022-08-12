/*
 * Copyright (c) 2020 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */
#define LOG_TAG "AmlMpTest"
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
#include <utils/Amlsysfsutils.h>

using namespace aml_mp;

std::string get_vfm_map_checkpoint()
{
    char buf[1024];
    if (amsysfs_get_sysfs_str("/sys/class/vfm/map", buf, sizeof(buf)) < 0)
    {
        return "";
    }
    return std::string(buf);
}

std::string get_frame_count_checkpoint()
{
    char buf[1024];
    if (amsysfs_get_sysfs_str("/sys/class/video_composer/receive_count", buf, sizeof(buf)) < 0)
    {
        return "";
    }
    return std::string(buf);
}

void check_frame_count(int timeoutMs __unused)
{
    std::string frame = "1";
    std::string frame_temp = "1";
    int i = AML_MP_OK;
    while (i < kCheckFrameTimeOutMs)
    {
        sleep(kSleepTimeMs);
        frame = get_frame_count_checkpoint();
        MLOGI("frame is %s", frame.c_str());
        MLOGI("frame_temp is %s", frame_temp.c_str());
        if (frame > frame_temp)
        {
            EXPECT_TRUE(frame > frame_temp);
            frame_temp = frame;
        }else{
            EXPECT_FALSE(frame > frame_temp);
        }
        i++;
    }
}

void check_vfm_map(int timeoutMs)
{
    std::string ret;
    clock_t t;
    t = clock();
    while (clock() - t < timeoutMs)
    {
        ret = get_vfm_map_checkpoint();
        EXPECT_TRUE(ret.find(VCOM_MAP) != std::string::npos);
        std::smatch m;
        std::regex vdec_map_pattern ("^.* \\[\\d+\\]\\s+vdec-map-1 \\{\\s+vdec\\.\\w+\\.\\d+\\(1\\) dimulti\\.1\\(1\\) v4lvideo\\.0\\} .*$");
        EXPECT_EQ(std::regex_search (ret, m, vdec_map_pattern), AML_MP_OK);
    }
}

void AmlMpBase::createMpTestSupporter(bool isPlayer)
{
    if (mpTestSupporter == nullptr)
    {
        mpTestSupporter = new AmlMpTestSupporter;
        if (isPlayer) {
            mpTestSupporter->playerRegisterEventCallback([] (void * userData, Aml_MP_PlayerEventType event, int64_t param){ AmlMpBase * self = (AmlMpBase *) userData; return self->playereventCallback(event, param); }, this);
        } else {
            mpTestSupporter->DVRRecorderRegisterEventCallback([] (void * userData, AML_MP_DVRRecorderEventType event, int64_t param){ AmlMpBase * self = (AmlMpBase *) userData; return self->dvrRecorderEventCallback(event, param); }, this);
        }
    }
}

void AmlMpBase::createMpTestSupporter2(bool isPlayer)
{
    if (mpTestSupporter2 == nullptr)
    {
        mpTestSupporter2 = new AmlMpTestSupporter;
        if (isPlayer) {
            mpTestSupporter2->playerRegisterEventCallback([] (void * userData, Aml_MP_PlayerEventType event, int64_t param){ AmlMpBase * self = (AmlMpBase *) userData; return self->playereventCallback(event, param); }, this);
        } else {
            mpTestSupporter2->DVRRecorderRegisterEventCallback([] (void * userData, AML_MP_DVRRecorderEventType event, int64_t param){ AmlMpBase * self = (AmlMpBase *) userData; return self->dvrRecorderEventCallback(event, param); }, this);
        }
    }
}

void AmlMpBase::checkHasVideo(std::string url)
{
    if (mpTestSupporter->hasVideo())
    {
        EXPECT_TRUE(waitFirstVFrameEvent()) << defaultFailureMessage(url);
        EXPECT_TRUE(waitVideoChangedEvent(kWaitVideoChangedMs));
    }
}

void AmlMpBase::SetVideoWindow(const std::string & url, int32_t x, int32_t y, int32_t width, int32_t height)
{
    runTest(url, false);
    if (mRet == 0)
    {
        void *player = getPlayer();
        EXPECT_EQ(Aml_MP_Player_SetVideoWindow(player, x, y, width, height), AML_MP_OK);
        EXPECT_FALSE(waitPlayingErrors());
    }
    MLOGI("----------ParameterTest END----------\n");
    stopPlaying();
}

void AmlMpBase::runTest(const std::string & url, bool isStop)
{
    startPlaying(url);
    checkHasVideo(url);
    EXPECT_FALSE(waitPlayingErrors());
    if (isStop) {
        stopPlaying();
    }
}

void AmlMpBase::startPlaying(const std::string & url)
{
    MLOGI("url:%s", url.c_str());
    mRet = 0;
    createMpTestSupporter();
    mpTestSupporter->setDataSource(url);
    if (CryptoMode)
    {
        mRet = mpTestSupporter->prepare(CryptoMode);
    } else {
        mRet = mpTestSupporter->prepare();
    }

    if (mRet != 0)
    {
        return;
    }
    //EXPECT_EQ(ret, 0) << defaultFailureMessage(url);
    int ret = mpTestSupporter->startPlay();
    EXPECT_EQ(ret, 0) << defaultFailureMessage(url);
    if (ret != 0)
    {
        mPlayingHaveErrors = true;
    }
}

void AmlMpBase::stopPlaying()
{
    mpTestSupporter->stop();
    if (mpTestSupporter2 != nullptr) {
        mpTestSupporter2->stop();
    }
    mFirstVFrameDisplayed = false;
    mPlayingHaveErrors = false;
}

bool AmlMpBase::waitFirstVFrameEvent(int timeoutMs)
{
    std::unique_lock <std::mutex> _l(mLock);
    return mCond.wait_for(_l, std::chrono::milliseconds(timeoutMs), [this] () { return mFirstVFrameDisplayed; });
}

bool AmlMpBase::waitVideoChangedEvent(int timeoutMs)
{
    std::unique_lock <std::mutex> _l(mLock);
    return mCond.wait_for(_l, std::chrono::milliseconds(timeoutMs), [this] () { return mVideoChanged; });
}

bool AmlMpBase::waitDataLossEvent(int timeoutMs)
{
    std::unique_lock <std::mutex> _l(mLock);
    return mCond.wait_for(_l, std::chrono::milliseconds(timeoutMs), [this] () { return mDataLoss; });
}

bool AmlMpBase::waitAVSyncDoneEvent(int timeoutMs)
{
    std::unique_lock <std::mutex> _l(mLock);
    return mCond.wait_for(_l, std::chrono::milliseconds(timeoutMs), [this] () { return mAVSyncDone; });
}

bool AmlMpBase::waitPidChangedEvent(int timeoutMs)
{
    std::unique_lock <std::mutex> _l(mLock);
    return mCond.wait_for(_l, std::chrono::milliseconds(timeoutMs), [this] () { return mPidChanged; });
}

bool AmlMpBase::waitDvrRecorderStatusEvent(int timeoutMs)
{
    std::unique_lock <std::mutex> _l(mLock);
    return mCond.wait_for(_l, std::chrono::milliseconds(timeoutMs), [this] () { return mDvrRecorderStatus; });
}

bool AmlMpBase::waitDvrRecorderErrorEvent(int timeoutMs)
{
    std::unique_lock <std::mutex> _l(mLock);
    return mCond.wait_for(_l, std::chrono::milliseconds(timeoutMs), [this] () { return mDvrRecorderError; });
}

bool AmlMpBase::waitDvrRecorderSyncEndEvent(int timeoutMs)
{
    std::unique_lock <std::mutex> _l(mLock);
    return mCond.wait_for(_l, std::chrono::milliseconds(timeoutMs), [this] () { return mDvrRecorderSyncEnd; });
}

bool AmlMpBase::waitDvrRecorderCryptoStatusEvent(int timeoutMs)
{
    std::unique_lock <std::mutex> _l(mLock);
    return mCond.wait_for(_l, std::chrono::milliseconds(timeoutMs), [this] () { return mDvrRecorderCryptoStatus; });
}

bool AmlMpBase::waitDvrRecorderWriteErrorEvent(int timeoutMs)
{
    std::unique_lock <std::mutex> _l(mLock);
    return mCond.wait_for(_l, std::chrono::milliseconds(timeoutMs), [this] () { return mDvrRecorderWriteError; });
}

bool AmlMpBase::waitPlayingErrors(int timeoutMs)
{
    std::unique_lock <std::mutex> _l(mLock);
    std::thread checkvfmmap (check_vfm_map, timeoutMs);
    std::thread checkframecount (check_frame_count, timeoutMs);
    checkvfmmap.detach();
    checkframecount.detach();
    return mCond.wait_for(_l, std::chrono::milliseconds(timeoutMs), [this] () { return mPlayingHaveErrors; });
}

bool AmlMpBase::waitPlaying(int timeoutMs)
{
    std::unique_lock <std::mutex> _l(mLock);
    return mCond.wait_for(_l, std::chrono::milliseconds(timeoutMs), [this] () { return mPlayingHaveErrors; });
}

void AmlMpBase::playereventCallback(Aml_MP_PlayerEventType event, int64_t param __unused)
{
    std::unique_lock <std::mutex> _l(mLock);
    switch (event)
    {
        case AML_MP_PLAYER_EVENT_FIRST_FRAME:
        {
            mFirstVFrameDisplayed = true;
            mCond.notify_all();
            break;
        }
        case AML_MP_PLAYER_EVENT_VIDEO_CHANGED:
        {
            mVideoChanged = true;
            MLOGI("video changed-----");
            mCond.notify_all();
            break;
        }
        case AML_MP_PLAYER_EVENT_AUDIO_CHANGED:
        {
            mAudioChanged = true;
            MLOGI("audio changed-----");
            mCond.notify_all();
            break;
        }
        case AML_MP_PLAYER_EVENT_DATA_LOSS:
        {
            mDataLoss = true;
            MLOGI("data loss-----");
            mCond.notify_all();
            break;
        }
        case AML_MP_PLAYER_EVENT_AV_SYNC_DONE:
        {
            mAVSyncDone = true;
            MLOGI("av sync done-----");
            mCond.notify_all();
            break;
        }
        case AML_MP_PLAYER_EVENT_PID_CHANGED:
        {
            mPidChanged = true;
            MLOGI("pid changed-----");
            mCond.notify_all();
            break;
        }
        default:
            break;
    }
}

AML_MP_PLAYER AmlMpBase::getPlayer()
{
    sptr <TestModule> testModule = mpTestSupporter->getPlayback();
    AML_MP_PLAYER player = testModule->getCommandHandle();
    return player;
}

void AmlMpBase::dvrRecorderEventCallback(AML_MP_DVRRecorderEventType event, int64_t param __unused)
{
    std::unique_lock <std::mutex> _l(mLock);
    switch (event)
    {
        case AML_MP_DVRRECORDER_EVENT_STATUS:
        {
            mDvrRecorderStatus = true;
            MLOGI("dvr recorder event status triggered-----");
            mCond.notify_all();
            break;
        }
        case AML_MP_DVRRECORDER_EVENT_ERROR:
        {
            mDvrRecorderError = true;
            MLOGI("dvr recorder event error triggered-----");
            mCond.notify_all();
            break;
        }
        case AML_MP_DVRRECORDER_EVENT_SYNC_END:
        {
            mDvrRecorderSyncEnd = true;
            MLOGI("dvr recorder event sync end triggered-----");
            mCond.notify_all();
            break;
        }
        case AML_MP_DVRRECORDER_EVENT_CRYPTO_STATUS:
        {
            mDvrRecorderCryptoStatus = true;
            mCond.notify_all();
            break;
        }
        case AML_MP_DVRRECORDER_EVENT_WRITE_ERROR:
        {
            mDvrRecorderWriteError = true;
            MLOGI("dvr recorder event write error triggered-----");
            mCond.notify_all();
            break;
        }
        default:
            break;
    }
}

AML_MP_DVRRECORDER AmlMpBase::getRecorder()
{
    sptr <TestModule> testModule = mpTestSupporter->getRecorder();
    AML_MP_DVRRECORDER recorder = testModule->getCommandHandle();
    return recorder;
}

void AmlMpBase::DVRSegment(std::string url, bool isDelete)
{
    uint32_t segmentNums;
    uint64_t* segmentIds = nullptr;
    int segmentIndex = 0;
    Aml_MP_DVRSegmentInfo segmentInfo;
    url = mpTestSupporter2->stripUrlIfNeeded(url);
    EXPECT_EQ(Aml_MP_DVRRecorder_GetSegmentList(url.c_str(), &segmentNums, &segmentIds), AML_MP_OK);
    EXPECT_EQ(Aml_MP_DVRRecorder_GetSegmentInfo(url.c_str(), segmentIds[segmentIndex], &segmentInfo), AML_MP_OK);
    if (isDelete) {
        EXPECT_EQ(Aml_MP_DVRRecorder_DeleteSegment(url.c_str(), segmentIds[segmentIndex]), AML_MP_OK);
    }
}
