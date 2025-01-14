/*
 * Copyright (c) 2020 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#define LOG_TAG "AmlDvbCasHal"
#include "AmlDvbCasHal.h"
#include <pthread.h>
#include <utils/AmlMpUtils.h>
#include <utils/AmlMpLog.h>

static const char* mName = LOG_TAG;

///////////////////////////////////////////////////////////////////////////////
#ifdef HAVE_CAS_HAL
extern CasHandle g_casHandle;

static CA_SERVICE_TYPE_t convertToCAServiceType(Aml_MP_CASServiceType casServiceType)
{
    switch (casServiceType) {
    case AML_MP_CAS_SERVICE_LIVE_PLAY:
        return SERVICE_LIVE_PLAY;

    case AML_MP_CAS_SERVICE_PVR_PLAY:
        return SERVICE_PVR_PLAY;

    case AML_MP_CAS_SERVICE_PVR_RECORDING:
        return SERVICE_PVR_RECORDING;

    case AML_MP_CAS_SERVICE_PVR_TIMESHIFT_PLAY:
        return SERVICE_PVR_TIMESHIFT_PLAY;

    case AML_MP_CAS_SERVICE_PVR_TIMESHIFT_RECORDING:
        return SERVICE_PVR_TIMESHIFT_RECORDING;
    default:
        break;
    }

    return SERVICE_TYPE_INVALID;
}

static CA_SERVICE_MODE_t convertToCAServiceMode(Aml_MP_CASServiceMode serviceMode)
{
    switch (serviceMode) {
    case AML_MP_CAS_SERVICE_DVB:
        return SERVICE_DVB;

    case AML_MP_CAS_SERVICE_IPTV:
        return SERVICE_IPTV;

    default:
        break;
    }

    return SERVICE_DVB;
}

static void convertToCAServiceInfo(AM_CA_ServiceInfo_t* caServiceInfo, Aml_MP_CASServiceInfo* mpServiceInfo)
{
    caServiceInfo->service_id = mpServiceInfo->service_id;
    caServiceInfo->fend_dev = mpServiceInfo->fend_dev;
    caServiceInfo->dmx_dev = mpServiceInfo->dmx_dev;
    caServiceInfo->dsc_dev = mpServiceInfo->dsc_dev;
    caServiceInfo->dvr_dev = mpServiceInfo->dvr_dev;
    caServiceInfo->service_mode = convertToCAServiceMode(mpServiceInfo->serviceMode);
    caServiceInfo->service_type = convertToCAServiceType(mpServiceInfo->serviceType);
    caServiceInfo->ecm_pid = mpServiceInfo->ecm_pid;
    memcpy(caServiceInfo->stream_pids, mpServiceInfo->stream_pids, sizeof(caServiceInfo->stream_pids));
    caServiceInfo->stream_num = mpServiceInfo->stream_num;
    memcpy(caServiceInfo->ca_private_data, mpServiceInfo->ca_private_data, sizeof(caServiceInfo->ca_private_data));
    caServiceInfo->ca_private_data_len = mpServiceInfo->ca_private_data_len;
}

static int convertToAmlMPErrorCode(AM_RESULT CasResult) {
    switch (CasResult) {
        case AM_ERROR_SUCCESS:
            return AML_MP_OK;
        case AM_ERROR_NOT_LOAD:
            return AML_MP_ERROR_NO_INIT;
        case AM_ERROR_NOT_SUPPORTED:
            return AML_MP_ERROR_INVALID_OPERATION;
        case AM_ERROR_OVERFLOW:
            return AML_MP_ERROR_BAD_INDEX;
        default:
            return AML_MP_ERROR;
    }
}

#endif

///////////////////////////////////////////////////////////////////////////////
namespace aml_mp {
#ifdef HAVE_CAS_HAL
std::mutex AmlDvbCasHal::sCasHalSessionLock;
std::map<CasSession, wptr<AmlDvbCasHal>> AmlDvbCasHal::sCasHalSessionMap;
#endif

AmlDvbCasHal::AmlDvbCasHal(Aml_MP_CASServiceType serviceType)
: AmlCasBase(serviceType)
{
#ifdef HAVE_CAS_HAL
    AM_RESULT ret = AM_ERROR_GENERAL_ERROR;
    CA_SERVICE_TYPE_t caServiceType = convertToCAServiceType(mServiceType);
    ret = AM_CA_OpenSession(g_casHandle, &mCasSession, caServiceType);
    if (ret != AM_ERROR_SUCCESS) {
        MLOGE("AM_CA_OpenSession failed!");
        return;
    }

    MLOG("openSession:%#zx", mCasSession);

    std::unique_lock<std::mutex> _l(sCasHalSessionLock);
    auto result = sCasHalSessionMap.emplace(mCasSession, this);
    if (!result.second) {
        MLOGE("save CasSession failed! (%#zx, %p)", mCasSession, this);
    }
#endif
}

AmlDvbCasHal::~AmlDvbCasHal()
{
    MLOG();

#ifdef HAVE_CAS_HAL
    AM_RESULT ret = AM_ERROR_GENERAL_ERROR;
    ret = AM_CA_CloseSession(mCasSession);
    if (ret != AM_ERROR_SUCCESS) {
        MLOGE("AM_CA_CloseSession failed!");
    }

    std::unique_lock<std::mutex> _l(sCasHalSessionLock);
    auto result = sCasHalSessionMap.erase(mCasSession);
    if (result == 0) {
        MLOGE("remove CasSession failed! (%#zx, %p)", mCasSession, this);
    }
#endif
}

int AmlDvbCasHal::registerEventCallback(Aml_MP_CAS_EventCallback cb, void* userData)
{
    MLOG();

    int ret = AML_MP_ERROR;

    mCb = cb;
    mUserData = userData;

#ifdef HAVE_CAS_HAL
    ret = convertToAmlMPErrorCode(AM_CA_RegisterEventCallback(mCasSession, sCasHalCb));
#else
    AML_MP_UNUSED(cb);
#endif

    return ret;
}

int AmlDvbCasHal::startDescrambling(Aml_MP_CASServiceInfo* serviceInfo)
{
    MLOG();

    int ret = AML_MP_ERROR;
#ifdef HAVE_CAS_HAL
    AM_CA_ServiceInfo_t caServiceInfo;
    convertToCAServiceInfo(&caServiceInfo, serviceInfo);

    ret = convertToAmlMPErrorCode(AM_CA_StartDescrambling(mCasSession, &caServiceInfo));
#else
    AML_MP_UNUSED(serviceInfo);
#endif

    return ret;
}

int AmlDvbCasHal::stopDescrambling()
{
    MLOG();

    int ret = AML_MP_ERROR;

#ifdef HAVE_CAS_HAL
    ret = convertToAmlMPErrorCode(AM_CA_StopDescrambling(mCasSession));
#endif

    return ret;
}

int AmlDvbCasHal::updateDescramblingPid(int oldStreamPid, int newStreamPid)
{
    MLOG("oldStreamPid:%d, newStreamPid:%d", oldStreamPid, newStreamPid);

    int ret = AML_MP_ERROR;

#ifdef HAVE_CAS_HAL
   ret = convertToAmlMPErrorCode(AM_CA_UpdateDescramblingPid(mCasSession, oldStreamPid, newStreamPid));
#endif

    return ret;
}

int AmlDvbCasHal::startDVRRecord(Aml_MP_CASServiceInfo* serviceInfo)
{
    MLOG();

    int ret = AML_MP_ERROR;

#ifdef HAVE_CAS_HAL
    AM_CA_ServiceInfo_t caServiceInfo;
    convertToCAServiceInfo(&caServiceInfo, serviceInfo);

    ret = convertToAmlMPErrorCode(AM_CA_DVRStart(mCasSession, &caServiceInfo));
#else
    AML_MP_UNUSED(serviceInfo);
#endif

    return ret;
}

int AmlDvbCasHal::stopDVRRecord()
{
    MLOG();

    int ret = AML_MP_ERROR;

#ifdef HAVE_CAS_HAL
    ret = convertToAmlMPErrorCode(AM_CA_DVRStop(mCasSession));
#endif

    return ret;
}

int AmlDvbCasHal::startDVRReplay(Aml_MP_CASDVRReplayParams* dvrReplayParams)
{
    MLOG();

    int ret = AML_MP_ERROR;
#ifdef HAVE_CAS_HAL
    AM_CA_PreParam_t caParams;
    caParams.dmx_dev = dvrReplayParams->dmxDev;

    ret = convertToAmlMPErrorCode(AM_CA_DVRSetPreParam(mCasSession, &caParams));
#else
    AML_MP_UNUSED(dvrReplayParams);
#endif

    return ret;
}

int AmlDvbCasHal::stopDVRReplay()
{
    MLOG();

    int ret = AML_MP_ERROR;

#ifdef HAVE_CAS_HAL
    ret = convertToAmlMPErrorCode(AM_CA_DVRStopReplay(mCasSession));
#endif

    return ret;
}

int AmlDvbCasHal::dvrReplay(Aml_MP_CASCryptoParams* cryptoParams)
{
    MLOG();

    int ret = AML_MP_ERROR;

#ifdef HAVE_CAS_HAL
    AM_CA_CryptoPara_t* amCryptoParams = reinterpret_cast<AM_CA_CryptoPara_t*>(cryptoParams);
    ret = convertToAmlMPErrorCode(AM_CA_DVRReplay(mCasSession, amCryptoParams));
#else
    AML_MP_UNUSED(cryptoParams);
#endif

    return ret;
}

int AmlDvbCasHal::DVREncrypt(Aml_MP_CASCryptoParams* cryptoParams)
{
    int ret = AML_MP_ERROR;
#ifdef HAVE_CAS_HAL
    static_assert(sizeof(Aml_MP_CASCryptoParams) == sizeof(AM_CA_CryptoPara_t), "Incompatible with AM_CA_CryptoPara_t!");
    static_assert(sizeof(loff_t) == sizeof(int64_t), "Incompatible loff_t vs int64_t");
    AM_CA_CryptoPara_t* amCryptoParams = reinterpret_cast<AM_CA_CryptoPara_t*>(cryptoParams);

    ret = convertToAmlMPErrorCode(AM_CA_DVREncrypt(mCasSession, amCryptoParams));
#else
    AML_MP_UNUSED(cryptoParams);
#endif

    return ret;
}

int AmlDvbCasHal::DVRDecrypt(Aml_MP_CASCryptoParams* cryptoParams)
{
    int ret = AML_MP_ERROR;
#ifdef HAVE_CAS_HAL
    static_assert(sizeof(Aml_MP_CASCryptoParams) == sizeof(AM_CA_CryptoPara_t), "Incompatible with AM_CA_CryptoPara_t!");
    static_assert(sizeof(loff_t) == sizeof(int64_t), "Incompatible loff_t vs int64_t");
    AM_CA_CryptoPara_t* amCryptoParams = reinterpret_cast<AM_CA_CryptoPara_t*>(cryptoParams);

    ret = convertToAmlMPErrorCode(AM_CA_DVRDecrypt(mCasSession, amCryptoParams));
#else
    AML_MP_UNUSED(cryptoParams);
#endif

    return ret;
}

AML_MP_SECMEM AmlDvbCasHal::createSecmem(Aml_MP_CASServiceType type, void** pSecbuf, uint32_t* size)
{
#ifdef HAVE_CAS_HAL
    SecMemHandle secMem = 0;
    CA_SERVICE_TYPE_t caServiceType = convertToCAServiceType(type);

    secMem = AM_CA_CreateSecmem(mCasSession, caServiceType, pSecbuf, size);
    MLOG("service type:%d, secMem:%#zx", type, secMem);

    return (AML_MP_SECMEM)secMem;
#else
    AML_MP_UNUSED(type);
    AML_MP_UNUSED(pSecbuf);
    AML_MP_UNUSED(size);

    return nullptr;
#endif

}

int AmlDvbCasHal::destroySecmem(AML_MP_SECMEM secMem)
{

    int ret = AML_MP_ERROR;

#ifdef HAVE_CAS_HAL
    MLOG("secMem:%#zx", (SecMemHandle)secMem);
    ret = convertToAmlMPErrorCode(AM_CA_DestroySecmem(mCasSession, (SecMemHandle)secMem));
#else
    AML_MP_UNUSED(secMem);
#endif

    return ret;
}

int AmlDvbCasHal::ioctl(const char* inJson, char* outJson, uint32_t outLen)
{
    int ret = AML_MP_ERROR;
#ifdef HAVE_CAS_HAL
    ret = convertToAmlMPErrorCode(AM_CA_Ioctl(mCasSession, inJson, outJson, outLen));
#else
    AML_MP_UNUSED(inJson);
    AML_MP_UNUSED(outJson);
    AML_MP_UNUSED(outLen);
#endif
    return ret;
}

int AmlDvbCasHal::getStoreRegion(Aml_MP_CASStoreRegion* region, uint8_t* regionCount)
{
    int ret = AML_MP_ERROR;
#ifdef HAVE_CAS_HAL
    static_assert(sizeof(Aml_MP_CASStoreRegion) == sizeof(AM_CA_StoreRegion_t), "Incompatible with AM_CA_StoreRegion_t!");
    ret = convertToAmlMPErrorCode(AM_CA_GetStoreRegion(mCasSession, reinterpret_cast<AM_CA_StoreRegion_t*>(region), regionCount));
#else
    AML_MP_UNUSED(region);
    AML_MP_UNUSED(regionCount);
#endif
    return ret;
}

#ifdef HAVE_CAS_HAL
AM_RESULT AmlDvbCasHal::sCasHalCb(CasSession session, char* json)
{
    sptr<AmlDvbCasHal> dvbCasCal;

    {
        std::unique_lock<std::mutex> _l(sCasHalSessionLock);
        auto ret = sCasHalSessionMap.find(session);
        if (ret != sCasHalSessionMap.end()) {
            dvbCasCal = ret->second.promote();
        }
    }

    if (dvbCasCal != nullptr) {
        dvbCasCal->notifyListener(json);
    } else {
        MLOGE("dvbCasCal is NULL, session:%#zx", session);
    }

    return AM_ERROR_SUCCESS;
}
#endif

void AmlDvbCasHal::notifyListener(char* json)
{
    if (mCb) {
        mCb(this, json);
    } else {
        MLOGW("mCb is NULL, json:%s", json);
    }
}


}

