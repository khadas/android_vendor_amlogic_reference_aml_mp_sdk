/*
 * Copyright (c) 2020 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#define LOG_TAG "AmlMpConfig"
#include <utils/AmlMpLog.h>
#include "AmlMpConfig.h"
#include <cutils/properties.h>
#include <string>

namespace aml_mp {
#ifdef ANDROID

template <typename T>
void AmlMpConfig::initProperty(const char* propertyName, T& value)
{
    char bufs[PROP_VALUE_MAX];

    if (property_get(propertyName, bufs, 0) > 0) {
        value = strtol(bufs, nullptr, 0);
    }
}
#endif

#ifdef ANDROID

template<>
void AmlMpConfig::initProperty(const char* propertyName, std::string& value)
{
    char bufs[PROP_VALUE_MAX];

    if (property_get(propertyName, bufs, 0) > 0) {
        value = bufs;
    }
}
#endif

void AmlMpConfig::reset()
{
    mLogDebug = 0;
#if ANDROID_PLATFORM_SDK_VERSION >= 30
    mTsPlayerNonTunnel = 1;
#else
    mTsPlayerNonTunnel = 0;
#endif
    mWaitingEcmMode = 1;
    mWriteBufferSize = 2; // default write buffer size set to 2MB.
    mDumpPackts = 0;

#if ANDROID_PLATFORM_SDK_VERSION == 29
    mUseVideoTunnel = 0;
#elif ANDROID_PLATFORM_SDK_VERSION >= 30
    mUseVideoTunnel = 1;
#endif
    mCasPipSupport = 0;
    mCasFCCSupport = 0;
    mSecMemSize = 0;
    mCasType = "none";
}

void AmlMpConfig::init()
{
#ifdef ANDROID

    initProperty("vendor.amlmp.log-debug", mLogDebug);
    initProperty("vendor.amtsplayer.pipeline", mTsPlayerNonTunnel);
    initProperty("vendor.amlmp.waiting-ecm-mode", mWaitingEcmMode);
    initProperty("vendor.amlmp.write-buffer-size", mWriteBufferSize);
    initProperty("vendor.enable.dump.packts", mDumpPackts);
    initProperty("vendor.cas.support.pip.function", mCasPipSupport);
    initProperty("vendor.cas.support.fcc.function", mCasFCCSupport);
    initProperty("vendor.secmem.size", mSecMemSize);
    initProperty("vendor.cas.type", mCasType);
#endif

}

AmlMpConfig::AmlMpConfig()
{
    reset();
    init();
}


}
