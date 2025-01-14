/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#ifndef _AM_CAS_LIB_WRAPPER_H_
#define _AM_CAS_LIB_WRAPPER_H_

#include <stdint.h>
#include <mutex>
#include <Aml_MP/Common.h>
#include <utils/AmlMpRefBase.h>


namespace aml_mp {

typedef enum
{
	AM_CAS_SUCCESS,
	AM_CAS_ERROR,
	AM_CAS_ERR_SYS,
} AmCasCode_t;

typedef enum
{
    CAS_STATUS_OK,
    CAS_STATUS_ERROR,
    CAS_STATUS_NOT_IMPLEMENTED,
} AmCasStatus_t;

typedef struct iptvserverinfo {
    char * storepath;
    char * serveraddr;
    char * serverport;
    int    enablelog;
} iptvserverinfo_t;

typedef struct drmserverinfo {
    char * storepath;
    char * serveraddr;
    char * serverport;
    int    enablelog;
} drmserverinfo_t;

typedef struct __tagCAS_INIT_HEADERS {
	char *license_url;
	char *provision_url;
	char *request_header;
	char *request_body;
	char *content_type;
} CasInitHeaders;

template <Aml_MP_CASServiceType serviceType>
class AmCasLibWrapper : public AmlMpRefBase
{
public:
    explicit AmCasLibWrapper(const char* libName);
    AmCasCode_t setCasInstanceId(int casInstanceId);
    int getCasInstanceId();
    AmCasCode_t setPrivateData(void *iDate, int iSize);
    AmCasCode_t provision();
    AmCasCode_t openSession(uint8_t *sessionId);
    AmCasCode_t closeSession(uint8_t *sessionId);
    AmCasCode_t setPids(int vPid, int aPid);
    AmCasCode_t processEcm(int isSection, int isVideoEcm, int vEcmPid, int aEcmPid, uint8_t*pBuffer, int iBufferLength);
    AmCasCode_t processEmm(int isSection, int iPid, uint8_t *pBuffer, int iBufferLength);
    AmCasCode_t decrypt(uint8_t *in, uint8_t *out, int size, void *ext_data);
    AmCasCode_t selectTrack(int trackType,int trackPid,int trackFormat);
    AmCasCode_t releaseAll();
    AmCasCode_t releaseAmCas();
    uint8_t* getOutbuffer();
    //CasStreamInfo mCasStreamInfo;

private:
    char mName[64];

    void* mCasObj = nullptr;
    ~AmCasLibWrapper();

    typedef AmCasStatus_t (*createAmCasFunc)(void **casObj);
    typedef AmCasStatus_t (*setCasInstanceIdFunc)(void *casObj, int casInstanceId);
    typedef int (*getCasInstanceIdFunc)(void *casObj);
    typedef AmCasStatus_t (*setPrivateDataFunc)(void *casObj, void *iDate, int iSize);
    typedef AmCasStatus_t (*provisionFunc)(void *casObj);
    typedef AmCasStatus_t (*setPidsFunc)(void *casObj, int vPid, int aPid);
    typedef AmCasStatus_t (*processEcmFunc)(void *casObj, int isSection, int isVideoEcm, int vEcmPid, int aEcmPid, unsigned char *pBuffer, int iBufferLength);
    typedef AmCasStatus_t (*processEmmFunc)(void *casObj, int isSection, int iPid, uint8_t *pBuffer, int iBufferLength);
    typedef AmCasStatus_t (*decryptFunc)(void *casObj, uint8_t *in, uint8_t *out, int size, void *ext_data);
    typedef AmCasStatus_t (*openSessionFunc)(void *casObj, uint8_t *sessionId);
    typedef AmCasStatus_t (*closeSessionFunc)(void *casObj, uint8_t *sessionId);
    typedef AmCasStatus_t (*releaseAllFunc)(void *casObj);
    typedef uint8_t* (*getOutbufferFunc)(void *casObj);
    typedef AmCasStatus_t (*selectTrackFunc)(void *casObj, int trackType, int trackPid, int trackFormat);
    typedef AmCasStatus_t (*releaseAmCasFunc)(void *casObj);

    struct CasSymbols
    {
        createAmCasFunc createAmCas;
        setCasInstanceIdFunc setCasInstanceId;
        getCasInstanceIdFunc getCasInstanceId;
        setPrivateDataFunc setPrivateData;
        provisionFunc provision;
        setPidsFunc setPids;
        processEcmFunc processEcm;
        processEmmFunc processEmm;
        decryptFunc decrypt;
        openSessionFunc openSession;
        closeSessionFunc closeSession;
        releaseAllFunc releaseAll;
        getOutbufferFunc getOutbuffer;
        selectTrackFunc selectTrack;
        releaseAmCasFunc releaseAmCas;
    };
    static CasSymbols sCasSymbols;
    static void* sCasHandle;
    static std::once_flag sLoadCasLibFlag;

    static void loadLib(const char* libName);
    static void releaseLib();
    template <typename F>
    static F lookupSymbol(const char* symbolName);

private:
    AmCasLibWrapper(const AmCasLibWrapper&) = delete;
    AmCasLibWrapper& operator=(const AmCasLibWrapper&) = delete;
};

}

#endif
