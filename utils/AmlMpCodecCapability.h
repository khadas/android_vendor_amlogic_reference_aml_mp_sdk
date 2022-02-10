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
typedef enum {
    AML_MP_RESOLUTION_1080P,
    AML_MP_RESOLUTION_4K,
    AML_MP_RESOLUTION_8K,
    AML_MP_RESOLUTION_MAX,
} Aml_MP_Resolution;

typedef struct {
    Aml_MP_CodecID decoderName;
    Aml_MP_Resolution decoderMaxResolution;
} Aml_MP_DecoderCapabilityInfo;

typedef struct {
    Aml_MP_CodecID decoderName;
    char decoderNameStr[10];
}DecoderNamePair;

typedef struct {
    Aml_MP_Resolution maxResolution;
    char resolutionStr[10];
}ResolutionNamePair;

class AmlMpCodecCapability
{
public:
    static AmlMpCodecCapability* getCodecCapabilityHandle();
    void getCodecCapabilityStr(Aml_MP_CodecID codecId, char* caps, size_t size);
private:
    std::vector<Aml_MP_DecoderCapabilityInfo> mVideoCapabilityMap;
    std::vector<Aml_MP_DecoderCapabilityInfo> mAudioCapabilityMap;
    AmlMpCodecCapability();
    ~AmlMpCodecCapability();
    void updateVideoCapability();
    void updateAudioCapability();

    void getVideoDecoderCapability(std::string str);
    bool matchVideoCodecType(std::string str, Aml_MP_CodecID* codecId);
    bool matchMaxResolution(std::string str, Aml_MP_Resolution* resolution);
    const char* convertToResolutionString(Aml_MP_Resolution resolution);
};
}
