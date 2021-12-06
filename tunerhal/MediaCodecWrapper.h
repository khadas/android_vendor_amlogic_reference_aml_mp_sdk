#ifndef MEDIACODEC_WRAPPER_H_
#define MEDIACODEC_WRAPPER_H_

#include <media/NdkMediaCodec.h>
#include <media/NdkMediaFormat.h>
#include <media/NdkMediaFormatPriv.h>

#include <utils/AmlMpUtils.h>
#include <utils/AmlMpRefBase.h>
#include "Aml_MP/Common.h"


struct ANativeWindow;
struct AMediaCodec;
struct AMediaFormat;

namespace aml_mp {

struct MediaCodecParams {
    Aml_MP_CodecID codecId;
    ANativeWindow *nativewindow;
    // TunerFramework params begin
    int filterId;
    int hwAvSyncId;
    bool isPassthrough;
    // TunerFramework params end
};

class MediaCodecWrapper : public AmlMpRefBase {
public:
    MediaCodecWrapper();
    ~MediaCodecWrapper();
    void configure(MediaCodecParams mediaCodecParams);
    void start();
    size_t writeData(const uint8_t *data, size_t size);
    void flush();
    void stop();
    void release();
private:
    char mName[50];
    AMediaCodec* mCodec = nullptr;
    AMediaFormat* mFormat = nullptr;
    MediaCodecParams mMediaCodecParams;
};

} // namespace aml_mp

#endif /* MEDIACODEC_WRAPPER_H_ */