/***
 * @FilePath     : freetype_processor.cpp
 * @Author       : huangjunda
 * @Date         : 2025-06-10 16:34:15
 * @LastEditors  : huangjunda
 * @LastEditTime : 2025-06-10 16:34:16
 * @Description  :
 */

#include "freetype_processor.h"

#include "ini_disposed.h"
#include "data_length.h"
#include "event_define.h"
#include "path_define.h"
#include "IpcRet.h"
#include "dlog.h"

#define CODEC_ALIGN(x, a) (((x) + (a) - 1) & ~((a) - 1))

int CFreetypeProcessor::open_freetype()
{
    char aFile[LENGTH256];
    int nRet = ini_read_profile_char("Font_File", "path", aFile, sizeof(aFile), ITC_FONT_FILE, STREAM_INI);
    if (nRet == 0)
    {
        /* 不存在添加 */
        ini_write_profile_char("Font_File", "path", aFile, STREAM_INI);
    }

    /*判断文件不存在*/
    if (access(aFile, F_OK) != 0)
    {
        strncpy(aFile, ITC_FONT_FILE, sizeof(aFile));
    }

    g_stCd = iconv_open("wchar_t", "UTF-8");
    FT_Error error = FT_Init_FreeType(&g_pFTLib);
    if (error)
    {
        dlog(LOG_ERROR, "init freeType error(%d)", error);
        return -1;
    }
    error = FT_New_Face(g_pFTLib, aFile, 0, &g_pFTFace);
    if (error)
    {
        dlog(LOG_ERROR, "FT_New_Face 失败 %d", error);
        return -1;
    }

    FT_Select_Charmap(g_pFTFace, FT_ENCODING_UNICODE);
    return 0;
}

/*
 * @brief        将char类型转化为wchar
 * @author      : zhouzirui
 * @param        {char} *pSrc char类型数据
 * @param        {int} *pWsize 成功返回0 否则返回-1
 * @return       {*} 返回wchar类型数据
 */
wchar_t *CFreetypeProcessor::char_to_Wchar(const char *pSrc, int *pWsize)
{
    size_t nSrcLen = strlen(pSrc);
    size_t nDstLen = 5 * nSrcLen;
    char *pDst = (char *)malloc(nDstLen);
    memset(pDst, 0, nDstLen);
    char *pIn = (char *)pSrc;
    char *pOut = pDst;
    int nConv = iconv(g_stCd, &pIn, &nSrcLen, &pOut, &nDstLen);
    if (nConv == -1)
    {
        int nErr = errno;
        switch (nErr)
        {
        case E2BIG:
        {
            printf("errno:E2BGI（OutBuf空间不够）\n");
            break;
        }
        case EILSEQ:
        {
            printf("errno:EILSEQ（InBuf多字节序无效）\n");
            break;
        }
        case EINVAL:
        {
            printf("errno:EINVAL（有残留的字节未转换）\n");
            break;
        }
        default:
            break;
        }
        *pWsize = 0;
        return (wchar_t *)pDst;
    }
    *pWsize = (pOut - pDst) / 4;
    return (wchar_t *)pDst;
}

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
int CFreetypeProcessor::FT_TextToBitmap_ARGB4444(FT_Face *pFace, FT_Library *pLib, FT_ULong uCode, unsigned int nFontColor, unsigned int nBorderColor, unsigned int nBorder, FT_Bitmap *pBmp)
{
    unsigned char *pBoderBuf = NULL;
    unsigned char *pFontBuf = NULL;
    FT_Stroker stroker;

    /*字体颜色 ARGB4444*/
    unsigned char nSa = (nFontColor >> 24) & 0xFF;
    unsigned char nSr = (nFontColor >> 16) & 0xFF;
    unsigned char nSg = (nFontColor >> 8) & 0xFF;
    unsigned char nSb = nFontColor & 0xFF;

    /*边框颜色 ARGB4444*/
    unsigned char nDa = (nBorderColor >> 24) & 0xFF;
    unsigned char nDr = (nBorderColor >> 16) & 0xFF;
    unsigned char nDg = (nBorderColor >> 8) & 0xFF;
    unsigned char nDb = nBorderColor & 0xFF;

    FT_UInt nIndex = FT_Get_Char_Index(*pFace, uCode);
    if (!nIndex)
    {
        printf("FT_Get_Char_Index error\n");
        return -1;
    }

    if (FT_Load_Glyph(*pFace, nIndex, FT_LOAD_NO_BITMAP))
    {
        printf("FT_Load_Glyph error\n");
        return -1;
    }

    FT_Glyph glyph;

    if (FT_Get_Glyph((*pFace)->glyph, &glyph))
    {
        printf("FT_Get_Glyph error\n");
        return -1;
    }

    if (nBorder > 0)
    {
        if (FT_Stroker_New(*pLib, &stroker))
        {
            printf("FT_Stroker_New error\n");
            return -1;
        }

        FT_Stroker_Set(stroker, (int)(nBorder * 64), FT_STROKER_LINECAP_ROUND, FT_STROKER_LINEJOIN_ROUND, 0);

        if (FT_Glyph_StrokeBorder(&glyph, stroker, 0, 1))
        {
            printf("FT_Glyph_StrokeBorder error\n");
            return -1;
        }
    }

    FT_Outline *outline = &((FT_OutlineGlyph)glyph)->outline;
    FT_BBox bbox;
    FT_Glyph_Get_CBox(glyph, FT_GLYPH_BBOX_GRIDFIT, &bbox);

    int nWidth = (bbox.xMax - bbox.xMin) >> 6;
    int nRows = (bbox.yMax - bbox.yMin) >> 6;

    pBmp->width = nWidth;
    pBmp->rows = nRows;
    // 为ARGB4444格式分配内存，每个像素2字节
    pBmp->buffer = (unsigned char *)malloc(nWidth * nRows * 2);
    memset(pBmp->buffer, 0, nWidth * nRows * 2);

    FT_Raster_Params params;

    if (nBorder > 0)
    {
        FT_Bitmap stBoderbmp;
        stBoderbmp.buffer = (unsigned char *)malloc(nWidth * nRows);
        memset(stBoderbmp.buffer, 0, nWidth * nRows);
        stBoderbmp.width = nWidth;
        stBoderbmp.rows = nRows;
        stBoderbmp.pitch = nWidth;
        stBoderbmp.pixel_mode = FT_PIXEL_MODE_GRAY;
        stBoderbmp.num_grays = 256;

        memset(&params, 0, sizeof(params));
        params.source = outline;
        params.target = &stBoderbmp;
        params.flags = FT_RASTER_FLAG_AA;
        FT_Outline_Translate(outline, -bbox.xMin, -bbox.yMin);
        FT_Outline_Render(*pLib, outline, &params);
        pBoderBuf = stBoderbmp.buffer;
    }

    FT_BBox bbox_in;
    FT_Glyph glyph_fg;
    FT_Get_Glyph((*pFace)->glyph, &glyph_fg);
    FT_Glyph_Get_CBox(glyph_fg, FT_GLYPH_BBOX_GRIDFIT, &bbox_in);

    FT_Bitmap stFontbmp;
    stFontbmp.buffer = (unsigned char *)malloc(nWidth * nRows);
    memset(stFontbmp.buffer, 0, nWidth * nRows);
    stFontbmp.width = nWidth;
    stFontbmp.rows = nRows;
    stFontbmp.pitch = nWidth;
    stFontbmp.pixel_mode = FT_PIXEL_MODE_GRAY;
    stFontbmp.num_grays = 256;
    outline = &((FT_OutlineGlyph)glyph_fg)->outline;
    memset(&params, 0, sizeof(params));
    params.source = outline;
    params.target = &stFontbmp;
    params.flags = FT_RASTER_FLAG_AA;
    FT_Outline_Translate(outline, -bbox.xMin, -bbox.yMin);
    FT_Outline_Render(*pLib, outline, &params);
    pFontBuf = stFontbmp.buffer;

    int pitch = nWidth;
    uint16_t *pArgb4444 = (uint16_t *)pBmp->buffer;

    for (int yy = 0; yy < nRows; ++yy)
    {
        for (int xx = 0; xx < nWidth; ++xx)
        {
            int nSi = yy * nWidth + xx; // ARGB4444格式索引

            /*画边框*/
            if (nBorder > 0)
            {
                int nBoderAlpha = pBoderBuf[yy * pitch + xx] * nDa / 255.0f;
                if (nBoderAlpha)
                {
                    // 将边框颜色转换为ARGB4444格式
                    uint16_t borderColor4444 = ((nBoderAlpha & 0xF0) << 8) | // A (高4位)
                                               ((nDr & 0xF0) << 4) |         // R (高4位)
                                               (nDg & 0xF0) |                // G (高4位)
                                               ((nDb & 0xF0) >> 4);          // B (高4位)
                    pArgb4444[nSi] = borderColor4444;
                }
            }

            /*画字体*/
            int nFontAlpha = pFontBuf[yy * pitch + xx] * nSa / 255.0f;
            if (nFontAlpha)
            {
                // 将字体颜色转换为ARGB4444格式
                uint16_t fontColor4444 = ((nFontAlpha & 0xF0) << 8) | // A (高4位)
                                         ((nSr & 0xF0) << 4) |        // B (高4位)
                                         (nSg & 0xF0) |               // G (高4位)
                                         ((nSb & 0xF0) >> 4);         // R (高4位)
                pArgb4444[nSi] = fontColor4444;
            }
        }
    }

    if (pBoderBuf)
    {
        free(pBoderBuf);
    }
    if (pFontBuf)
    {
        free(pFontBuf);
    }

    if (nBorder > 0)
    {
        FT_Stroker_Done(stroker);
    }

    FT_Done_Glyph(glyph_fg);
    FT_Done_Glyph(glyph);

    return 0;
}

/*
 * @brief      将RGBA8888颜色转换为ARGB4444颜色
 * @param      {unsigned int} rgba8888 RGBA8888格式的颜色值
 * @return     {uint16_t} ARGB4444格式的颜色值
 */
uint16_t CFreetypeProcessor::RGBA8888ToARGB4444(unsigned int rgba8888)
{
    uint8_t r = (rgba8888 >> 24) & 0xFF; // 提取R分量
    uint8_t g = (rgba8888 >> 16) & 0xFF; // 提取G分量
    uint8_t b = (rgba8888 >> 8) & 0xFF;  // 提取B分量
    uint8_t a = rgba8888 & 0xFF;         // 提取A分量

    // 将每个8位分量转换为4位，并组合成ARGB4444格式
    uint16_t argb4444 = ((a & 0xF0) << 8) | // A的高4位移至最高四位
                        ((r & 0xF0) << 4) | // R的高4位移至次四位
                        (g & 0xF0) |        // G的高4位
                        ((b & 0xF0) >> 4);  // B的高4位移至低四位
    return argb4444;
}

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

int CFreetypeProcessor::txt2Argb4444(const char *pTxt, int nTxtWidth, int nTxtHeight, CharMap_S *pMap, unsigned int nBackColorkey, unsigned int nSubtitleColorkey, int nW, int nH, int nXpos)
{
    int nBorderWidth = 0; // 边框宽度（像素）
    int k = 0, i = 0, j = 0;
    wchar_t *pwTxt = NULL;
    int nWsize = 0;
    int nLineNub = 0;     // 行数
    int nTotalWidth = 0;  // 总宽度
    int nPicMaxWidth = 0; // 图片最大宽度
    int nRgbWidth = 0;    // 图片实际宽度
    int nRgbHeight = 0;   // 图片实际高度
    int nRgbOffetY = 0;
    int nTotalHeight = 0;
    char *pRgb = NULL;
    /*设置字体大小*/
    if (FT_Set_Char_Size(g_pFTFace, nTxtWidth << 6, nTxtHeight << 6, 0, 0) != OK)
    {
        dlog(LOG_ERROR, "FT_Set_Char_Size 失败");
        return ERR;
    }
    // FT_Set_Pixel_Sizes(g_pFTFace,0 ,nTxtHeight);

    /*图片最大宽度*/
    if (nXpos < nW && nXpos >= 0)
    {
        nPicMaxWidth = nW - nXpos;
    }
    else
    {
        nPicMaxWidth = nW;
    }

    /*字符转换*/
    pwTxt = char_to_Wchar(pTxt, &nWsize);

    if (nWsize <= 0)
    {
        free(pwTxt);
        char *pSrc = " ";
        pwTxt = char_to_Wchar(pSrc, &nWsize);
    }
    CharMap_S *pCharMap = (CharMap_S *)malloc(nWsize * sizeof(CharMap_S));
    memset(pCharMap, 0, nWsize * sizeof(CharMap_S));

    int nMul = 2; // argb4444格式占用2字节，而不是rgba8888的4字节

    for (k = 0; k < nWsize; k++)
    {
        FT_Bitmap stBmp;
        FT_Bitmap *bmp = &stBmp;
        if (FT_TextToBitmap_ARGB4444(&g_pFTFace, &g_pFTLib, pwTxt[k], nSubtitleColorkey, nSubtitleColorkey, nBorderWidth, &stBmp) == -1)
        {
            continue;
        }

        nTotalHeight = g_pFTFace->size->metrics.height / 64; // 字体高度+间隔高度

        int nWidth = g_pFTFace->glyph->advance.x / 64 + nBorderWidth * 2;
        int nHeight = g_pFTFace->glyph->metrics.vertAdvance / 64; // 字体高度

        int nOffetX = g_pFTFace->glyph->metrics.horiBearingX / 64;
        int nOffetY = g_pFTFace->glyph->metrics.vertBearingY / 64;

        pCharMap[k].pBuffer = (char *)malloc(nWidth * nTotalHeight * nMul);
        uint16_t *pData = (uint16_t *)pCharMap[k].pBuffer;
        memset(pData, 0, nWidth * nTotalHeight * nMul);

        pCharMap[k].nWidth = nWidth;
        pCharMap[k].nHeight = nHeight;

        // 将背景颜色转换为ARGB4444格式
        uint16_t backColorKey4444 = RGBA8888ToARGB4444(nBackColorkey);

        for (i = 0; i < nHeight; i++) // 填充背景颜色
        {
            for (j = 0; j < nWidth; j++)
            {
                pData[i * nWidth + j] = backColorKey4444;
            }
        }
        uint16_t *pVal = (uint16_t *)pCharMap[k].pBuffer;
        int m;
        int n = 0;
        for (i = 0; i < bmp->rows; ++i)
        {
            m = (nOffetY + i) * nWidth + nOffetX;
            for (j = 0; j < bmp->width; ++j)
            {
                // 按2字节处理ARGB4444
                uint16_t argb4444 = *((uint16_t *)(stBmp.buffer + n * 2)); // 读取2字节
                uint8_t a = (argb4444 >> 12) & 0x0F;                       // 提取Alpha高4位
                uint8_t r = (argb4444 >> 8) & 0x0F;                        // 提取Red高4位
                uint8_t g = (argb4444 >> 4) & 0x0F;                        // 提取Green高4位
                uint8_t b = argb4444 & 0x0F;                               // 提取Blue高4位
                if (r || g || b || a)
                {
#if 1 /*替换*/
                    // 将RGBA8888转换为ARGB4444
                    uint16_t argb4444 = ((a & 0xF0) << 8) | ((r & 0xF0) << 4) | (g & 0xF0) | ((b & 0xF0) >> 4);
                    pVal[m] = argb4444;
#else /*叠加*/
                    // 将原始ARGB4444分解
                    uint16_t orig = pVal[m];
                    uint8_t origA = (orig >> 12) & 0x0F;
                    uint8_t origR = (orig >> 8) & 0x0F;
                    uint8_t origG = (orig >> 4) & 0x0F;
                    uint8_t origB = orig & 0x0F;

                    // 新值转换为4位格式
                    uint8_t newA = (a >> 4) & 0x0F;
                    uint8_t newR = (r >> 4) & 0x0F;
                    uint8_t newG = (g >> 4) & 0x0F;
                    uint8_t newB = (b >> 4) & 0x0F;

                    // 混合计算
                    uint8_t blendA = (origA + newA > 0x0F) ? 0x0F : (origA + newA);
                    uint8_t blendR = origR + ((newR - origR) * newA / 15);
                    uint8_t blendG = origG + ((newG - origG) * newA / 15);
                    uint8_t blendB = origB + ((newB - origB) * newA / 15);

                    // 组合为ARGB4444
                    pVal[m] = (blendA << 12) | (blendR << 8) | (blendG << 4) | blendB;
#endif
                }

                m++;
                n += 1;
            }
        }

        // uint16_t *pVal = (uint16_t *)pCharMap[k].pBuffer;
        // int m;
        // int n = 0;
        // for (i = 0; i < bmp->rows; ++i) {
        //     m = (nOffetY + i) * nWidth + nOffetX;
        //     for (j = 0; j < bmp->width; ++j) {
        //         // 读取ARGB4444像素（每像素2字节）
        //         uint16_t src_pixel = ((uint16_t*)stBmp.buffer)[i * (bmp->pitch/2) + j];

        //         // 提取Alpha通道（高4位）
        //         uint8_t alpha = (src_pixel >> 12) & 0x0F;
        //         if (alpha > 0) {
        //             // 直接替换背景（或实现Alpha混合）
        //             pVal[m] = src_pixel;
        //         }
        //         m++;
        //     }
        // }

        if (stBmp.buffer)
        {
            free(stBmp.buffer);
        }

        nTotalWidth += nWidth;
        if (nRgbWidth + nWidth <= nPicMaxWidth)
        {
            nRgbWidth += nWidth;
        }

        if (nRgbHeight == 0 || nHeight > nRgbHeight)
        {
            nRgbHeight = nHeight;
        }

        if (nRgbOffetY == 0 || nRgbOffetY > nOffetY)
        {
            nRgbOffetY = nOffetY;
        }
    }

    /*计算所有字符总宽度*/
    nRgbWidth = CODEC_ALIGN(nRgbWidth, 16); // 是否是16??
    if (nW - nRgbWidth > 0 && nW - nRgbWidth < nTxtWidth)
    {
        nRgbWidth = nW;
    }

    int nWid = 0;

    /*计算一共有几行*/
    for (k = 0, nLineNub = 1; k < nWsize; k++)
    {
        nWid += pCharMap[k].nWidth;

        /*判断剩余宽度是否能容纳下一个字符，否则换行*/
        if ((nRgbWidth - nWid) >= ((k < nWsize - 1) ? pCharMap[k + 1].nWidth : 0))
        {
        }
        /*换行*/
        else
        {
            nWid = 0;
            nLineNub++;
        }
    }
    nRgbHeight = nTotalHeight * nLineNub;

    /*nLineNub+2是为了防止数据可能出现越界导致程序崩溃多申请的空间*/
    int nMemSize = nRgbWidth * nRgbHeight * nMul * (nLineNub + 1);
    pRgb = (char *)malloc(nMemSize);

    memset(pRgb, 0, nMemSize);
    int nDiff = 0;
    nWid = 0;
    for (k = 0; k < nWsize; k++)
    {
        for (i = nRgbOffetY; i < pCharMap[k].nHeight; i++)
        {
            memcpy(pRgb + nDiff + (i - nRgbOffetY) * nRgbWidth * nMul, pCharMap[k].pBuffer + pCharMap[k].nWidth * nMul * i, pCharMap[k].nWidth * nMul);
        }

        nWid += pCharMap[k].nWidth;

        /*判断剩余宽度是否能容纳下一个字符，否则换行*/
        if ((nRgbWidth - nWid) >= ((k < nWsize - 1) ? pCharMap[k + 1].nWidth : 0))
        {
            nDiff += pCharMap[k].nWidth * nMul;
        }
        /*换行*/
        else
        {
            /*计算剩余宽度*/
            int nWidthLeft = nRgbWidth - nWid;
            nWid = 0;
            /*判断剩余空间不能容纳一个字符，填满这个空间*/
            if (nWidthLeft > 0 && nWidthLeft < nTxtWidth)
            {
                nDiff += nWidthLeft * nMul;
            }
            nDiff += pCharMap[k].nWidth * nMul;
            nDiff += nRgbWidth * pCharMap[k].nHeight * nMul;
        }

        if (pCharMap[k].pBuffer)
        {
            free(pCharMap[k].pBuffer);
            pCharMap[k].pBuffer = NULL;
        }
    }

    /*判断图片最大高度不能超过nH,否则只取最nH高度内部分数据*/
    if (nRgbHeight > nH)
    {
        nRgbHeight = nH;
    }

    if (NULL != pwTxt)
    {
        free(pwTxt);
        pwTxt = NULL;
    }

    if (NULL != pCharMap)
    {
        free(pCharMap);
        pCharMap = NULL;
    }

    pMap->nWidth = nRgbWidth;
    pMap->nHeight = nRgbHeight;
    pMap->pBuffer = pRgb;

    dlog_trace("pMap->nWidth: %d", pMap->nWidth);
    dlog_trace("pMap->nHeight: %d", pMap->nHeight);
    dlog_trace("pMap->pBuffer: %x", pMap->pBuffer);
    return 0;
}