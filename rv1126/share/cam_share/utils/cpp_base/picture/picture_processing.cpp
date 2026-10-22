/**
 * @FilePath     : picture_processing.cpp
 * @Author       : zhouzirui
 * @Date         : 2025-03-07 14:03:01
 * @LastEditors  : zhouzirui
 * @LastEditTime : 2025-03-07 15:58:57
 * @Description  : 图片处理
 */
#include "picture_processing.h"
#include "dlog.h"

unsigned char *picture_processing_loadFFmpeg(const char *filename, int &width, int &height, int &channels)
{
    avformat_network_init();
    AVFormatContext *fmtCtx = avformat_alloc_context();

    if (avformat_open_input(&fmtCtx, filename, nullptr, nullptr) != 0)
    {
        dlog(LOG_ERROR,"无法打开图像: %s",filename);
        return nullptr;
    }

    if (avformat_find_stream_info(fmtCtx, nullptr) < 0)
    {
        dlog(LOG_ERROR,"无法获取流信息");
        avformat_close_input(&fmtCtx);
        return nullptr;
    }

    AVCodecContext *codecCtx = nullptr;
    AVCodec *codec = nullptr;
    int streamIndex = -1;

    for (unsigned int i = 0; i < fmtCtx->nb_streams; i++)
    {
        if (fmtCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
        {
            codec = avcodec_find_decoder(fmtCtx->streams[i]->codecpar->codec_id);
            if (!codec)
            {
                dlog(LOG_ERROR,"找不到解码器");
                avformat_close_input(&fmtCtx);
                return nullptr;
            }
            codecCtx = avcodec_alloc_context3(codec);
            avcodec_parameters_to_context(codecCtx, fmtCtx->streams[i]->codecpar);
            if (avcodec_open2(codecCtx, codec, nullptr) != 0)
            {
                dlog(LOG_ERROR,"无法打开解码器");
                avcodec_free_context(&codecCtx);
                avformat_close_input(&fmtCtx);
                return nullptr;
            }
            streamIndex = i;
            break;
        }
    }

    if (streamIndex == -1)
    {
        dlog(LOG_ERROR,"未找到视频流");
        avcodec_free_context(&codecCtx);
        avformat_close_input(&fmtCtx);
        return nullptr;
    }

    AVFrame *frame = av_frame_alloc();
    AVPacket packet;

    int ret = 0;
    while (av_read_frame(fmtCtx, &packet) >= 0)
    {
        if (packet.stream_index == streamIndex)
        {
            ret = avcodec_send_packet(codecCtx, &packet);
            if (ret < 0)
                break;

            ret = avcodec_receive_frame(codecCtx, frame);
            if (ret == 0)
                break;
        }
        av_packet_unref(&packet);
    }

    av_packet_unref(&packet);

    width = codecCtx->width;
    height = codecCtx->height;
    channels = 4; // BGRA

    // 处理已废弃的 YUVJ 格式
    AVPixelFormat pixFmt = codecCtx->pix_fmt;
    if (pixFmt == AV_PIX_FMT_YUVJ420P)
        pixFmt = AV_PIX_FMT_YUV420P;
    if (pixFmt == AV_PIX_FMT_YUVJ422P)
        pixFmt = AV_PIX_FMT_YUV422P;
    if (pixFmt == AV_PIX_FMT_YUVJ444P)
        pixFmt = AV_PIX_FMT_YUV444P;

    SwsContext *swsCtx = sws_getContext(width, height, pixFmt, width, height, AV_PIX_FMT_BGRA,
                                        SWS_BICUBIC, nullptr, nullptr, nullptr);
    if (!swsCtx)
    {
        dlog(LOG_ERROR,"无法创建图像转换上下文");
        av_frame_free(&frame);
        avcodec_free_context(&codecCtx);
        avformat_close_input(&fmtCtx);
        return nullptr;
    }

    unsigned char *pData = (unsigned char *)malloc(width * height * 4);
    if (!pData)
    {
        dlog(LOG_ERROR,"内存分配失败");
        sws_freeContext(swsCtx);
        av_frame_free(&frame);
        avcodec_free_context(&codecCtx);
        avformat_close_input(&fmtCtx);
        return nullptr;
    }

    uint8_t *dest[1] = {pData};
    int dest_linesize[1] = {width * 4};

    sws_scale(swsCtx, frame->data, frame->linesize, 0, height, dest, dest_linesize);

    sws_freeContext(swsCtx);
    av_frame_free(&frame);
    avcodec_free_context(&codecCtx);
    avformat_close_input(&fmtCtx);

    return pData;
}

void picture_processing_gaussianBlur(unsigned char *img, unsigned int width, unsigned int height, unsigned int channels, unsigned int radius)
{
    radius = min(max(1, radius), 248);
    unsigned int kernelSize = 1 + radius * 2;
    unsigned int *kernel = (unsigned int *)malloc(kernelSize * sizeof(unsigned int));
    memset(kernel, 0, kernelSize * sizeof(unsigned int));
    int(*mult)[256] = (int(*)[256])malloc(kernelSize * 256 * sizeof(int));
    memset(mult, 0, kernelSize * 256 * sizeof(int));

    int xStart = 0;
    int yStart = 0;
    width = xStart + width - max(0, (xStart + width) - width);
    height = yStart + height - max(0, (yStart + height) - height);
    int imageSize = width * height;
    int widthstep = width * channels;
    if (channels == 3 || channels == 4)
    {
        unsigned char *CacheImg = nullptr;
        CacheImg = (unsigned char *)malloc(sizeof(unsigned char) * imageSize * 6);
        if (CacheImg == nullptr)
            return;
        unsigned char *rCache = CacheImg;
        unsigned char *gCache = CacheImg + imageSize;
        unsigned char *bCache = CacheImg + imageSize * 2;
        unsigned char *r2Cache = CacheImg + imageSize * 3;
        unsigned char *g2Cache = CacheImg + imageSize * 4;
        unsigned char *b2Cache = CacheImg + imageSize * 5;
        int sum = 0;
        for (int K = 1; K < radius; K++)
        {
            unsigned int szi = radius - K;
            kernel[radius + K] = kernel[szi] = szi * szi;
            sum += kernel[szi] + kernel[szi];
            for (int j = 0; j < 256; j++)
            {
                mult[radius + K][j] = mult[szi][j] = kernel[szi] * j;
            }
        }
        kernel[radius] = radius * radius;
        sum += kernel[radius];
        for (int j = 0; j < 256; j++)
        {
            mult[radius][j] = kernel[radius] * j;
        }
        for (int Y = 0; Y < height; ++Y)
        {
            unsigned char *LinePS = img + Y * widthstep;
            unsigned char *LinePR = rCache + Y * width;
            unsigned char *LinePG = gCache + Y * width;
            unsigned char *LinePB = bCache + Y * width;
            for (int X = 0; X < width; ++X)
            {
                int p2 = X * channels;
                LinePR[X] = LinePS[p2];
                LinePG[X] = LinePS[p2 + 1];
                LinePB[X] = LinePS[p2 + 2];
            }
        }
        int kernelsum = 0;
        for (int K = 0; K < kernelSize; K++)
        {
            kernelsum += kernel[K];
        }
        float fkernelsum = 1.0f / kernelsum;
        for (int Y = yStart; Y < height; Y++)
        {
            int heightStep = Y * width;
            unsigned char *LinePR = rCache + heightStep;
            unsigned char *LinePG = gCache + heightStep;
            unsigned char *LinePB = bCache + heightStep;
            for (int X = xStart; X < width; X++)
            {
                int cb = 0;
                int cg = 0;
                int cr = 0;
                for (int K = 0; K < kernelSize; K++)
                {
                    unsigned int readPos = ((X - radius + K + width) % width);
                    int *pmult = mult[K];
                    cr += pmult[LinePR[readPos]];
                    cg += pmult[LinePG[readPos]];
                    cb += pmult[LinePB[readPos]];
                }
                unsigned int p = heightStep + X;
                r2Cache[p] = cr * fkernelsum;
                g2Cache[p] = cg * fkernelsum;
                b2Cache[p] = cb * fkernelsum;
            }
        }
        for (int X = xStart; X < width; X++)
        {
            int WidthComp = X * channels;
            int WidthStep = width * channels;
            unsigned char *LinePS = img + X * channels;
            unsigned char *LinePR = r2Cache + X;
            unsigned char *LinePG = g2Cache + X;
            unsigned char *LinePB = b2Cache + X;
            for (int Y = yStart; Y < height; Y++)
            {
                int cb = 0;
                int cg = 0;
                int cr = 0;
                for (int K = 0; K < kernelSize; K++)
                {
                    unsigned int readPos = ((Y - radius + K + height) % height) * width;
                    int *pmult = mult[K];
                    cr += pmult[LinePR[readPos]];
                    cg += pmult[LinePG[readPos]];
                    cb += pmult[LinePB[readPos]];
                }
                int p = Y * WidthStep;
                LinePS[p] = (unsigned char)(cr * fkernelsum);
                LinePS[p + 1] = (unsigned char)(cg * fkernelsum);
                LinePS[p + 2] = (unsigned char)(cb * fkernelsum);
            }
        }
        free(CacheImg);
    }
    else if (channels == 1)
    {
        unsigned char *CacheImg = nullptr;
        CacheImg = (unsigned char *)malloc(sizeof(unsigned char) * imageSize * 2);
        if (CacheImg == nullptr)
            return;
        unsigned char *rCache = CacheImg;
        unsigned char *r2Cache = CacheImg + imageSize;

        int sum = 0;
        for (int K = 1; K < radius; K++)
        {
            unsigned int szi = radius - K;
            kernel[radius + K] = kernel[szi] = szi * szi;
            sum += kernel[szi] + kernel[szi];
            for (int j = 0; j < 256; j++)
            {
                mult[radius + K][j] = mult[szi][j] = kernel[szi] * j;
            }
        }
        kernel[radius] = radius * radius;
        sum += kernel[radius];
        for (int j = 0; j < 256; j++)
        {
            mult[radius][j] = kernel[radius] * j;
        }
        for (int Y = 0; Y < height; ++Y)
        {
            unsigned char *LinePS = img + Y * widthstep;
            unsigned char *LinePR = rCache + Y * width;
            for (int X = 0; X < width; ++X)
            {
                LinePR[X] = LinePS[X];
            }
        }
        int kernelsum = 0;
        for (int K = 0; K < kernelSize; K++)
        {
            kernelsum += kernel[K];
        }
        float fkernelsum = 1.0f / kernelsum;
        for (int Y = yStart; Y < height; Y++)
        {
            int heightStep = Y * width;
            unsigned char *LinePR = rCache + heightStep;
            for (int X = xStart; X < width; X++)
            {
                int cb = 0;
                int cg = 0;
                int cr = 0;
                for (int K = 0; K < kernelSize; K++)
                {
                    unsigned int readPos = ((X - radius + K + width) % width);
                    int *pmult = mult[K];
                    cr += pmult[LinePR[readPos]];
                }
                unsigned int p = heightStep + X;
                r2Cache[p] = cr * fkernelsum;
            }
        }
        for (int X = xStart; X < width; X++)
        {
            int WidthComp = X * channels;
            int WidthStep = width * channels;
            unsigned char *LinePS = img + X * channels;
            unsigned char *LinePR = r2Cache + X;
            for (int Y = yStart; Y < height; Y++)
            {
                int cb = 0;
                int cg = 0;
                int cr = 0;
                for (int K = 0; K < kernelSize; K++)
                {
                    unsigned int readPos = ((Y - radius + K + height) % height) * width;
                    int *pmult = mult[K];
                    cr += pmult[LinePR[readPos]];
                }
                int p = Y * WidthStep;
                LinePS[p] = (unsigned char)(cr * fkernelsum);
            }
        }
        free(CacheImg);
    }
    free(kernel);
    free(mult);
}

void picture_processing_fastGaussianBlurRGB888(unsigned char *data, int width, int height)
{
    int kernel_size = 5;                                             // 5x5 高斯核
    float kernel[5] = {0.06136, 0.24477, 0.38774, 0.24477, 0.06136}; // 一维高斯核
    int radius = kernel_size / 2;

    unsigned char *temp = (unsigned char *)malloc(width * height * 3);
    if (!temp)
        return;
    memcpy(temp, data, width * height * 3);

    // 水平方向模糊
    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            float sumR = 0, sumG = 0, sumB = 0;
            float weightSum = 0;

            for (int k = -radius; k <= radius; k++)
            {
                int nx = x + k;
                if (nx >= 0 && nx < width)
                {
                    int index = (y * width + nx) * 3;
                    float weight = kernel[k + radius];
                    sumR += temp[index] * weight;
                    sumG += temp[index + 1] * weight;
                    sumB += temp[index + 2] * weight;
                    weightSum += weight;
                }
            }
            int idx = (y * width + x) * 3;
            data[idx] = (unsigned char)(sumR / weightSum);
            data[idx + 1] = (unsigned char)(sumG / weightSum);
            data[idx + 2] = (unsigned char)(sumB / weightSum);
        }
    }

    // 竖直方向模糊
    memcpy(temp, data, width * height * 3);
    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            float sumR = 0, sumG = 0, sumB = 0;
            float weightSum = 0;

            for (int k = -radius; k <= radius; k++)
            {
                int ny = y + k;
                if (ny >= 0 && ny < height)
                {
                    int index = (ny * width + x) * 3;
                    float weight = kernel[k + radius];
                    sumR += temp[index] * weight;
                    sumG += temp[index + 1] * weight;
                    sumB += temp[index + 2] * weight;
                    weightSum += weight;
                }
            }
            int idx = (y * width + x) * 3;
            data[idx] = (unsigned char)(sumR / weightSum);
            data[idx + 1] = (unsigned char)(sumG / weightSum);
            data[idx + 2] = (unsigned char)(sumB / weightSum);
        }
    }

    free(temp);
}
