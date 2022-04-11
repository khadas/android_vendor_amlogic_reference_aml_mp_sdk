/*
 * Copyright (c) 2020 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#define LOG_TAG "AmlDummyTsPlayer"
#include <utils/AmlMpLog.h>
#include <utils/AmlMpUtils.h>
#include "AmlPlayerBase.h"
#include "AmlTsPlayer.h"
#include "AmlDummyTsPlayer.h"

namespace aml_mp {

AmlDummyTsPlayer::AmlDummyTsPlayer(Aml_MP_PlayerCreateParams* createParams, int instanceId)
: aml_mp::AmlTsPlayer(createParams, instanceId)
{
    snprintf(mName, sizeof(mName), "%s_%d", LOG_TAG, instanceId);
    MLOGI("%s:%d", __FUNCTION__, __LINE__);
}

AmlDummyTsPlayer::~AmlDummyTsPlayer()
{
    MLOGI("%s:%d", __FUNCTION__, __LINE__);
}

///////////////////////////////////////////////////////////////////////////////
//         dvr audio/video/ad real operation finish in libdvr                //
///////////////////////////////////////////////////////////////////////////////

int AmlDummyTsPlayer::start()
{
    AmlPlayerBase::start();
    MLOGI("%s:%d", __FUNCTION__, __LINE__);
    return 0;
}

int AmlDummyTsPlayer::stop()
{
    AmlPlayerBase::stop();
    MLOGI("%s:%d", __FUNCTION__, __LINE__);
    return 0;
}

int AmlDummyTsPlayer::pause()
{
    MLOGI("%s:%d", __FUNCTION__, __LINE__);
    return 0;
}

int AmlDummyTsPlayer::resume()
{
    MLOGI("%s:%d", __FUNCTION__, __LINE__);
    return 0;
}

int AmlDummyTsPlayer::flush()
{
    MLOGI("%s:%d", __FUNCTION__, __LINE__);
    return 0;
}

int AmlDummyTsPlayer::startVideoDecoding()
{
    MLOGI("%s:%d", __FUNCTION__, __LINE__);
    return 0;
}

int AmlDummyTsPlayer::stopVideoDecoding()
{
    MLOGI("%s:%d", __FUNCTION__, __LINE__);
    return 0;
}

int AmlDummyTsPlayer::pauseVideoDecoding()
{
    MLOGI("%s:%d", __FUNCTION__, __LINE__);
    return 0;
}

int AmlDummyTsPlayer::resumeVideoDecoding()
{
    MLOGI("%s:%d", __FUNCTION__, __LINE__);
    return 0;
}

int AmlDummyTsPlayer::startAudioDecoding()
{
    MLOGI("%s:%d", __FUNCTION__, __LINE__);
    return 0;
}

int AmlDummyTsPlayer::stopAudioDecoding()
{
    MLOGI("%s:%d", __FUNCTION__, __LINE__);
    return 0;
}

int AmlDummyTsPlayer::pauseAudioDecoding()
{
    MLOGI("%s:%d", __FUNCTION__, __LINE__);
    return 0;
}

int AmlDummyTsPlayer::resumeAudioDecoding()
{
    MLOGI("%s:%d", __FUNCTION__, __LINE__);
    return 0;
}

int AmlDummyTsPlayer::startADDecoding()
{
    MLOGI("%s:%d", __FUNCTION__, __LINE__);
    return 0;
}

int AmlDummyTsPlayer::stopADDecoding()
{
    MLOGI("%s:%d", __FUNCTION__, __LINE__);
    return 0;
}

int AmlDummyTsPlayer::setVideoParams(const Aml_MP_VideoParams* params)
{
    MLOGI("%s:%d", __FUNCTION__, __LINE__);

    am_tsplayer_result ret;
    Aml_MP_DemuxMemSecLevel secureLevel = params->secureLevel;
    ret = AmTsPlayer_setParams(mPlayer, AM_TSPLAYER_KEY_VIDEO_SECLEVEL, (void*)&secureLevel);
    if (ret != AM_TSPLAYER_OK) {
        return -1;
    }

    return 0;
}

int AmlDummyTsPlayer::setAudioParams(const Aml_MP_AudioParams* params)
{
    MLOGI("%s:%d", __FUNCTION__, __LINE__);

    am_tsplayer_result ret;
    Aml_MP_DemuxMemSecLevel secureLevel = params->secureLevel;
    ret = AmTsPlayer_setParams(mPlayer, AM_TSPLAYER_KEY_AUDIO_SECLEVEL, (void*)&secureLevel);
    if (ret != AM_TSPLAYER_OK) {
        return -1;
    }
    return 0;
}

int AmlDummyTsPlayer::setADParams(const Aml_MP_AudioParams* params, bool enableMix)
{
    MLOGI("%s:%d", __FUNCTION__, __LINE__);
    AML_MP_UNUSED(params);
    AML_MP_UNUSED(enableMix);
    return 0;
}

}
