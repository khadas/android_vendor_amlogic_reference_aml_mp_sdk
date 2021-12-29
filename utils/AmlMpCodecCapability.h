/*
 * Copyright (c) 2020 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#include <Aml_MP/Common.h>
#include <string>
#include <vector>
#include "json/json.h"

namespace aml_mp {
class AmlMpCodecCapability
{
public:
    typedef struct {
        Aml_MP_CodecID decoderName;
        Aml_MP_Resolution decoderMaxResolution;
    } Aml_MP_DecoderCapabilityInfo;
    static AmlMpCodecCapability* getCodecCapabilityHandle();
    void getCodecCapabilityStr(Aml_MP_StreamType streamType, char* str);
private:
    std::vector<Aml_MP_DecoderCapabilityInfo> mVideoCapabilityMap;
    std::vector<Aml_MP_DecoderCapabilityInfo> mAudiooCapabilityMap;
    std::string mVideoCabilityJsonStr;
    std::string mAudioCabilityJsonStr;
    AmlMpCodecCapability();
    ~AmlMpCodecCapability();
    static AmlMpCodecCapability* mHandle;
    void getVideoDecoderCapability(std::string str);
    void updateVideoCapability();
    bool matchCodecType(std::string str, Aml_MP_CodecID* codecId);
    bool matchMaxResolution(std::string str, Aml_MP_Resolution* resolution);
    void getCodecCapabilityJson(Aml_MP_StreamType streamType, Json::Value* videoInfo);
};
}