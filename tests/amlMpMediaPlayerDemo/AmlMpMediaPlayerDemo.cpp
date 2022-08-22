/*
 * Copyright (c) 2020 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#define LOG_NDEBUG 0
#define LOG_TAG "AmlMpMediaPlayerDemo"
#include <utils/AmlMpLog.h>
#include <unistd.h>
#include <getopt.h>
#include "TestUtils.h"
#include <Aml_MP/Aml_MediaPlayer.h>
#include <string>
#include <vector>
#include <inttypes.h>
#include <signal.h>
#include <thread>
#include "../amlMpTestSupporter/TestModule.h"
#include <../../mediaplayer/AmlMediaPlayerBase.h>
static const char* mName = LOG_TAG;
using namespace aml_mp;

struct AmlMpMediaPlayerDemo : public TestModule
{
        AmlMpMediaPlayerDemo();
        ~AmlMpMediaPlayerDemo();

        int fetchAndProcessCommands();
        virtual void* getCommandHandle() const;
        void setCommandHandle(void * handle);
        void waitPreparedEvent();
        void broadcast();
        int installSignalHandler();
public:
        AML_MP_MEDIAPLAYER mPlayer = NULL;
        bool mQuitPending = false;
        std::condition_variable mCondition;
        std::mutex mMutex;

protected:
        const TestModule::Command* getCommandTable() const;
        bool processCommand(const std::vector<std::string>& args);

private:
        sptr<CommandProcessor> mCommandProcessor;
        std::thread mSignalHandleThread;
};

AmlMpMediaPlayerDemo::AmlMpMediaPlayerDemo()
{
    MLOGI();
}

AmlMpMediaPlayerDemo::~AmlMpMediaPlayerDemo()
{
    MLOGI();
}

int AmlMpMediaPlayerDemo::fetchAndProcessCommands()
{
    char promptBuf[50];
    snprintf(promptBuf, sizeof(promptBuf), "AmlMpMediaPlayerDemo >");

    mCommandProcessor = new CommandProcessor(promptBuf);
    if (mCommandProcessor == nullptr) {
        return -1;
    }

    mCommandProcessor->setCommandVisitor(std::bind(&AmlMpMediaPlayerDemo::processCommand, this, std::placeholders::_1));
    mCommandProcessor->setInterrupter([this]() {return mQuitPending;});
    return mCommandProcessor->fetchAndProcessCommands();
}

bool AmlMpMediaPlayerDemo::processCommand(const std::vector<std::string>& args)
{
    bool ret = true;
    std::string cmd = *args.begin();

    if (cmd == "osd") {
        MLOGI("%s,%d cmd==osd", __FUNCTION__, __LINE__);

    } else if (cmd == "video") {
        MLOGI("%s,%d cmd==video", __FUNCTION__, __LINE__);

    } else if (cmd == "zorder") {
        MLOGI("%s,%d cmd==zorder", __FUNCTION__, __LINE__);

    } else if (cmd == "resize") {
        MLOGI("%s,%d cmd==resize", __FUNCTION__, __LINE__);

    }  else if (cmd == "quit") {
        MLOGI("%s,%d set mQuitPending = true", __FUNCTION__, __LINE__);
        mQuitPending = true;

    } else {
        TestModule::processCommand(args);
    }

    return ret;
}

///////////////////////////////////////////////////////////////////////////////
static void printTrackInfo(Aml_MP_TrackInfo *trackInfo)
{
    if (trackInfo == NULL)
        return;

    int index = 0;
    Aml_MP_TrackInfo *info = trackInfo;

    printf("nb_streams:%d \n", info->nb_streams);
    for (index = 0; index < info->nb_streams; ++index) {
        if (info->streamInfo[index].streamType == AML_MP_STREAM_TYPE_VIDEO) {
            printf("index:%d [video] id:%d, videoCodec:%d, width:%d, height:%d, mine:%s\n", index
                ,info->streamInfo[index].vInfo.id
                ,info->streamInfo[index].vInfo.videoCodec
                ,info->streamInfo[index].vInfo.width
                ,info->streamInfo[index].vInfo.height
                ,info->streamInfo[index].vInfo.mine);
        } else if (info->streamInfo[index].streamType == AML_MP_STREAM_TYPE_AUDIO) {
            printf("index:%d [audio] id:%d, audioCodec:%d, nChannels:%d, nSampleRate:%d, mine:%s, language:%s\n", index
                ,info->streamInfo[index].aInfo.id
                ,info->streamInfo[index].aInfo.audioCodec
                ,info->streamInfo[index].aInfo.nChannels
                ,info->streamInfo[index].aInfo.nSampleRate
                ,info->streamInfo[index].aInfo.mine
                ,info->streamInfo[index].aInfo.language);
        } else if (info->streamInfo[index].streamType == AML_MP_STREAM_TYPE_SUBTITLE) {
            printf("index:%d [subtitle] id:%d, subtitleCodec:%d, mine:%s, language:%s\n", index
                ,info->streamInfo[index].sInfo.id
                ,info->streamInfo[index].sInfo.subtitleCodec
                ,info->streamInfo[index].sInfo.mine
                ,info->streamInfo[index].sInfo.language);
        } else {
            ;
        }
    }
}

static void printMediaInfo(Aml_MP_MediaInfo *mediaInfo)
{
    if (mediaInfo == NULL)
        return;

    int index = 0;
    Aml_MP_MediaInfo *info = mediaInfo;

    printf("[mediaInfo] filename:%s, duration:%" PRId64 ", file_size:%" PRId64 ", bitrate:%" PRId64 ", nb_streams:%d\n"
            ,info->filename
            ,info->duration
            ,info->file_size
            ,info->bitrate
            ,info->nb_streams);
}


static struct TestModule::Command g_commandTable[] = {
    {
        "help", 0, "help",
        [](AML_MP_MEDIAPLAYER player __unused, const std::vector<std::string>& args __unused) -> int {
            TestModule::printCommands(g_commandTable, true);
            return 0;
        }
    },

    {
        "pause", 0, "pause player",
        [](AML_MP_MEDIAPLAYER player, const std::vector<std::string>& args __unused) -> int {
            int ret = Aml_MP_MediaPlayer_Pause(player);
            printf("call pause player, ret:%d\n", ret);
            return ret;
        }
    },

    {
        "resume", 0, "resume player",
        [](AML_MP_MEDIAPLAYER player, const std::vector<std::string>& args __unused) -> int {
            int ret = Aml_MP_MediaPlayer_Resume(player);
            printf("call resume player, ret:%d\n", ret);
            return ret;
        }
    },
/*
    {
        "hide", 0, "hide video",
        [](AML_MP_MEDIAPLAYER player, const std::vector<std::string>& args __unused) -> int {
            printf("call hide video, not finished\n");
            return Aml_MP_MediaPlayer_HideVideo(player);
        }
    },

    {
        "show", 0, "show video",
        [](AML_MP_MEDIAPLAYER player, const std::vector<std::string>& args __unused) -> int {
            printf("call show video, not finished\n");
            return Aml_MP_MediaPlayer_ShowVideo(player);
        }
    },
*/
    {
        "seek", 1, "call seek",
        [](AML_MP_MEDIAPLAYER player, const std::vector<std::string>& args __unused) -> int {
            int ret = 0;
            int msec = atoi(args.at(1).c_str());
            ret = Aml_MP_MediaPlayer_SeekTo(player, msec);
            printf("call seek, msec:%d, size:%zu, ret:%d\n", msec, args.size(), ret);
            return ret;
        }
    },

    {
        "selectTrack", 1, "select track",
        [](AML_MP_MEDIAPLAYER player, const std::vector<std::string>& args __unused) -> int {
            int ret = 0;
            Aml_MP_MediaPlayerInvokeRequest request;
            Aml_MP_MediaPlayerInvokeReply reply;
            memset(&request, 0, sizeof(Aml_MP_MediaPlayerInvokeRequest));
            memset(&reply, 0, sizeof(Aml_MP_MediaPlayerInvokeReply));

            request.requestId = AML_MP_MEDIAPLAYER_INVOKE_ID_SELECT_TRACK;
            request.u.data32 = atoi(args.at(1).c_str());
            ret = Aml_MP_MediaPlayer_Invoke(player, &request, &reply);
            printf("call selectTrack, index:%d, ret:%d\n", request.u.data32, ret);
            return ret;
        }
    },

    {
        "unselectTrack", 1, "unselect track",
        [](AML_MP_MEDIAPLAYER player, const std::vector<std::string>& args __unused) -> int {
            int ret = 0;
            Aml_MP_MediaPlayerInvokeRequest request;
            Aml_MP_MediaPlayerInvokeReply reply;
            memset(&request, 0, sizeof(Aml_MP_MediaPlayerInvokeRequest));
            memset(&reply, 0, sizeof(Aml_MP_MediaPlayerInvokeReply));

            request.requestId = AML_MP_MEDIAPLAYER_INVOKE_ID_UNSELECT_TRACK;
            request.u.data32 = atoi(args.at(1).c_str());
            ret = Aml_MP_MediaPlayer_Invoke(player, &request, &reply);
            printf("call unselect track, index:%d, ret:%d\n", request.u.data32, ret);
            return ret;
        }
    },

/*
    {
        "gVolume", 0, "get volume",
        [](AML_MP_MEDIAPLAYER player, const std::vector<std::string>& args __unused) -> int {
            float volume;
            int ret = Aml_MP_MediaPlayer_GetVolume(player, &volume);
            printf("Current volume: %f, ret: %d\n", volume, ret);
            return ret;
        }
    },

    {
        "sVolume", 1, "set volume",
        [](AML_MP_MEDIAPLAYER player, const std::vector<std::string>& args __unused) -> int {
            float volume, volume2;
            if (args.size() != 2) {
                printf("Input example: sVolume volume\n");
                return -1;
            }
            printf("String input: %s\n", args[1].data());
            volume = stof(args[1]);
            int ret = Aml_MP_MediaPlayer_SetVolume(player,volume);
            ret = Aml_MP_MediaPlayer_GetVolume(player, &volume2);
            printf("Get volume: %f, set volume: %f, ret: %d\n", volume2, volume, ret);
            return ret;
        }
    },
*/

    {
        "sFast", 1, "set Fast rate",
        [](AML_MP_MEDIAPLAYER player, const std::vector<std::string>& args __unused) -> int {
            int ret = 0;
            float x = atof(args.at(1).c_str());
            ret = Aml_MP_MediaPlayer_SetPlaybackRate(player, x);
            printf("call set Fast rate, speed:%f, ret:%d\n", x, ret);
            return ret;
        }
    },

    {
        "sWindow", 4, "set video window",
        [](AML_MP_MEDIAPLAYER player, const std::vector<std::string>& args __unused) -> int {
            int ret = 0;
            int x = atoi(args.at(1).c_str());
            int y = atoi(args.at(2).c_str());
            int width = atoi(args.at(3).c_str());
            int height = atoi(args.at(4).c_str());
            ret = Aml_MP_MediaPlayer_SetVideoWindow(player, x, y, width, height);
            printf("call set video window, x:%d, y:%d, width:%d, height:%d, ret:%d\n",x, y, width, height, ret);
            return ret;
        }
    },

/*
    {
        "sZorder", 0, "set zorder",
        [](AML_MP_MEDIAPLAYER player, const std::vector<std::string>& args __unused) -> int {
            printf("call set zorder, not finished\n");
            return -1;
        }
    },
*/
    {
        "sOnlyHint", 1, "set only hint",
        [](AML_MP_MEDIAPLAYER player, const std::vector<std::string>& args __unused) -> int {
            int ret = 0;
            Aml_MP_MediaPlayerOnlyHintType type = (Aml_MP_MediaPlayerOnlyHintType)atoi(args.at(1).c_str());

            ret = Aml_MP_MediaPlayer_SetParameter(player, AML_MP_MEDIAPLAYER_PARAMETER_ONLYHINT_TYPE, (void*)&type);
            printf("AML_MP_MEDIAPLAYER_PARAMETER_ONLYHINT_TYPE set type: %d, ret: %d\n", type, ret);

            return ret;
        }
    },
/*
    {
        "sParam", 0, "set param",
        [](AML_MP_MEDIAPLAYER player, const std::vector<std::string>& args __unused) -> int {
            int ret = 0;

            printf("set param ret: %d\n", ret);
            return ret;
        }
    },

    {
        "gParam", 0, "get param",
        [](AML_MP_MEDIAPLAYER player, const std::vector<std::string>& args __unused) -> int {
            int ret = 0;

            printf("get param ret: %d\n", ret);
            return ret;
        }
    },

    {
        "sSync", 0, "set sync mode",
        [](AML_MP_MEDIAPLAYER player, const std::vector<std::string>& args __unused) -> int {
            printf("call set sync mode, not finished\n");
            return -1;
        }
    },
*/

    {
        "sMute", 1, "set mute umute",
        [](AML_MP_MEDIAPLAYER player, const std::vector<std::string>& args __unused) -> int {
            bool mute;
            if (args.size() != 2) {
                printf("Input example: sMute mute/umute\n");
                return -1;
            }
            printf("String input: %s\n", args[1].data());
            mute = stof(args[1]);
            int ret = Aml_MP_MediaPlayer_SetMute(player, mute);
            printf("Set mute: %d, ret: %d\n", mute, ret);
            return ret;
        }
    },

    {
        "gPos", 0, "get current position",
        [](AML_MP_MEDIAPLAYER player, const std::vector<std::string>& args __unused) -> int {
            int ret = 0, position = 0;
            ret = Aml_MP_MediaPlayer_GetCurrentPosition(player, &position);
            printf("Aml_MP_MediaPlayer_GetCurrentPosition get position: %d(ms), ret: %d\n", position, ret);

            return ret;
        }
    },

    {
        "gDuration", 0, "get duration",
        [](AML_MP_MEDIAPLAYER player, const std::vector<std::string>& args __unused) -> int {
            int ret = 0, duration = 0;
            ret = Aml_MP_MediaPlayer_GetDuration(player, &duration);
            printf("Aml_MP_MediaPlayer_GetDuration get duration: %d(ms), ret: %d\n", duration, ret);

            return ret;
        }
    },
/*
    {
        "gIsPlaying", 0, "get is playing",
        [](AML_MP_MEDIAPLAYER player, const std::vector<std::string>& args __unused) -> int {
            bool isPlaying = Aml_MP_MediaPlayer_IsPlaying(player);
            printf("Aml_MP_MediaPlayer_IsPlaying get isPlaying: %d, ret: %d\n", isPlaying, 0);

            return 0;
        }
    },
*/

    {
        "invoke", 0, "invoke generic method",
        [](AML_MP_MEDIAPLAYER player, const std::vector<std::string>& args __unused) -> int {
            int ret = 0;
            Aml_MP_MediaPlayerInvokeRequest request;
            Aml_MP_MediaPlayerInvokeReply reply;


            memset(&request, 0, sizeof(Aml_MP_MediaPlayerInvokeRequest));
            memset(&reply, 0, sizeof(Aml_MP_MediaPlayerInvokeReply));
            request.requestId = AML_MP_MEDIAPLAYER_INVOKE_ID_GET_TRACK_INFO;
            ret = Aml_MP_MediaPlayer_Invoke(player, &request, &reply);
            printf("AML_MP_MEDIAPLAYER_INVOKE_ID_GET_TRACK_INFO, reply id:%x ret: %d\n",reply.requestId, ret);
            printTrackInfo(&reply.u.trackInfo);


            memset(&request, 0, sizeof(Aml_MP_MediaPlayerInvokeRequest));
            memset(&reply, 0, sizeof(Aml_MP_MediaPlayerInvokeReply));
            request.requestId = AML_MP_MEDIAPLAYER_INVOKE_ID_GET_MEDIA_INFO;
            ret = Aml_MP_MediaPlayer_Invoke(player, &request, &reply);
            printf("\nAML_MP_MEDIAPLAYER_INVOKE_ID_GET_MEDIA_INFO,reply id:%x ret: %d\n",reply.requestId, ret);
            printMediaInfo(&reply.u.mediaInfo);

            return ret;
        }
    },

    {
        "reset", 0, "call reset",
        [](AML_MP_MEDIAPLAYER player, const std::vector<std::string>& args __unused) -> int {
            int ret = Aml_MP_MediaPlayer_Reset(player);
            printf("call reset ret: %d\n", ret);
            return ret;
        }
    },

    {
        "sDatasource", 1, "set data source",
        [](AML_MP_MEDIAPLAYER player, const std::vector<std::string>& args __unused) -> int {
            const char *url = args.at(1).c_str();
            int ret = Aml_MP_MediaPlayer_SetDataSource(player, url);
            printf("call set data source:%s ret: %d\n", url, ret);
            return ret;
        }
    },

    {
        "prepare", 0, "call prepare",
        [](AML_MP_MEDIAPLAYER player, const std::vector<std::string>& args __unused) -> int {
            int ret = Aml_MP_MediaPlayer_Prepare(player);
            printf("call prepare ret: %d\n", ret);
            return ret;
        }
    },

    {
        "start", 0, "call start",
        [](AML_MP_MEDIAPLAYER player, const std::vector<std::string>& args __unused) -> int {
            int ret = Aml_MP_MediaPlayer_Start(player);
            printf("call start ret: %d\n", ret);
            return ret;
        }
    },

    {
        "stop", 0, "call stop",
        [](AML_MP_MEDIAPLAYER player, const std::vector<std::string>& args __unused) -> int {
            int ret = Aml_MP_MediaPlayer_Stop(player);
            printf("call stop ret: %d\n", ret);
            return ret;
        }
    },

    {
        "quit", 0, "exit playback",
        [](AML_MP_MEDIAPLAYER player, const std::vector<std::string>& args __unused) -> int {
            printf("exit playback\n");
            return 0;
        }
    },

    {nullptr, 0, nullptr, nullptr}
};

const TestModule::Command* AmlMpMediaPlayerDemo::getCommandTable() const
{
    return g_commandTable;
}

void* AmlMpMediaPlayerDemo::getCommandHandle() const
{
    return mPlayer;
}

void AmlMpMediaPlayerDemo::setCommandHandle(void * handle)
{
    if (handle != NULL)
        mPlayer = handle;
}

void AmlMpMediaPlayerDemo::waitPreparedEvent()
{
    printf("waitPreparedEvent \n");

    std::unique_lock<std::mutex> autoLock(mMutex);
    mCondition.wait(autoLock);
}

void AmlMpMediaPlayerDemo::broadcast()
{
    printf("broadcast \n");
    std::unique_lock<std::mutex> autoLock(mMutex);
    mCondition.notify_all();
}

int AmlMpMediaPlayerDemo::installSignalHandler()
{
    int ret = 0;
    sigset_t blockSet, oldMask;
    sigemptyset(&blockSet);
    sigaddset(&blockSet, SIGINT);
    ret = pthread_sigmask(SIG_SETMASK, &blockSet, &oldMask);
    if (ret != 0) {
        MLOGE("pthread_sigmask failed! %d", ret);
        return -1;
    }

    mSignalHandleThread = std::thread([blockSet, oldMask, this] {
        int signo;
        int err;

        for (;;) {
            err = sigwait(&blockSet, &signo);
            if (err != 0) {
                MLOGE("sigwait error! %d", err);
                exit(0);
            }

            printf("%s\n", strsignal(signo));

            switch (signo) {
            case SIGINT:
            {
                //quit
                mQuitPending = true;
            }
            break;

            default:
                break;
            }

            if (pthread_sigmask(SIG_SETMASK, &oldMask, nullptr) != 0) {
                MLOGE("restore sigmask failed!");
            }
        }
    });
    mSignalHandleThread.detach();

    return ret;
}

struct Argument
{
    std::string url;
    int x = 0;
    int y = 0;
    int viewWidth = -1;
    int viewHeight = -1;
    int channelId = -1;
    bool onlyVideo = false;
    bool onlyAudio = false;
};

static int parseCommandArgs(int argc, char* argv[], Argument* argument)
{
    static const struct option longopts[] = {
        {"help",        no_argument,        nullptr, 'h'},
        {"size",        required_argument,  nullptr, 's'},
        {"pos",         required_argument,  nullptr, 'p'},
        {"channelId",   required_argument,  nullptr, 'c'},
        {"onlyVideo",   no_argument,        nullptr, 'V'},
        {"onlyAudio",   no_argument,        nullptr, 'A'},
        {nullptr,       no_argument,        nullptr, 0},
    };

    int opt, longindex;
    while ((opt = getopt_long(argc, argv, "", longopts, &longindex)) != -1) {
        //printf("opt = %d\n", opt);
        switch (opt) {
        case 's':
        {
            int width, height;
            if (sscanf(optarg, "%dx%d", &width, &height) != 2) {
                printf("parse %s failed! %s\n", longopts[longindex].name, optarg);
            } else {
                argument->viewWidth = width;
                argument->viewHeight = height;
                printf("size:%dx%d\n", width, height);
            }
        }
        break;
        case 'p':
            {
                int x, y;
                //printf("optarg:%s\n", optarg);
                if (sscanf(optarg, "%dx%d", &x, &y) != 2) {
                    printf("parse %s failed! %s\n", longopts[longindex].name, optarg);
                } else {
                    argument->x = x;
                    argument->y = y;
                    printf("pos:%dx%d\n", x, y);
                }
            }
            break;
        case 'c':
        {
            int channelId = strtol(optarg, nullptr, 0);
            printf("channel id:%d\n", channelId);
            argument->channelId = channelId;
        }
        break;
        case 'V':
             {
                 argument->onlyVideo = true;
                 printf("onlyVideo\n");
             }
             break;
        case 'A':
             {
                 argument->onlyAudio = true;
                 printf("onlyAudio\n");
             }
             break;

        case 'h':
        default:
            return -1;
        }
    }

    //printf("optind:%d, argc:%d\n", optind, argc);
    if (optind < argc) {
        argument->url = argv[argc-1];
        printf("url:%s\n", argument->url.c_str());
    }

    return 0;
}

static void showUsage()
{
    printf("Usage: amlMpMediaPlayerDemo <options> <url>\n"
            "options:\n"
            "    --size:       eg: 1920x1080\n"
            "    --pos:        eg: 0x0\n"
            "    --channelId:  eg: 0 main, others: pip\n"
            "    --onlyVideo         \n"
            "    --onlyAudio         \n"
            "\n"
            "url format:\n"
            "    clear ts, eg: dclr:http://10.28.8.30:8881/data/a.ts\n"
            "    clear local file, eg: dclr:/data/a.ts\n"
            "\n"
            );
}

void demoCallback(void* userData, Aml_MP_MediaPlayerEventType event, int64_t param)
{
    switch (event) {
        case AML_MP_MEDIAPLAYER_EVENT_PLAYBACK_COMPLETE:
        {
            printf("%s at #%d AML_MP_MEDIAPLAYER_EVENT_PLAYBACK_COMPLETE\n",__FUNCTION__,__LINE__);
            break;
        }
        case AML_MP_MEDIAPLAYER_EVENT_PREPARED:
        {
            AmlMpMediaPlayerDemo* demo = (AmlMpMediaPlayerDemo*)(userData);
            if (demo) {
                demo->broadcast();
            }
            printf("%s at #%d AML_MP_MEDIAPLAYER_EVENT_PREPARED\n",__FUNCTION__,__LINE__);
            break;
        }
        default:
            break;
    }
}

int main(int argc, char *argv[])
{
    Argument argument{};

    if (parseCommandArgs(argc, argv, &argument) < 0 || argument.url.empty()) {
        showUsage();
        return 0;
    }

    AmlMpMediaPlayerDemo* commandsProcess = new AmlMpMediaPlayerDemo;
    commandsProcess->installSignalHandler();

    AML_MP_MEDIAPLAYER mPlayer = NULL;

    //create
    Aml_MP_MediaPlayer_Create(&mPlayer);

    //only settings
    // channelid is used for pip function which start the demo in a different process.
    // it means different play channels,0 : main, else pip
    if (argument.channelId != -1)
        Aml_MP_MediaPlayer_SetParameter(mPlayer, AML_MP_MEDIAPLAYER_PARAMETER_CHANNEL_ID, (void*)(&argument.channelId));
    Aml_MP_MediaPlayerOnlyHintType type = AML_MP_MEDIAPLAYER_ONLYNONE;
    if (argument.onlyVideo)
        type = AML_MP_MEDIAPLAYER_VIDEO_ONLYHIT;
    if (argument.onlyAudio)
        type = AML_MP_MEDIAPLAYER_AUDIO_ONLYHIT;
    Aml_MP_MediaPlayer_SetParameter(mPlayer, AML_MP_MEDIAPLAYER_PARAMETER_ONLYHINT_TYPE, (void*)&type);

    //setdatasource
    Aml_MP_MediaPlayer_SetDataSource(mPlayer, argument.url.c_str());
    printf("AmlMpMediaPlayerDemo ----------url: %s\n", argument.url.c_str());

    //registerEventCallBack
    Aml_MP_MediaPlayer_RegisterEventCallBack(mPlayer, demoCallback, reinterpret_cast<void*>(commandsProcess));

    //set video window after setting datasource and before starting, take effect after starting.
    //or use --size options when amlMpMediaPlayerDemo
    Aml_MP_MediaPlayer_SetVideoWindow(mPlayer, argument.x, argument.y, argument.viewWidth, argument.viewHeight);

    //prepare
    Aml_MP_MediaPlayer_PrepareAsync(mPlayer);
    commandsProcess->waitPreparedEvent();

#ifndef ANDROID
    //set videotunnel id = 0 on yocto
    //must before prepare
    int iD = argument.channelId == -1 ? 0 : argument.channelId;
    Aml_MP_MediaPlayer_SetParameter(mPlayer, AML_MP_MEDIAPLAYER_PARAMETER_VIDEO_TUNNEL_ID, (void*)(&iD));
#endif

    //start
    Aml_MP_MediaPlayer_Start(mPlayer);
    printf("AmlMpMediaPlayerDemo ----------Aml_MP_MediaPlayer_Start\n");

    //commands process
    printf("AmlMpMediaPlayerDemo ----------fetchAndProcessCommands\n");
    commandsProcess->setCommandHandle(mPlayer);
    commandsProcess->fetchAndProcessCommands();

    //stop
    Aml_MP_MediaPlayer_Stop(mPlayer);

    //reset
    Aml_MP_MediaPlayer_Reset(mPlayer);

    //destroy
    Aml_MP_MediaPlayer_Destroy(mPlayer);
    printf("AmlMpMediaPlayerDemo ----------Aml_MP_MediaPlayer_Destroy\n");

    if (commandsProcess) {
        delete commandsProcess;
        commandsProcess = NULL;
    }
    printf("AmlMpMediaPlayerDemo ----------play exited!\n");
    return 0;
}
