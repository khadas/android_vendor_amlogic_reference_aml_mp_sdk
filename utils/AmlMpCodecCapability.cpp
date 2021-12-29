/*
 * Copyright (c) 2020 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */
#include "AmlMpUtils.h"
#include "AmlMpCodecCapability.h"
#include "Amlsysfsutils.h"
#include "AmlMpLog.h"

#define LOG_TAG "AmlMpCodecCapability"

typedef struct {
    Aml_MP_CodecID decoderName;
    char decoderNameStr[10];
}DecoderNamePair;

typedef struct {
    Aml_MP_Resolution maxResolution;
    char resolutionStr[10];
}ResolutionNamePair;

#define VIDEO_SUPPORT_INFO_PATH "/sys/class/amstream/vcodec_profile"

namespace aml_mp {

static const char* mName = LOG_TAG;

static const DecoderNamePair mDecoderNameMap[] = {
    {AML_MP_VIDEO_CODEC_H264,   "mh264"},
    {AML_MP_VIDEO_CODEC_HEVC,   "mh265"},
    {AML_MP_VIDEO_CODEC_MJPEG,  "mmjpeg"},
    {AML_MP_VIDEO_CODEC_MPEG12, "mmpeg12"},
    {AML_MP_VIDEO_CODEC_MPEG4,  "mmpeg4"},
    {AML_MP_VIDEO_CODEC_VP9,    "mvp9"},
    {AML_MP_VIDEO_CODEC_AVS2,   "mavs2"},
    {AML_MP_VIDEO_CODEC_AVS,    "mavs"},
    {AML_MP_VIDEO_CODEC_AV1,    "mav1"},
};

static const ResolutionNamePair mResolutionMap[] = {
    {AML_MP_RESOLUTION_8K,      "8k"},
    {AML_MP_RESOLUTION_4K,      "4k"},
    {AML_MP_RESOLUTION_1080P,   "1080p"},
};

AmlMpCodecCapability* AmlMpCodecCapability::mHandle;

AmlMpCodecCapability::AmlMpCodecCapability()
{
    updateVideoCapability();

    Json::Value json;
    getCodecCapabilityJson(AML_MP_STREAM_TYPE_VIDEO, &json);

    Json::FastWriter writer;
    mVideoCabilityJsonStr = writer.write(json);
}

AmlMpCodecCapability::~AmlMpCodecCapability()
{
    MLOGI();
}

AmlMpCodecCapability* AmlMpCodecCapability::getCodecCapabilityHandle() {

    if (mHandle == nullptr) {
        mHandle = new AmlMpCodecCapability();
    }

    return mHandle;
}

bool AmlMpCodecCapability::matchCodecType(std::string str, Aml_MP_CodecID* codecId) {
    for (DecoderNamePair testPair : mDecoderNameMap) {
        if (testPair.decoderNameStr == str) {
            *codecId = testPair.decoderName;
            return true;
        }
    }
    return false;
}

bool AmlMpCodecCapability::matchMaxResolution(std::string str, Aml_MP_Resolution* resolution) {
    *resolution = AML_MP_RESOLUTION_1080P;

    for (ResolutionNamePair testPair : mResolutionMap) {
        if (testPair.resolutionStr == str) {
            *resolution = testPair.maxResolution;
            return true;
        }
    }

    return true;
}

void AmlMpCodecCapability::getVideoDecoderCapability(std::string str){
    std::vector<std::string> splitInfo;
    Aml_MP_CodecID decoderName;

    split(str, splitInfo, " ,:;");

    if (matchCodecType(splitInfo[0], &decoderName)) {
        Aml_MP_DecoderCapabilityInfo info;
        info.decoderName = decoderName;
        if (splitInfo.size() > 1) {
            matchMaxResolution(splitInfo[1], &(info.decoderMaxResolution));
        }
        mVideoCapabilityMap.push_back(info);
    }
}

void AmlMpCodecCapability::updateVideoCapability() {
    char videoSupportInfoStr[2000];
    std::string strOfInfo;
    std::vector<std::string> InfoByLine;

    amsysfs_get_sysfs_str(VIDEO_SUPPORT_INFO_PATH, videoSupportInfoStr, sizeof(videoSupportInfoStr));
    strOfInfo = videoSupportInfoStr;

    split(strOfInfo, InfoByLine, "\n");

    for (std::string tmp : InfoByLine) {
        getVideoDecoderCapability(tmp);
    }
}

void AmlMpCodecCapability::getCodecCapabilityJson(Aml_MP_StreamType streamType, Json::Value* videoInfo) {
    int decoderCount = 0;
    (*videoInfo)["name"] = mpStreamType2Str(streamType);

    Json::Value codecInfoList;
    for (Aml_MP_DecoderCapabilityInfo supportInfo : mVideoCapabilityMap) {
        Json::Value infoPair;
        infoPair["codecName"] = convertToMIMEString(supportInfo.decoderName);
        if (streamType == AML_MP_STREAM_TYPE_VIDEO) {
            infoPair["maxResolution"] = convertToResolutionString(supportInfo.decoderMaxResolution);
        }
        codecInfoList[decoderCount] = infoPair;
        decoderCount++;
    }
    (*videoInfo)["codecList"] = codecInfoList;

/*
    Json::FastWriter writer;

    std::string debugStr = writer.write(*videoInfo);
    MLOGI("json string :%s", debugStr.c_str());
*/
}

void AmlMpCodecCapability::getCodecCapabilityStr(Aml_MP_StreamType streamType, char* str) {
    switch (streamType) {
        case AML_MP_STREAM_TYPE_VIDEO:
        {
            memcpy(str, mVideoCabilityJsonStr.c_str(), mVideoCabilityJsonStr.length()+1);
            break;
        }
    }
}

}