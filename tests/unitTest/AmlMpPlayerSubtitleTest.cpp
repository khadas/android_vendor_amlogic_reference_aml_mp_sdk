#define LOG_TAG "AmlMpPlayerSubtitleTest"
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

using namespace aml_mp;

void AmlMpBase::TeletextControlTest(const std::string & url, Aml_MP_PlayerParameterKey key, AML_MP_TeletextCtrlParam parameter)
{
    MLOGI("----------TeletextControlTest START----------\n");
    startPlaying(url);
    checkHasVideo(url);
    EXPECT_FALSE(waitPlayingErrors());
    if (mRet == 0)
    {
        void *player = getPlayer();
        EXPECT_EQ(Aml_MP_Player_SetParameter(player, key, &parameter), Aml_MP_Player_GetParameter(player, key, &parameter));
        EXPECT_FALSE(waitPlayingErrors(10 * 1000ll));
    }
    MLOGI("----------TeletextControlTest END----------\n");
    stopPlaying();
}

TEST_F(AmlMpTest, SwitchSubtitleTest)
{
    std::string url;
    for (auto &url: mUrls)
    {
        MLOGI("----------SwitchSubtitleTest START----------\n");
        createMpTestSupporter();
        mpTestSupporter->setDataSource(url);
        sptr<ProgramInfo> mProgramInfo = mpTestSupporter->getProgramInfo();
        if (mProgramInfo == nullptr)
        {
            printf("Format for this stream is not ts.");
            continue;
        }
        std::vector<StreamInfo> subtitleStreams = mProgramInfo->subtitleStreams;
        mProgramInfo->scrambled=true;
        mpTestSupporter->startPlay();
        void *player = getPlayer();
        //subtitleParams
        Aml_MP_SubtitleParams subtitleParams{};
        memset(&subtitleParams, 0, sizeof(subtitleParams));
        for (size_t i = 0; i < subtitleStreams.size(); i++)
        {
            subtitleParams.subtitleCodec = subtitleStreams.at(i).codecId;
            subtitleParams.videoFormat = subtitleStreams.at(i).codecId;
            subtitleParams.pid = subtitleStreams.at(i).pid;
            EXPECT_EQ(Aml_MP_Player_SwitchSubtitleTrack(player, &subtitleParams), AML_MP_OK);
        }
        EXPECT_FALSE(waitPlayingErrors(10 * 1000ll));
        stopPlaying();
        MLOGI("----------SwitchSubtitleTest END----------\n");
    }
}

TEST_F(AmlMpTest, SetSubtitleWindowTest)
{
    std::string url;
    createMpTestSupporter();
    AmlMpTestSupporter::DisplayParam displayParam;
    displayParam.x = 200;
    displayParam.y = 200;
    displayParam.width = 600;
    displayParam.height = 600;
    displayParam.zorder = 1;
    displayParam.videoMode = 1;
    mpTestSupporter->setDisplayParam(displayParam);
    for (auto &url: mUrls)
    {
        MLOGI("----------SetSubtitleWindowTest START----------");
        startPlaying(url);
        checkHasVideo(url);
        if (mRet != 0)
        {
        continue;
        }
        EXPECT_FALSE(waitPlayingErrors());
        void *player = getPlayer();
        EXPECT_EQ(Aml_MP_Player_SetSubtitleWindow(player, displayParam.x, displayParam.y, displayParam.width, displayParam.height),
             AML_MP_OK);
        MLOGI("----------SetSubtitleWindowTest END----------");
        stopPlaying();
    }
}

TEST_F(AmlMpTest, ShowHideSubtitleTest)
{
    std::string url;
    for (auto &url: mUrls)
    {
        startPlaying(url);
        if (mRet == 0)
        {
            checkHasVideo(url);
            EXPECT_FALSE(waitPlayingErrors());
            void *player = getPlayer();
            EXPECT_EQ(Aml_MP_Player_HideSubtitle(player), AML_MP_OK);
            EXPECT_FALSE(waitPlayingErrors());
            EXPECT_EQ(Aml_MP_Player_ShowSubtitle(player), AML_MP_OK);
            EXPECT_FALSE(waitPlayingErrors());
            EXPECT_EQ(Aml_MP_Player_HideSubtitle(player), AML_MP_OK);
            EXPECT_FALSE(waitPlayingErrors());
            EXPECT_EQ(Aml_MP_Player_ShowSubtitle(player), AML_MP_OK);
            EXPECT_FALSE(waitPlayingErrors());
        }
        stopPlaying();
    }
}
