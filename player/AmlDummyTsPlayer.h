/*
 * Copyright (c) 2020 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#ifndef _AML_DUMMY_TSPLAYER_H_
#define _AML_DUMMY_TSPLAYER_H_

#include "AmlPlayerBase.h"
#include <AmTsPlayer.h>
#include <utils/AmlMpUtils.h>
#include <utils/AmlMpBuffer.h>

#ifdef ANDROID
namespace android {
class NativeHandle;
}
#endif

namespace aml_mp {
class AmlDummyTsPlayer : public aml_mp::AmlTsPlayer
{
public:
    AmlDummyTsPlayer(Aml_MP_PlayerCreateParams* createParams, int instanceId);
    ~AmlDummyTsPlayer();
    int start() override;
    int stop() override;
    int pause() override;
    int resume() override;
    int flush() override;

    int startVideoDecoding() override;
    int stopVideoDecoding() override;
    int pauseVideoDecoding() override;
    int resumeVideoDecoding() override;

    int startAudioDecoding() override;
    int stopAudioDecoding() override;

    int startADDecoding() override;
    int stopADDecoding() override;

    int pauseAudioDecoding() override;
    int resumeAudioDecoding() override;

    int setVideoParams(const Aml_MP_VideoParams* params) override;
    int setAudioParams(const Aml_MP_AudioParams* params) override;
    int setADParams(const Aml_MP_AudioParams* params, bool enableMix) override;

private:
    char mName[50];
    AmlDummyTsPlayer(const AmlDummyTsPlayer&) = delete;
    AmlDummyTsPlayer& operator= (const AmlDummyTsPlayer&) = delete;
};

}

#endif
