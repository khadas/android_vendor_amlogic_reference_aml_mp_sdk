/*
 * Copyright (c) 2020 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#include <utils/AmlMpRefBase.h>
#include <list>

namespace aml_mp {
struct AmlMpBuffer;

struct ElementaryStreamQueue : public AmlMpRefBase
{
    enum Mode {
        INVALID = 0,
        H264,
        H265,
        AAC,
        AC3,
        EAC3,
        AC4,
        MPEG_AUDIO,
        MPEG_VIDEO,
        MPEG4_VIDEO,
        PCM_AUDIO,
        METADATA,
    };

    explicit ElementaryStreamQueue(Mode mode);
    int appendData(const void* data, size_t size, int64_t pts, int32_t payloadOffset);
    void clear();
    sptr<AmlMpBuffer> dequeueAccessUnit();

private:
    struct RangeInfo {
        int64_t mPts;
        size_t mLength;
        int32_t mPesOffset;
    };

    Mode mMode;
    sptr<AmlMpBuffer> mBuffer;
    std::list<RangeInfo> mRangeInfos;

    sptr<AmlMpBuffer> mFormat;

    int mAUIndex = 0;

    sptr<AmlMpBuffer> dequeueAccessUnitH264();
    sptr<AmlMpBuffer> dequeueAccessUnitH265();
    sptr<AmlMpBuffer> dequeueAccessUnitAAC();
    sptr<AmlMpBuffer> dequeueAccessUnitEAC3();
    sptr<AmlMpBuffer> dequeueAccessUnitAC4();
    sptr<AmlMpBuffer> dequeueAccessUnitMPEGAudio();
    sptr<AmlMpBuffer> dequeueAccessUnitMPEGVideo();
    sptr<AmlMpBuffer> dequeueAccessUnitMPEG4Video();
    sptr<AmlMpBuffer> dequeueAccessUnitPCMAudio();
    sptr<AmlMpBuffer> dequeueAccessUnitMetadata();

    int64_t fetchTimestamp(size_t size, int32_t *pesOffset = NULL);
    size_t makePESHeader(uint8_t* data, size_t size, size_t frameLength, int64_t pts);

private:
    ElementaryStreamQueue(const ElementaryStreamQueue&) = delete;
    ElementaryStreamQueue& operator=(const ElementaryStreamQueue&) = delete;
};

}
