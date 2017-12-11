#include "GMAudioTranscode.h"

#define __STDC_CONSTANT_MACROS

extern "C" {
#include "libavformat/avformat.h"
#include "libavformat/avio.h"
#include "libavcodec/avcodec.h"
#include "libavutil/audio_fifo.h"
#include "libavutil/avassert.h"
#include "libavutil/avstring.h"
#include "libavutil/frame.h"
#include "libavutil/opt.h"
#include "libswresample/swresample.h"
};


// 输出文件的码率
#define OUTPUT_BIT_RATE 96000
// 输出文件的声道数
#define OUTPUT_CHANNELS 2

