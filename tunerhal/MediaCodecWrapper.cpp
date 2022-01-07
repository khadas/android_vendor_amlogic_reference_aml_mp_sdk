#define LOG_TAG "MediaCodecWrapper"
#include <media/stagefright/MediaCodecList.h>
#include "MediaCodecWrapper.h"

using android::sp;
using android::AMessage;
using android::Vector;
using android::AString;
using android::MediaCodecList;

namespace aml_mp {

static const char *MEDIA_MIMETYPE_VIDEO_AVC = "video/avc";
static const char *MEDIA_MIMETYPE_VIDEO_AVS = "video/avs";
static const char *MEDIA_MIMETYPE_VIDEO_HEVC = "video/hevc";
static const char *MEDIA_MIMETYPE_VIDEO_VP9 = "video/x-vnd.on2.vp9";
//static const char *MEDIA_MIMETYPE_VIDEO_DOLBY_VISION = "video/dolby-vision";
//static const char *MEDIA_MIMETYPE_VIDEO_MPEG = "video/mpeg";
static const char *MEDIA_MIMETYPE_VIDEO_MPEG2 = "video/mpeg2";
static const char *MEDIA_MIMETYPE_VIDEO_MPEG4 = "video/mp4v-es";
static const char *MEDIA_MIMETYPE_VIDEO_MJPEG = "video/mjpeg";

static const char * convertCodecIdToMimeType(Aml_MP_CodecID aml_MP_VideoCodec) {
    switch (aml_MP_VideoCodec) {
        case AML_MP_VIDEO_CODEC_MPEG12:
            return MEDIA_MIMETYPE_VIDEO_MPEG2;
        case AML_MP_VIDEO_CODEC_MPEG4:
            return MEDIA_MIMETYPE_VIDEO_MPEG4;
        case AML_MP_VIDEO_CODEC_MJPEG:
            return MEDIA_MIMETYPE_VIDEO_MJPEG;
        case AML_MP_VIDEO_CODEC_H264:
            return MEDIA_MIMETYPE_VIDEO_AVC;
        case AML_MP_VIDEO_CODEC_AVS:
            return MEDIA_MIMETYPE_VIDEO_AVS;
        case AML_MP_VIDEO_CODEC_VP9:
            return MEDIA_MIMETYPE_VIDEO_VP9;
        case AML_MP_VIDEO_CODEC_HEVC:
            return MEDIA_MIMETYPE_VIDEO_HEVC;
        default:
            break;
    }
    return NULL;
}

MediaCodecWrapper::MediaCodecWrapper() {
    snprintf(mName, sizeof(mName), "%s", LOG_TAG);
    memset(&mMediaCodecParams, 0, sizeof(mMediaCodecParams));
}

MediaCodecWrapper::~MediaCodecWrapper() {
    release();
}

void MediaCodecWrapper::configure(MediaCodecParams mediaCodecParams) {
    mMediaCodecParams = mediaCodecParams;
    // find mediacodec list, prefer the omx.amlogic decoder
    Vector<AString> matchingCodecs;
    size_t index = 0;
    const char* mime = convertCodecIdToMimeType(mediaCodecParams.codecId);
    MediaCodecList::findMatchingCodecs(mime, false, MediaCodecList::Flags::kHardwareCodecsOnly, &matchingCodecs);
    for (index = 0; index < matchingCodecs.size(); index++) {
        if (matchingCodecs[index].startsWithIgnoreCase("omx.amlogic")) {
            break;
        }
    }
    if (index == matchingCodecs.size()) {
        MLOGI("findMatchingCodecs: there is no omx.amlogic decoder");
        index = 0;
    }
    const char * codecName = matchingCodecs[index].c_str();
    MLOGI("AMediaCodec_createCodecByName: %s", codecName);
    mCodec = AMediaCodec_createCodecByName(codecName);
    sp<AMessage> format = new AMessage;
    format->setString("mime", mime);
    format->setInt32("width", 1920);
    format->setInt32("height", 1080);
    format->setInt32("vendor.tunerhal.video-filter-id", mediaCodecParams.filterId);
    format->setInt32("vendor.tunerhal.hw-av-sync-id", mediaCodecParams.hwAvSyncId);
    format->setInt32("feature-tunneled-playback", 1);
    mFormat = AMediaFormat_fromMsg(&format);
    AMediaCodec_configure(mCodec, mFormat, mediaCodecParams.nativewindow, nullptr, 0);
}

void MediaCodecWrapper::start() {
    AMediaCodec_start(mCodec);
}

size_t MediaCodecWrapper::writeData(const uint8_t *data, size_t size) {
    AML_MP_UNUSED(data);
    // for passthrough writeData is not necessary
    return size;
}

void MediaCodecWrapper::flush() {
    AMediaCodec_flush(mCodec);
}

void MediaCodecWrapper::stop() {
    AMediaCodec_stop(mCodec);
}

void MediaCodecWrapper::release() {
    if (mCodec) {
        AMediaCodec_delete(mCodec);
        mCodec = nullptr;
    }
    if (mFormat) {
        AMediaFormat_clear(mFormat);
        AMediaFormat_delete(mFormat);
        mFormat = nullptr;
    }
}

} // namespace aml_mp
