/**
 * @FilePath     : ffmpeg_image.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2025-08-18 11:36:15
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2025-10-30 15:48:22
 * @Description  : 封装FFmpeg图像编码功能的类（仅支持YVU420SP输入）
 */

#include "ffmpeg_image.h"
#include <stdexcept>
#include <algorithm>
#include <cstring>
#include <sys/stat.h>
#include <iostream>

CFfmpegImage::CFfmpegImage()
    : m_pFmtCtx(nullptr), m_pCodecCtx(nullptr), m_pFrame(nullptr), m_pPkt(nullptr)
{
}

CFfmpegImage::~CFfmpegImage()
{
    Close();
}

bool CFfmpegImage::CreateDirectoryRecursive(const std::string &path)
{
    if (path.empty())
        return false;

    /* 检查目录是否已存在 */
    struct stat info;
    if (stat(path.c_str(), &info) == 0 && S_ISDIR(info.st_mode))
    {
        return true;
    }

    /* 递归创建父目录 */
    size_t pos = path.find_last_of('/');
    if (pos != std::string::npos)
    {
        std::string parent = path.substr(0, pos);
        if (!CreateDirectoryRecursive(parent))
        {
            return false;
        }
    }

    /* 创建当前目录 */
    return mkdir(path.c_str(), 0755) == 0;
}

bool CFfmpegImage::Open(const std::string &strFilename, int nWidth, int nHeight)
{
    try
    {
        m_strFilename = strFilename;
        m_nWidth = nWidth;
        m_nHeight = nHeight;

        /* 确保目录存在 */
        size_t pos = strFilename.find_last_of('/');
        if (pos != std::string::npos)
        {
            std::string dir = strFilename.substr(0, pos);
            if (!CreateDirectoryRecursive(dir))
            {
                std::cerr << "Failed to create directory: " << dir << std::endl;
                return false;
            }
        }

        int ret;

        /* 创建输出上下文 */
        ret = avformat_alloc_output_context2(&m_pFmtCtx, nullptr, "image2", strFilename.c_str());
        if (ret < 0 || !m_pFmtCtx)
        {
            std::cerr << "avformat_alloc_output_context2 failed: " << ret << std::endl;
            return false;
        }

        /* 根据文件名后缀选择编码器 */
        std::string ext;
        size_t dot = strFilename.find_last_of('.');
        if (dot != std::string::npos)
        {
            ext = strFilename.substr(dot + 1);
            std::transform(ext.begin(), ext.end(), ext.begin(),
                           [](unsigned char c)
                           {
                               return std::tolower(c);
                           });
        }

        const AVCodec *codec = nullptr;
        if (ext == "jpg" || ext == "jpeg")
        {
            codec = avcodec_find_encoder_by_name("mjpeg");
        }
        else if (ext == "png")
        {
            codec = avcodec_find_encoder_by_name("png");
        }
        else
        {
            std::cerr << "Unsupported image extension: " << ext << std::endl;
            return false;
        }

        if (!codec)
        {
            std::cerr << "Encoder not found for ext: " << ext << std::endl;
            return false;
        }

        /* 新建一个视频流 */
        AVStream *st = avformat_new_stream(m_pFmtCtx, codec);
        if (!st)
        {
            std::cerr << "avformat_new_stream failed" << std::endl;
            return false;
        }

        /* 创建编码器上下文 */
        m_pCodecCtx = avcodec_alloc_context3(codec);
        if (!m_pCodecCtx)
        {
            std::cerr << "avcodec_alloc_context3 failed" << std::endl;
            return false;
        }

        m_pCodecCtx->codec_id = codec->id;
        m_pCodecCtx->codec_type = AVMEDIA_TYPE_VIDEO;
        m_pCodecCtx->width = nWidth;
        m_pCodecCtx->height = nHeight;
        m_pCodecCtx->time_base = AVRational{ 1, 25 };

        /* 不同格式需要的像素格式 */
        if (ext == "jpg" || ext == "jpeg")
        {
            m_pCodecCtx->pix_fmt = AV_PIX_FMT_YUVJ420P;  /* JPEG需要YUVJ420P */
            m_pCodecCtx->color_range = AVCOL_RANGE_JPEG; /* JPEG色彩范围 */
            /* 设置JPEG质量参数，避免文件大小波动 */
            // m_pCodecCtx->qmin = 2;                          /* 最小量化参数 */
            // m_pCodecCtx->qmax = 10;                         /* 最大量化参数（越小质量越高）*/
            // m_pCodecCtx->bit_rate = 0;                      /* 使用质量模式而非码率模式 */
            // m_pCodecCtx->flags |= AV_CODEC_FLAG_QSCALE;     /* 启用质量模式 */
            // m_pCodecCtx->global_quality = FF_QP2LAMBDA * 2; /* 全局质量 */
        }
        else if (ext == "png")
        {
            m_pCodecCtx->pix_fmt = AV_PIX_FMT_RGB24; /* PNG常用RGB24 */
            /* PNG是无损压缩，设置压缩级别 */
            // m_pCodecCtx->compression_level = 5; /* 0-9，数值越大压缩率越高但速度越慢 */
        }

        /* 将参数拷贝到 stream */
        ret = avcodec_parameters_from_context(st->codecpar, m_pCodecCtx);
        if (ret < 0)
        {
            std::cerr << "avcodec_parameters_from_context failed: " << ret << std::endl;
            return false;
        }

        /* 打开编码器 */
        ret = avcodec_open2(m_pCodecCtx, codec, nullptr);
        if (ret < 0)
        {
            std::cerr << "avcodec_open2 failed: " << ret << std::endl;
            return false;
        }

        /* 分配帧 */
        m_pFrame = av_frame_alloc();
        if (!m_pFrame)
        {
            std::cerr << "av_frame_alloc failed" << std::endl;
            return false;
        }

        m_pFrame->format = m_pCodecCtx->pix_fmt;
        m_pFrame->width = nWidth;
        m_pFrame->height = nHeight;
        ret = av_frame_get_buffer(m_pFrame, 1); /* 使用1字节对齐 */
        if (ret < 0)
        {
            std::cerr << "av_frame_get_buffer failed: " << ret << std::endl;
            return false;
        }

        /* 分配包 */
        m_pPkt = av_packet_alloc();
        if (!m_pPkt)
        {
            std::cerr << "av_packet_alloc failed" << std::endl;
            return false;
        }

        /* 打开输出文件 */
        if (!(m_pFmtCtx->oformat->flags & AVFMT_NOFILE))
        {
            ret = avio_open(&m_pFmtCtx->pb, strFilename.c_str(), AVIO_FLAG_WRITE);
            if (ret < 0)
            {
                std::cerr << "avio_open failed: " << ret << ", file: " << strFilename << std::endl;
                return false;
            }
        }

        /* 写文件头 */
        ret = avformat_write_header(m_pFmtCtx, nullptr);
        if (ret < 0)
        {
            std::cerr << "avformat_write_header failed: " << ret << std::endl;
            return false;
        }

        return true;
    }
    catch (const std::exception &e)
    {
        std::cerr << "Exception in Open: " << e.what() << std::endl;
        return false;
    }
}

void CFfmpegImage::FillFrameFromYVU420SP(const char *pData)
{
    int y_size = m_nWidth * m_nHeight;

    const uint8_t *srcY = reinterpret_cast<const uint8_t *>(pData);
    const uint8_t *srcVU = srcY + y_size; /* VU交错数据起始位置 */

    /* 拷贝Y平面 */
    for (int i = 0; i < m_nHeight; i++)
    {
        memcpy(m_pFrame->data[0] + i * m_pFrame->linesize[0], srcY + i * m_nWidth, m_nWidth);
    }

    /* 拆分VU -> U/V 平面 */
    uint8_t *dstU = m_pFrame->data[1];  /* U分量目标地址 */
    uint8_t *dstV = m_pFrame->data[2];  /* V分量目标地址 */

    int uvHeight = m_nHeight / 2;
    int uvWidth = m_nWidth / 2;

    for (int j = 0; j < uvHeight; j++)
    {
        const uint8_t *srcVU_row = srcVU + j * m_nWidth; /* 当前行的VU数据起始 */
        uint8_t *dstU_row = dstU + j * m_pFrame->linesize[1];
        uint8_t *dstV_row = dstV + j * m_pFrame->linesize[2];

        for (int i = 0; i < uvWidth; i++)
        {
            /* YVU420SP格式: VU交错存储 VUVUVU... */
            dstV_row[i] = srcVU_row[2 * i + 0]; /* V分量 */
            dstU_row[i] = srcVU_row[2 * i + 1]; /* U分量 */
        }
    }
}

bool CFfmpegImage::SendFrame(const char *pData, int nDataSize)
{
    if (!pData || nDataSize < m_nWidth * m_nHeight * 3 / 2)
    {
        std::cerr << "Invalid input data" << std::endl;
        return false;
    }

    if (!m_pFrame || !m_pCodecCtx)
    {
        std::cerr << "Not properly initialized" << std::endl;
        return false;
    }

    int ret = av_frame_make_writable(m_pFrame);
    if (ret < 0)
    {
        std::cerr << "av_frame_make_writable failed: " << ret << std::endl;
        return false;
    }

    /* YVU420SP -> YUV420P */
    FillFrameFromYVU420SP(pData);

    m_pFrame->pts = 0;
    /* 设置帧的质量参数 */
    // m_pFrame->quality = m_pCodecCtx->global_quality;

    /* 送帧编码 */
    ret = avcodec_send_frame(m_pCodecCtx, m_pFrame);
    if (ret < 0)
    {
        std::cerr << "avcodec_send_frame failed: " << ret << std::endl;
        return false;
    }

    /* 循环取包并写入文件 */
    bool success = false;
    while (ret >= 0)
    {
        ret = avcodec_receive_packet(m_pCodecCtx, m_pPkt);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
        {
            success = true; // 正常结束
            break;
        }
        else if (ret < 0)
        {
            std::cerr << "avcodec_receive_packet failed: " << ret << std::endl;
            return false;
        }

        /* 写文件 */
        ret = av_interleaved_write_frame(m_pFmtCtx, m_pPkt);
        if (ret < 0)
        {
            std::cerr << "av_interleaved_write_frame failed: " << ret << std::endl;
            av_packet_unref(m_pPkt);
            return false;
        }

        av_packet_unref(m_pPkt);
        success = true;
    }

    return success;
}

void CFfmpegImage::Close()
{
    if (m_pFmtCtx)
    {
        /* 刷新编码器 */
        if (m_pCodecCtx)
        {
            avcodec_send_frame(m_pCodecCtx, nullptr); // 发送NULL刷新

            /* 获取剩余的包 */
            int ret;
            while ((ret = avcodec_receive_packet(m_pCodecCtx, m_pPkt)) >= 0)
            {
                av_interleaved_write_frame(m_pFmtCtx, m_pPkt);
                av_packet_unref(m_pPkt);
            }
        }

        /* 写文件尾 */
        av_write_trailer(m_pFmtCtx);

        /*  关闭文件 */
        if (!(m_pFmtCtx->oformat->flags & AVFMT_NOFILE))
        {
            avio_close(m_pFmtCtx->pb);
        }

        avformat_free_context(m_pFmtCtx);
        m_pFmtCtx = nullptr;
    }

    if (m_pCodecCtx)
    {
        avcodec_free_context(&m_pCodecCtx);
        m_pCodecCtx = nullptr;
    }

    if (m_pFrame)
    {
        av_frame_free(&m_pFrame);
        m_pFrame = nullptr;
    }

    if (m_pPkt)
    {
        av_packet_free(&m_pPkt);
        m_pPkt = nullptr;
    }
}
