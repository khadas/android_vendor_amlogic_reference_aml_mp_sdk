/*
 * Copyright (c) 2020 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#ifndef _AML_MP_CAS_H_
#define _AML_MP_CAS_H_

#include "Common.h"

#define MAX_CHAN_COUNT (8)
#define MAX_DATA_LEN (8)

/**\brief Service Mode of the program*/
typedef enum {
    AML_MP_CAS_SERVICE_DVB, /**< DTV live playing.*/
    AML_MP_CAS_SERVICE_IPTV /**< IPTV.*/
} Aml_MP_CASServiceMode;

typedef struct {
    uint16_t service_id;                    /**< The service's index.*/
    uint8_t dmx_dev;                        /**< The demux device's index.*/
    uint8_t dsc_dev;                        /**< The descrambler device's index.*/
    uint8_t dvr_dev;                        /**< The DVR device's index.*/
    Aml_MP_CASServiceMode serviceMode;      /**< Service mode.*/
    Aml_MP_CASServiceType serviceType;      /**< Service type.*/
    uint16_t ecm_pid;                       /**< ECM's PID.*/
    uint16_t stream_pids[MAX_CHAN_COUNT];   /**< Elementry streams' index.*/
    uint32_t stream_num;                    /**< Elementary streams' number.*/
    uint8_t ca_private_data[MAX_DATA_LEN];  /**< Private data.*/
    uint8_t ca_private_data_len;            /**< Private data's length.*/
    uint8_t fend_dev __AML_MP_RESERVE_ALIGNED; /**< The frontend device's index.*/
    long reserved[7];
} Aml_MP_CASServiceInfo;

typedef enum {
    AML_MP_CAS_SECTION_PMT,
    AML_MP_CAS_SECTION_CAT,
    AML_MP_CAS_SECTION_NIT,
}Aml_MP_CASSectionType;

typedef struct {
    Aml_MP_DemuxId          dmxDev;
    uint16_t                serviceId;
    Aml_MP_CASSectionType   sectionType;
} Aml_MP_CASSectionReportAttr;

typedef int (*Aml_MP_CAS_EventCallback)(AML_MP_CASSESSION casSession, const char *json);

typedef enum {
    AML_MP_CAS_ENCRYPT, /**< Encrypt.*/
    AML_MP_CAS_DECRYPT  /**< Decrypt.*/
} Aml_MP_CASCryptoType;

typedef struct {
    Aml_MP_CASCryptoType  type;                             /**< Work type.*/
    char                  filepath[AML_MP_MAX_PATH_SIZE];   /**< Location of the record file.*/
    int                   segmentId;                        /**< Current segment's index.*/
    int64_t               offset;                           /**< Current offset in the segment file.*/
    Aml_MP_Buffer         inputBuffer;                      /**< Input data buffer.*/
    Aml_MP_Buffer         outputBuffer;                     /**< Output data buffer.*/
    size_t                outputSize;                       /**< Output data size in bytes.*/
} Aml_MP_CASCryptoParams;

typedef struct Aml_MP_CASDVRReplayParams {
    Aml_MP_DemuxId dmxDev;      /**< The demux device's index.*/
} Aml_MP_CASDVRReplayParams;

typedef struct {
    loff_t start;
    loff_t end;
} Aml_MP_CASStoreRegion;

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    char* keyuri;
    uint8_t iv[16];
    int seg_size;
    int key_type;
    int seg_no;
    int iv_reset;/*the start of the each segment need set iv*/
    long reserved[8];
} Aml_MP_CASHlsExtData;

///////////////////////////////////////////////////////////////////////////////
// global CAS function begin
/**
 * \brief Aml_MP_CAS_Initialize
 * Initialize CAS module
 *
 * \return 0 if success
 */
int Aml_MP_CAS_Initialize();

/**
 * \brief Aml_MP_CAS_Terminate
 * Uninitialize CAS module
 *
 * \return 0 if success
 */
int Aml_MP_CAS_Terminate();

/**
 * \brief Aml_MP_CAS_IsNeedWholeSection
 * Whether CAS module need whole section
 *
 * \return 1 need whole section
 * \return 0 not need whole section
 */
int Aml_MP_CAS_IsNeedWholeSection();

/**
 * \brief Aml_MP_CAS_IsSystemIdSupported
 * Check whether support specific CAS ID
 *
 * \param [in]  CAS ID
 *
 * \return 1 for support
 * \return 0 for not support
 */
int Aml_MP_CAS_IsSystemIdSupported(int caSystemId);

/**
 * \brief Aml_MP_CAS_ReportSection
 * Send section to CAS module
 *
 * \param [in]  section data type
 * \param [in]  section data
 * \param [in]  section data size
 *
 * \return 0 if success
 */
int Aml_MP_CAS_ReportSection(Aml_MP_CASSectionReportAttr* pAttr, uint8_t* data, size_t len);

/**
 * \brief Aml_MP_CAS_SetEmmPid
 * Set EMM id to CAS module
 *
 * \param [in]  demux ID
 * \param [in]  EMM PID
 *
 * \return 0 if success
 */
int Aml_MP_CAS_SetEmmPid(int dmx_dev, uint16_t emmPid);

/**
 * \brief Aml_MP_CAS_DVREncrypt
 * DVR encrypt function
 *
 * \param [in]  CAS session
 * \param [in]  crypto param
 *
 * \return 0 if success
 */
int Aml_MP_CAS_DVREncrypt(AML_MP_CASSESSION casSession, Aml_MP_CASCryptoParams *cryptoPara);

/**
 * \brief Aml_MP_CAS_DVRDecrypt
 * DVR decrypt function
 *
 * \param [in]  CAS session
 * \param [in]  crypto param
 *
 * \return 0 if success
 */
int Aml_MP_CAS_DVRDecrypt(AML_MP_CASSESSION casSession, Aml_MP_CASCryptoParams *cryptoPara);
// global CAS function end

/**
 * \brief Aml_MP_CAS_OpenSession
 * Open CAS session
 *
 * \param [out] CAS session
 * \param [in]  service type
 *
 * \return 0 if success
 */
int Aml_MP_CAS_OpenSession(AML_MP_CASSESSION* casSession, Aml_MP_CASServiceType serviceType);

/**
 * \brief Aml_MP_CAS_CloseSession
 * Close CAS session
 *
 * \param [in]  CAS session
 *
 * \return 0 if success
 */
int Aml_MP_CAS_CloseSession(AML_MP_CASSESSION casSession);

/**
 * \brief Aml_MP_CAS_RegisterEventCallback
 * Register CAS event callback
 *
 * \param [in]  CAS session
 * \param [in]  CAS event callback func
 * \param [in]  user data
 *
 * \return 0 if success
 */
int Aml_MP_CAS_RegisterEventCallback(AML_MP_CASSESSION casSession, Aml_MP_CAS_EventCallback cb, void* userData);

/**
 * \brief Aml_MP_CAS_Ioctl
 * CAS ioctl
 *
 * \param casSession session
 * \param inJson The input cmd string
 * \param outJson The output string
 * \param outLen The output string length
 *
 * \return 0 if success
 */
int Aml_MP_CAS_Ioctl(AML_MP_CASSESSION casSession, const char* inJson, char* outJson, uint32_t outLen);

//for live
/**
 * \brief Aml_MP_CAS_StartDescrambling
 * Start descrambling
 * Use before start player
 *
 * \param [in]  CAS session
 * \param [in]  descrambling service info
 *
 * \return 0 if success
 */
int Aml_MP_CAS_StartDescrambling(AML_MP_CASSESSION casSession, Aml_MP_CASServiceInfo* serviceInfo);

/**
 * \brief Aml_MP_CAS_StartDescramblingIPTV
 *
 * \param casSession handle
 * \param params iptv cas params
 *
 * \return 0 if success
 */
int Aml_MP_CAS_StartDescramblingIPTV(AML_MP_CASSESSION casSession, const Aml_MP_IptvCASParams* params);

/**
 * \brief Aml_MP_CAS_StopDescrambling
 * Stop descrambling
 *
 * \param [in]  CAS session
 *
 * \return 0 if success
 */
int Aml_MP_CAS_StopDescrambling(AML_MP_CASSESSION casSession);

/**
 * \brief Aml_MP_CAS_UpdateDescramblingPid
 * Update descrambling PID
 *
 * \param [in]  CAS session
 * \param [in]  old stream PID
 * \param [in]  new stream PID
 *
 * \return 0 if success
 */
int Aml_MP_CAS_UpdateDescramblingPid(AML_MP_CASSESSION casSession, int oldStreamPid, int newStreamPid);

/**
 * \brief Aml_MP_CAS_SwitchAudioTrack
 * switch audio track
 *
 * \param [in]  CAS session
 * \param [in]  audio params
 *
 * \return 0 if success
 */
int Aml_MP_CAS_SwitchAudioTrack(AML_MP_CASSESSION casSession, Aml_MP_AudioParams* params);

//for dvr record
/**
 * \brief Aml_MP_CAS_StartDVRRecord
 * Strat DVR record
 *
 * \param [in]  CAS session
 * \param [in]  service info
 *
 * \return 0 if success
 */
int Aml_MP_CAS_StartDVRRecord(AML_MP_CASSESSION casSession, Aml_MP_CASServiceInfo *serviceInfo);

/**
 * \brief Aml_MP_CAS_StopDVRRecord
 * Stop DVR record
 *
 * \param [in]  CAS session
 *
 * \return 0 if success
 */
int Aml_MP_CAS_StopDVRRecord(AML_MP_CASSESSION casSession);

//for dvr replay
/**
 * \brief Aml_MP_CAS_StartDVRReplay
 * Strat DVR replay
 *
 * \param [in]  CAS session
 * \param [in]  replay info
 *
 * \return 0 if success
 */
int Aml_MP_CAS_StartDVRReplay(AML_MP_CASSESSION casSession, Aml_MP_CASDVRReplayParams *dvrReplayParams);

/**
 * \brief Aml_MP_CAS_StopDVRReplay
 * Stop DVR replay
 *
 * \param [in]  CAS session
 *
 * \return 0 if success
 */
int Aml_MP_CAS_StopDVRReplay(AML_MP_CASSESSION casSession);

/**
 * \brief Aml_MP_CAS_DVRReplay
 * need be called after Aml_MP_CAS_StartDVRReplay
 *
 * \param casSession
 * \param cryptoParams
 *
 * \return 0 if success
 */
int Aml_MP_CAS_DVRReplay(AML_MP_CASSESSION casSession, Aml_MP_CASCryptoParams* cryptoParams);

/**
 * \brief Aml_MP_CAS_CreateSecmem
 * Create secure memory
 *
 * \param [in]  CAS session
 * \param [in]  type
 * \param [out] secure memory
 * \param [out] secure memory size
 *
 * \return secure memory handle
 */
AML_MP_SECMEM Aml_MP_CAS_CreateSecmem(AML_MP_CASSESSION casSession, Aml_MP_CASServiceType type, void **pSecbuf, uint32_t *size);

/**
 * \brief Aml_MP_CAS_DestroySecmem
 * Destroy secure memory
 *
 * \param [in]  CAS session
 * \param [in]  secure memory
 *
 * \return 0 if success
 */
int Aml_MP_CAS_DestroySecmem(AML_MP_CASSESSION casSession, AML_MP_SECMEM secMem);

/**
 * \brief Aml_MP_CAS_GetStoreRegion
 * get all region of store info
 *
 * \param casSession
 * \param region region of store info
 * \param regionCount region count
 *
 * \return 0 if success
 */
int Aml_MP_CAS_GetStoreRegion(AML_MP_CASSESSION casSession, Aml_MP_CASStoreRegion *region, uint8_t *regionCount);

/**
 * \brief Aml_MP_CAS_ProcessEcmIPTV
 * processEcm
 *
 * \param [in]  casSession session
 * \param [in]  isSection is section of is TS packet
 * \param [in]  ecmPid if isSection is true, then need specify the ecmPid of ecmData
 * \param [in]  data ecmData
 * \param [in]  size ecmData size
 *
 * \return 0 if success
 */
int Aml_MP_CAS_ProcessEcmIPTV(AML_MP_CASSESSION casSession, bool isSection, int ecmPid, const uint8_t* data, size_t size);

/**
 * \brief Aml_MP_CAS_DecryptIPTV
 * decrypt data
 *
 * \param [in]  casSession session
 * \param [in]  data encrypted data
 * \param [in]  size size of encrypted data
 * \param [in]  ext_data hls info
 * \param [out]  outbuffer decrypted buffer secure
 *
 * \return 0 if success
 */
int Aml_MP_CAS_DecryptIPTV(AML_MP_CASSESSION casSession, uint8_t* data, size_t size, void* ext_data, Aml_MP_Buffer* outbuffer);

#ifdef __cplusplus
}
#endif
#endif
