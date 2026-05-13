

#ifndef OS_CORE_SOURCE_FFMPEG_MEDIA_INCLUDE_
#define OS_CORE_SOURCE_FFMPEG_MEDIA_INCLUDE_


#ifdef __cplusplus
extern "C"
{
#endif
#include <libavutil/mathematics.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavcodec/avcodec.h>
#include <libavutil/opt.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
#include <libavfilter/avfilter.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
#include <libavutil/imgutils.h>

int ffmpegMedia_init();



#ifdef __cplusplus
}
#endif

#endif // OS_CORE_SOURCE_FFMPEG_MEDIA_INCLUDE_








