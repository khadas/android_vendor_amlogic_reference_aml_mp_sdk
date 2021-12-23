#ifndef _AML_MP_PLAYER_VIDEO_TEST_H_
#define _AML_MP_PLAYER_VIDEO_TEST_H_

#define LOG_TAG "AmlMpPlayerVideoTest"
#include <utils/AmlMpLog.h>
#include <utils/AmlMpUtils.h>
#include "../amlMpTestSupporter/Playback.h"
#include <gtest/gtest.h>
#include "TestUrlList.h"
#include "../amlMpTestSupporter/TestModule.h"
#include <AmlMpTestSupporter.h>
#include <pthread.h>
#include <getopt.h>
#include "source/Source.h"
#include <Aml_MP/Aml_MP.h>
#include <Aml_MP/Dvr.h>
#include <demux/AmlTsParser.h>
#include <vector>
#include "string.h"
#include <regex>
#include <iostream>
#include <thread>
#include <time.h>

std::string get_vfm_map_checkpoint();
std::string get_frame_count_checkpoint();
int amsysfs_get_sysfs_str(const char *path, char* buf, size_t len);
void check_vfm_map(int timeoutMs);
void check_frame_count(int timeoutMs);

const std::string VCOM_MAP = "vcom-map-0 { video_composer.0(1) video_render.0}";

namespace aml_mp {
static const char * mName = LOG_TAG;
static const int kWaitFirstVfameTimeOutMs = 2 * 1000ll;
static int kWaitPlayingErrorsMs = 15 * 1000ll;
static const int kWaitVideoChangedMs = 2 * 1000ll;
static const int kWaitAudioChangedMs = 2 * 1000ll;
static const int kSleepTimeMs = 0.05 * 1000ll;
static const int kCheckFrameTimeOutMs = 50 * 1000ll;
#define AML_MP_VIDEO_MODE_1 1
#define AML_MP_VIDEO_MODE_0 0
#define AML_MP_VIDEO_MODE_2 2
#define AML_MP_VIDEO_WINDOW_ZORDER_1 1
#define AML_MP_VIDEO_WINDOW_ZORDER_2 2
#define AML_MP_VIDEO_WINDOW_X 200
#define AML_MP_VIDEO_WINDOW_Y 200
#define AML_MP_VIDEO_WINDOW_HEIGHT 600
#define AML_MP_VIDEO_WINDOW_WIDTH 600
#define AML_MP_MASTER_VOLUME_70 70
#define AML_MP_SLAVE_VOLUME_30 30
#define AML_MP_MASTER_VOLUME_30 30
#define AML_MP_SLAVE_VOLUME_70 70
#define AML_MP_PID_CHANGED_DURATION 38 * 1000ll
#define AML_MP_PID_CHANGED_EVENT 2 * 1000ll
#define AML_MP_DATA_LOSS_EVENT 5 * 1000ll

struct AmlMpBase: public testing::Test
{
    void SetUp() override
    {
        MLOGI("SetUp");
    }
    void TearDown() override
    {
    }
    std::vector<StreamInfo> audioStreams;
    float volume, volume2;
    int mRet = 0;

public:
    void runTest(const std::string & url, bool isStop = true);
    void startPlaying(const std::string & url);
    void stopPlaying();
    bool waitFirstVFrameEvent(int timeoutMs = kWaitFirstVfameTimeOutMs);
    bool waitPlayingErrors(int msec = kWaitPlayingErrorsMs);
    bool waitPlaying(int msec = kWaitPlayingErrorsMs);
    bool waitVideoChangedEvent(int timeoutMs);
    bool waitAudioChangedEvent(int timeoutMs);
    bool waitAVSyncDoneEvent(int timeoutMs);
    bool waitPidChangedEvent(int timeoutMs);
    bool waitDvrRecorderStatusEvent(int timeoutMs);
    bool waitDataLossEvent(int timeoutMs);
    void eventCallback(Aml_MP_PlayerEventType event, int64_t param);
    void dvrRecorderEventCallback(AML_MP_DVRRecorderEventType event, int64_t param);
    void createMpTestSupporter(bool isPlayer = true);
    void createMpTestSupporter2();

    std::string defaultFailureMessage(const std::string & url)
    const
    {
        std::stringstream ss;
        ss << "playing " << url << " failed!";
        return ss.str();
    }
    AML_MP_PLAYER getPlayer();
    AML_MP_DVRRECORDER getRecorder();
    Aml_MP_PlayerParameterKey key;
    Aml_MP_VideoDisplayMode parameter;
    Aml_MP_AVSyncSource mSyncSource = AML_MP_AVSYNC_SOURCE_DEFAULT;
    void ADVolumeMixTest(const std::string & url, Aml_MP_PlayerParameterKey key, Aml_MP_ADVolume parameter);
    void checkHasVideo(std::string url);
    void TeletextControlTest(const std::string & url, Aml_MP_PlayerParameterKey key, AML_MP_TeletextCtrlParam parameter);
    template<typename T1, typename T2>
    void ParameterTest(const std::string & url, T1 key, T2 parameter);
    void SetVideoWindow(const std::string & url, int32_t x, int32_t y, int32_t width, int32_t height);
    void setAudioVideoParam(const std::string & url);
    void SetGetVolume(const std::string & url, float volume);
    void videoDecoding(const std::string & url, bool mStart, bool mSourceReceiver);
    void audioDecoding(const std::string & url, bool mStart, bool mSourceReceiver);
    void FCCAndPIPTest(const std::string & url, bool mPIP);
    Aml_MP_PlayerWorkMode mWorkMode = AML_MP_PLAYER_MODE_NORMAL;

protected:
    sptr <AmlMpTestSupporter> mpTestSupporter;
    sptr <AmlMpTestSupporter> mpTestSupporter2;

    std::mutex mLock;
    std::condition_variable mCond;
    bool mFirstVFrameDisplayed = false;
    bool mPlayingHaveErrors = false;
    bool mAVSyncDone = false;
    bool mPidChanged = false;
    bool mVideoChanged = false;
    bool mAudioChanged = false;
    bool mDataLoss = false;
    bool mDvrRecorderStatus = false;
    AmlMpTestSupporter::PlayMode mPlayMode = AmlMpTestSupporter::START_ALL_STOP_ALL;
};

struct AmlMpTest: AmlMpBase
{

public:
    void SetUp() override
    {
        MLOGI("SetUp 2");
        std::string name    = "AmlMpPlayerTest";
        std::list <std::string> urls;

        if (!TestUrlList::instance().getUrls(name, &urls))
        {
            return;
        }

        mUrls = urls;
    }

    void TearDown() override
    {
    }
    std::string url;

protected:
    std::list <std::string> mUrls;
};
}

#endif
