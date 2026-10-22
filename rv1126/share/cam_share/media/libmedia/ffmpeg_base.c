

#include "ffmpeg_base.h"

int ffmpegMedia_init()
{
    static int isinit = 0;
    if(isinit == 0)
    {
        isinit = 1;
        av_register_all();
        avcodec_register_all();
        avfilter_register_all();
        avformat_network_init();

        /* ffmpeg日志等级 */
        av_log_set_level(AV_LOG_WARNING);
//        av_log_set_level(AV_LOG_ERROR);
    }
    return 0;
}






