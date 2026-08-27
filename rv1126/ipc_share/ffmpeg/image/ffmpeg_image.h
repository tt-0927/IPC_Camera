/**
 * @FilePath     : ffmpeg_image.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2025-08-18 11:36:11
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-08-21 09:24:25
 * @Description  : 封装FFmpeg图像编码功能的类（仅支持YVU420SP输入）
 */

#pragma once

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/opt.h>
}

#include <string>

/**
 * @brief   : FFmpeg图像编码封装类
 * @note    : 支持将YVU420SP格式的图像数据编码为JPEG或PNG格式文件
 */
class CFfmpegImage
{
public:
    CFfmpegImage();
    ~CFfmpegImage();

    /**
     * @brief   : 初始化编码器并打开输出文件
     * @param   {std::string} &strFilename：输出文件路径
     * @param   {int} nWidth：图像宽度
     * @param   {int} nHeight：图像高度
     * @return  {bool} 成功返回true，失败返回false
     * @note    : 支持jpg/jpeg和png格式，根据文件后缀自动选择编码器
     */
    bool Open(const std::string &strFilename, int nWidth, int nHeight);

    /**
     * @brief   : 设置编码质量
     * @param   {int} nQuality：JPEG质量值（1-100，100为最佳），仅对JPEG编码生效
     * @note    : 必须在调用Open()之前调用才能生效；人脸抓拍场景建议设为95
     */
    void SetQuality(int nQuality);

    /**
     * @brief   : 发送图像数据进行编码并写入文件
     * @param   {const char*} pData：YVU420SP格式的图像数据
     * @param   {int} nDataSize：输入数据大小
     * @return  {bool} 成功返回true，失败返回false
     * @note    : 输入数据大小应不小于 width * height * 3 / 2（YVU420SP格式的大小）
     */
    bool SendFrame(const char *pData, int nDataSize);

    /**
     * @brief   : 关闭编码器并释放所有资源
     * @note    : 会完成编码过程，写入文件尾并释放所有FFmpeg相关结构体
     */
    void Close();

private:
    /**
     * @brief   : 将YVU420SP格式数据填充到编码帧
     * @param   {const char*} pData：YVU420SP格式的图像数据指针
     * @note    : YVU420SP格式说明：Y平面 + 交错的VU平面(VUVUVU...)
     */
    void FillFrameFromYVU420SP(const char *pData);

    /**
     * @brief   : 递归创建目录
     * @param   {std::string} &path：要创建的目录路径
     * @return  {bool} 成功返回true，失败返回false
     * @note    : 若目录已存在则直接返回成功
     */
    bool CreateDirectoryRecursive(const std::string &path);

private:
    AVFormatContext *m_pFmtCtx;  /* 格式上下文 */
    AVCodecContext *m_pCodecCtx; /* 编码器上下文 */
    AVFrame *m_pFrame;           /* 图像帧结构 */
    AVPacket *m_pPkt;            /* 编码数据包 */
    std::string m_strFilename;   /* 输出文件名 */
    int m_nWidth;                /* 图像宽度 */
    int m_nHeight;               /* 图像高度 */
    int m_nQuality;              /* 编码质量（1-100，0表示默认），仅JPEG生效 */
};
