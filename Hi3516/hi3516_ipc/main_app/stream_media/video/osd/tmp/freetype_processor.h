/*** 
 * @FilePath     : freetype_processor.h
 * @Author       : huangjunda
 * @Date         : 2025-06-18 10:32:06
 * @LastEditors  : huangjunda
 * @LastEditTime : 2025-06-18 10:32:07
 * @Description  : 
 */
/***
 * @FilePath     : freetype_processor.h
 * @Author       : huangjunda
 * @Date         : 2025-06-10 16:34:34
 * @LastEditors  : huangjunda
 * @LastEditTime : 2025-06-10 16:56:54
 * @Description  :
 */

#ifndef __FREETYPE_PROCESSOR_H__
#define __FREETYPE_PROCESSOR_H__

#pragma once

#include <iostream>
#include <atomic>
#include <list>
#include <map>
#include <thread>
#include <mutex>
#include <iconv.h>
#include <net/if.h>
#include <fcntl.h>
#include <unistd.h>

#include "ft2build.h"
#include "iconv.h"

#include FT_FREETYPE_H
#include FT_BITMAP_H
#include FT_GLYPH_H
#include FT_OUTLINE_H
#include FT_STROKER_H

class CFreetypeProcessor
{
public:
    /* 字符分辨率位图结构体 */
    typedef struct _CHARMAP
    {
        char *pBuffer;
        int nWidth;
        int nHeight;
    } CharMap_S;

    int open_freetype();

    /*
     * @brief        将char类型转化为wchar
     * @author      : zhouzirui
     * @param        {char} *pSrc char类型数据
     * @param        {int} *pWsize 成功返回0 否则返回-1
     * @return       {*} 返回wchar类型数据
     */
    wchar_t *char_to_Wchar(const char *pSrc, int *pWsize);

    /**
     * @brief         freetype 文字转换为ARGB4444位图数据
     * @author      : zhouzirui
     * @param        {FT_Face} *pFace freetype字体指针
     * @param        {FT_Library} *pLib freetype字体库指针
     * @param        {FT_ULong} uCode wchar字符
     * @param        {unsigned int} nFontColor 字体颜色
     * @param        {unsigned int} nBorderColor 边框颜色
     * @param        {unsigned int} nBorder 边框宽度
     * @param        {FT_Bitmap} *pBmp 转换后的位图数据
     * @return       {*} 成功返回0，失败返回-1
     */
    int FT_TextToBitmap_ARGB4444(FT_Face *pFace, FT_Library *pLib, FT_ULong uCode, unsigned int nFontColor, unsigned int nBorderColor, unsigned int nBorder, FT_Bitmap *pBmp);

    /*
     * @brief      将RGBA8888颜色转换为ARGB4444颜色
     * @param      {unsigned int} rgba8888 RGBA8888格式的颜色值
     * @return     {uint16_t} ARGB4444格式的颜色值
     */
    uint16_t RGBA8888ToARGB4444(unsigned int rgba8888);

    /*
     * @brief        将文字转换为argb4444位图
     * @author      : zhouzirui
     * @param        {char} *pTxt pTxt 文字
     * @param        {int} nTxtWidth 像素宽 小于16生成的图片rgn无效
     * @param        {int} nTxtHeight 像素高 小于16生成的图片rgn无效
     * @param        {CharMap_S} *pMap 生成的位图数据
     * @param        {unsigned int} nBackColorkey 背景颜色
     * @param        {unsigned int} nSubtitleColorkey 字幕颜色
     * @param        {int} nW 图片最大宽
     * @param        {int} nH 图片最大高
     * @param        {int} nXpos 偏移宽度
     * @return       {*} 成功返回开启的通道个数，失败返回-1
     */
    int txt2Argb4444(const char *pTxt, int nTxtWidth, int nTxtHeight, CharMap_S *pMap,
                     unsigned int nBackColorkey, unsigned int nSubtitleColorkey,
                     int nW, int nH, int nXpos);

    /*icnov句柄*/
    iconv_t g_stCd;
    /*freetype字体库指针*/
    FT_Library g_pFTLib;
    /*freetype字体指针*/
    FT_Face g_pFTFace;
};
#endif // __FREETYPE_PROCESSOR_H__