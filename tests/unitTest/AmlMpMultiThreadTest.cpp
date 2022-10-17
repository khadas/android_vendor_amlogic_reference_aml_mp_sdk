#define LOG_TAG "AmlMpMultiThreadTest"
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

using namespace aml_mp;

TEST_F(AmlMpTest, DVRMultiThreadStop)
{
    std::string url;
    char recordPathInfo[100];
    for (auto &url: mUrls) {
        sptr<AmlMpTestSupporter> testDvrPlayer = nullptr;
        DVRRecordRecordStream(url);

        createMpTestSupporter(&testDvrPlayer);
        testDvrPlayer->setDataSource(AML_MP_RECORD_PATH);
        testDvrPlayer->prepare(CryptoMode);
        testDvrPlayer->startDVRPlayback();
        AML_MP_DVRPLAYER dvrplayer = getDVRPlayer(testDvrPlayer);

        EXPECT_FALSE(waitPlaying(20* 1000ll));

        std::mutex lock;
        std::condition_variable cond;
        bool beginStop = false;

        std::thread stopThread1([&, dvrplayer]() {
            {
                std::unique_lock<std::mutex> _l(lock);
                cond.wait(_l, [&]{return beginStop;});
            }

            MLOGI("thread call stop");
            EXPECT_EQ(Aml_MP_DVRPlayer_Stop(dvrplayer), AML_MP_OK);
        });

        {
            std::unique_lock<std::mutex> _l(lock);
            beginStop = true;
            cond.notify_all();
        }

        MLOGI("main call stop");
        EXPECT_EQ(Aml_MP_DVRPlayer_Stop(dvrplayer), AML_MP_OK);

        stopThread1.join();
    }
}