/*
 * Copyright (c) 2020 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#ifndef _AML_MP_TEST_SUPPORTER_H_
#define _AML_MP_TEST_SUPPORTER_H_

#include <utils/AmlMpRefBase.h>
#include <utils/AmlMpHandle.h>
#include <utils/AmlMpUtils.h>
#include <string>
#include <thread>
#include <vector>
#include <Aml_MP/Aml_MP.h>
#include "TestUtils.h"
#include <mutex>
#include <Aml_MP/Dvr.h>

namespace aml_mp {
class Source;
class Parser;
class TestModule;
class ParserReceiver;
class CasPlugin;
class Playback;
class DVRRecord;
class DVRPlayback;
struct ProgramInfo;
struct NativeUI;
struct CommandProcessor;

class AmlMpTestSupporter : public AmlMpHandle
{
public:
    enum PlayMode {
        START_ALL_STOP_ALL,
        START_ALL_STOP_SEPARATELY,
        START_SEPARATELY_STOP_ALL,
        START_SEPARATELY_STOP_SEPARATELY,
        START_SEPARATELY_STOP_SEPARATELY_V2,
        START_AUDIO_START_VIDEO,
        START_VIDEO_START_AUDIO,
        PLAY_MODE_MAX,
    };

    struct DisplayParam {
        int x                   = 0;
        int y                   = 0;
        int width               = -1;
        int height              = -1;
        int zorder              = 0;
        int videoMode           = 0;
        ANativeWindow* aNativeWindow = nullptr;
        int channelId           = -1;
    };

    AmlMpTestSupporter();
    ~AmlMpTestSupporter();
    Aml_MP_PlayerWorkMode mWorkMode = AML_MP_PLAYER_MODE_NORMAL;
    void playerRegisterEventCallback(Aml_MP_PlayerEventCallback cb, void* userData);
    void DVRRecorderRegisterEventCallback(Aml_MP_DVRRecorderEventCallback cb, void* userData);
    int setDataSource(const std::string& url);
    int prepare(bool cryptoMode = false);
    void setDisplayParam(const DisplayParam& displayParam);
    void addOptions(uint64_t options);
    void setVideoErrorRecoveryMode(int recoveryMode);
    void setSourceMode(bool esMode, bool clearTVP);
    int startPlay(PlayMode playMode = START_ALL_STOP_ALL, bool mStart = true, bool mSourceReceiver = true);
    int startRecord(bool isSetStreams = true, bool isTimeShift = false);
    int startDVRPlayback(bool isSetStreams=true, bool isTimeShift=false);
    int DVRRecorder_setStreams();
    int DVRPlayback_setStreams();
    int startDVRRecorderAfterSetStreams();
    int startDVRPlaybackAfterSetStreams();
    std::string stripUrlIfNeeded(const std::string& url) const;

    int startUIOnly();
    int stop();
    int setAVSyncSource(Aml_MP_AVSyncSource syncSource);
    int setPcrPid(int pid);

    bool hasVideo() const;

    int installSignalHandler();
    int fetchAndProcessCommands();
    int setOsdBlank(int blank);

    sptr<TestModule> getPlayback() const;
    sptr<TestModule> getRecorder() const;
    sptr<TestModule> getDVRPlayer() const;

    sptr<NativeUI> createNativeUI();
#ifdef ANDROID
    sp<ANativeWindow> getSurfaceControl();
    void setWindow(bool mSurface = true);
#endif
    sptr<ProgramInfo> getProgramInfo() const;
    int setParameter(Aml_MP_PlayerParameterKey key, void* parameter);

    Aml_MP_ChannelId mChannelId = AML_MP_CHANNEL_ID_MAIN;
    Aml_MP_DemuxId mDemuxId = AML_MP_HW_DEMUX_ID_0;

private:
    //int startDVRPlayback();
    bool processCommand(const std::vector<std::string>& args);
    void signalQuit();

    std::string mUrl;
    sptr<Source> mSource;
    sptr<Parser> mParser;
    sptr<ParserReceiver> mParserReceiver;
    sptr<ProgramInfo> mProgramInfo;

    sptr<TestModule> mTestModule;

    sptr<CasPlugin> mCasPlugin;
    sptr<Playback> mPlayback;
    sptr<DVRRecord> mRecorder;
    sptr<DVRPlayback> mDVRPlayback;
    Aml_MP_PlayerEventCallback mEventCallback = nullptr;
    Aml_MP_DVRRecorderEventCallback mDVRRecorderEventCallback = nullptr;
    void* mUserData = nullptr;

    bool mIsDVRPlayback = false;
    bool mCryptoMode = false;
    uint64_t mOptions = 0;
    sptr<NativeUI> mNativeUI;
    NativeWindowHelper mNativeWindowHelper;
    std::thread mSignalHandleThread;
    sptr<CommandProcessor> mCommandProcessor;

    bool mQuitPending = false;

    PlayMode mPlayMode = START_ALL_STOP_ALL;
    //AML_MP_PLAYER mPlayer = AML_MP_INVALID_HANDLE;

    DisplayParam mDisplayParam;
    mutable std::mutex mLock;
    Aml_MP_AVSyncSource mSyncSource = AML_MP_AVSYNC_SOURCE_DEFAULT;
    int mPcrPid = AML_MP_INVALID_PID;

    int mpVideoErrorRecoveryMode = 0;
    bool mEsMode = false;
    bool mClearTVP = false;

    AmlMpTestSupporter(const AmlMpTestSupporter&) = delete;
    AmlMpTestSupporter& operator=(const AmlMpTestSupporter&) = delete;
};

}
#endif
