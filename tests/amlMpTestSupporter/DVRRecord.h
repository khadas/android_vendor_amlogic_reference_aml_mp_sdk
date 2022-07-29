/*
 * Copyright (c) 2020 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#include "demux/AmlTsParser.h"
#include "source/Source.h"
#include "TestModule.h"
#include <Aml_MP/Aml_MP.h>

namespace aml_mp {
class DVRRecord : public TestModule, public ISourceReceiver
{
public:
    DVRRecord(bool cryptoMode, Aml_MP_DemuxId demuxId, const sptr<ProgramInfo>& programInfo, bool isTimeShift, bool appendMode);
    ~DVRRecord();

    int start(bool isSetStreams=true);
    int setStreams();
    int stop();
    void signalQuit();
    void DVRRecorderRegisterEventCallback(Aml_MP_DVRRecorderEventCallback cb, void* userData);

protected:
    virtual const Command* getCommandTable() const override;
    virtual void* getCommandHandle() const override;

private:
    int initDVREncryptRecord(Aml_MP_DVRRecorderEncryptParams& encryptParams);
    int uninitDVREncryptRecord();

private:
    const std::string mUrl;
    const bool mCryptoMode;
    const Aml_MP_DemuxId mDemuxId;
    const sptr<ProgramInfo> mProgramInfo;
    AML_MP_DVRRECORDER mRecorder = AML_MP_INVALID_HANDLE;
    Aml_MP_DVRRecorderEventCallback mDVRRecorderEventCallback = nullptr;
    void* mUserData = nullptr;

    int maxSize = 20971520;
    int maxTime = 3600000;

    AML_MP_CASSESSION mCasSession = nullptr;
    AML_MP_SECMEM mSecMem = nullptr;

private:
    DVRRecord(const DVRRecord&) = delete;
    DVRRecord& operator=(const DVRRecord&) = delete;
};

}
