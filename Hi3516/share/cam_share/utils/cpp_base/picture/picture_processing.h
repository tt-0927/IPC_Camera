/**
 * @FilePath     : picture_processing.h
 * @Author       : zhouzirui
 * @Date         : 2025-03-07 14:03:08
 * @LastEditors  : zhouzirui
 * @LastEditTime : 2025-03-07 14:19:29
 * @Description  : 图片处理
 */
#ifndef __PICTURE_PROCESSING_H__
#define __PICTURE_PROCESSING_H__

#include "math.h"

// 定义 min 和 max 宏
#define min(a, b) ((a) < (b) ? (a) : (b))
#define max(a, b) ((a) > (b) ? (a) : (b))

#ifdef __cplusplus
extern "C"
{
#endif

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>

#ifdef __cplusplus
}
#endif

/**
 * @brief       : 使用 FFmpeg 读取图片并获取 BGRA8888格式数据
 * @author      : zhouzirui
 * @param        {char*} filename   图片路径
 * @param        {int&} width       图片宽
 * @param        {int&} height      图片高
 * @param        {int&} channels    通道数
 * @return       {*}    图片数据指针
 */
unsigned char *picture_processing_loadFFmpeg(const char *filename, int &width, int &height, int &channels);

/**
 * @brief       : 高斯模糊处理
 * @author      : zhouzirui
 * @param        {unsigned char} *img 图片数据指针
 * @param        {unsigned int} width 图片宽
 * @param        {unsigned int} height 图片高
 * @param        {unsigned int} channels 通道数
 * @param        {unsigned int} radius 模糊程度 1 - 248
 * @return       {*}
 */
void picture_processing_gaussianBlur(unsigned char *img, unsigned int width, unsigned int height, unsigned int channels, unsigned int radius);

/**
 * @brief       : 快速高斯模糊（使用分离卷积）处理RGB888格式
 * @author      : zhouzirui
 * @param        {unsigned char} *data 图片数据指针
 * @param        {int} width 图片宽
 * @param        {int} height 图片高
 * @return       {*}
 */
void picture_processing_fastGaussianBlurRGB888(unsigned char *data, int width, int height);

#endif // __PICTURE_PROCESSING_H__