/**
 * @file onvif_convert.cpp
 * @author tianl (tianl@kfb.cn)
 * @date 2025-10-09
 * 
 * @brief onvif类型转换

 */
#include <string.h>
#include <cstdio>
#include <math.h>

#include "dlog.h"
#include "onvif_convert.h"

#define BASE64_BUFF_LEN      2 * 1024

/* 遮挡报警坐标范围 */
#define ONVIF_TAMPER_WIDTH 704
#define ONVIF_TAMPER_HEIGHT 576
/* 通用坐标范围 */
#define COMMON_WIDTH 1920
#define COMMON_HEIGHT 1080
/* osd像素点高和宽 */
#define OSD_WIDTH   1920
#define OSD_HEIGHT  1080 
/********************************************************************************************/
/*******************************  base64,pack        ***************************************/
/********************************************************************************************/
const char base64i[81] = "\76XXX\77\64\65\66\67\70\71\72\73\74\75XXXXXXX\00\01\02\03\04\05\06\07\10\11\12\13\14\15\16\17\20\21\22\23\24\25\26\27\30\31XXXXXX\32\33\34\35\36\37\40\41\42\43\44\45\46\47\50\51\52\53\54\55\56\57\60\61\62\63";
const char base64o[65] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

#define blank(c)		((c)+1 > 0 && (c) <= 32)

unsigned int unpackbits(unsigned char *outp, unsigned char *inp,
            unsigned int outlen, unsigned int inlen)
{
    unsigned int i, len;
    int val;

    /* i counts output bytes; outlen = expected output size */
    for(i = 0; inlen > 1 && i < outlen;){
        /* get flag byte */
        len = *inp++;
        --inlen;

        if(len == 128) /* ignore this flag value */
            ; // warn_msg("RLE flag byte=128 ignored");
        else{
            if(len > 128){
                len = 1+256-len;

                /* get value to repeat */
                val = *inp++;
                --inlen;

                if((i+len) <= outlen)
                    memset(outp, val, len);
                else{
                    memset(outp, val, outlen-i); // fill enough to complete row
                    printf("unpacked RLE data would overflow row (run)\n");
                    len = 0; // effectively ignore this run, probably corrupt flag byte
                }
            }else{
                ++len;
                if((i+len) <= outlen){
                    if(len > inlen)
                        break; // abort - ran out of input data
                    /* copy verbatim run */
                    memcpy(outp, inp, len);
                    inp += len;
                    inlen -= len;
                }else{
                    memcpy(outp, inp, outlen-i); // copy enough to complete row
                    printf("unpacked RLE data would overflow row (copy)\n");
                    len = 0; // effectively ignore
                }
            }
            outp += len;
            i += len;
        }
    }
    if(i < outlen)
        printf("not enough RLE data for row\n");
    return i;
}

unsigned int packbits(unsigned char *src, unsigned char *dst, unsigned int n)
{
    unsigned char *p, *q = dst, *dataend = src + n;
    
    while (src < dataend) 
    {
        int maxrun = (dataend - src) > 128 ? 128 : (dataend - src);
        int is_repeat = 0;

        /* 检测至少2个连续重复字节 */ 
        if (src + 1 < dataend && *src == *(src + 1)) 
        {
            unsigned char val = *src;
            p = src + 2;
            while (p < src + maxrun && *p == val) p++;
            int count = p - src;
            if (count >= 2) 
            {   /* 触发重复块编码 */ 
                *q++ = 257 - count; 
                *q++ = val;
                src = p;
                is_repeat = 1;
            }
        }

        if (!is_repeat) 
        { 
            p = src + 1;
            /* 寻找连续非重复数据，直到遇到重复或达到maxrun */ 
            while (p < src + maxrun) 
            {
                if (p + 1 < dataend && *p == *(p + 1)) break;
                p++;
            }
            int count = p - src;
            *q++ = count - 1; 
            memcpy(q, src, count);
            q += count;
            src = p;
        }
    }
    /* 返回压缩后数据长度 */
    return q - dst; 
}

int s2base64(const unsigned char *src, char *dst, int srclen) 
{
    char *start = dst;
    while (srclen >= 3) 
    {   /* 处理完整三字节块 */ 
        unsigned int triple = (src[0] << 16) | (src[1] << 8) | src[2];
        *dst++ = base64o[(triple >> 18) & 0x3F];
        *dst++ = base64o[(triple >> 12) & 0x3F];
        *dst++ = base64o[(triple >> 6) & 0x3F];
        *dst++ = base64o[triple & 0x3F];
        src += 3;
        srclen -= 3;
    }
    if (srclen > 0) 
    { 
        /* 处理剩余1或2字节 */ 
        unsigned int triple = src[0] << 16;
        if (srclen > 1)
        {
            triple |= src[1] << 8;
        } 
        *dst++ = base64o[(triple >> 18) & 0x3F];
        *dst++ = base64o[(triple >> 12) & 0x3F];
        *dst++ = (srclen > 1) ? base64o[(triple >> 6) & 0x3F] : '=';
        *dst++ = '=';
    }
    /*  终止符 */
    *dst = '\0'; 
    return dst - start;
}


char * base642s(const char *s, char *t, size_t l, int *n)
{
    size_t i, j;
    char c;
    unsigned long m;
    char *p;
    if (!s || !*s)
    {
        if (n)
        *n = 0;

        return "\0\0\0";
    }

    if (!t)
    {
        l = (strlen(s) + 3) / 4 * 3 + 1;	/* make sure enough space for \0 */
    }

    if (!t)
        return NULL;

    p = t;

    if (n)
        *n = 0;

    for (i = 0; ; i += 3, l -= 3)
    {
        m = 0;
        j = 0;

        while (j < 4)
        {
            c = *s++;
            if (c == '=' || !c)
            {
                if (l >= j - 1)
                {
                    switch (j)
                    {
                        case 2:
                            *t++ = (char)((m >> 4) & 0xFF);
                            i++;
                            l--;
                            break;
                        case 3:
                            *t++ = (char)((m >> 10) & 0xFF);
                            *t++ = (char)((m >> 2) & 0xFF);
                            i += 2;
                            l -= 2;
                    }
                }

                if (n)
                    *n = (int)i;

                if (l)
                    *t = '\0';

                return p;
            }

            c -= '+';
            if (c >= 0 && c <= 79)
            {
                int b = base64i[c];
                m = (m << 6) + b;
                j++;
            }
            else if (!blank(c + '+'))
            {
            return NULL;
            }
        }

        if (l < 3)
        {
            if (n)
                *n = (int)i;
            if (l)
                *t = '\0';
            return p;
        }
        *t++ = (char)((m >> 16) & 0xFF);
        *t++ = (char)((m >> 8) & 0xFF);
        *t++ = (char)(m & 0xFF);
    }
}
/* 掩码数据转换成数组 */
void maskToArray(const unsigned char *pMask, int nMaskSize, unsigned int *pArray, int nArraySize) 
{
    int nIndex = 0;
    for (int i = 0; i < nMaskSize; i++) 
    {
        unsigned char byte = pMask[i];
        for (int j = 7; j >= 0; j--) 
        {
            if (nIndex >= nArraySize) break; 
            pArray[nIndex++] = (byte >> j) & 1; 
        }
    }
}
/* 数组转换成掩码数据 */
void arrayToMask(const unsigned int *array, int arraySize, unsigned char *mask, int maskSize) 
{
    int index = 0;
    for (int i = 0; i < maskSize; i++) 
    {
        unsigned char byte = 0;
        for (int j = 7; j >= 0; j--) 
        {
            if (index >= arraySize) break; 
            byte |= (array[index++] << j); 
        }
        mask[i] = byte;
    }
    //if (maskSize > 0) 
    //{
    //    mask[maskSize - 1] &= 0xF0; 
    //}
}

int ONVIF_MotionBase64StrToArray(char *pStr, unsigned int *pArray, int nArrayLen, int w, int h)
{
    unsigned char   pBuff1[BASE64_BUFF_LEN] = {0};
    unsigned char   pBuff2[BASE64_BUFF_LEN] = {0};
    int    len,outLen;

    w = 22;
    h = 18;

    if(0==pStr || 0==pArray || 0==nArrayLen) return -1;

    len = BASE64_BUFF_LEN;
    base642s(pStr,(char*)pBuff1, strlen(pStr), &len);

    outLen = w * h / 8 + (((w * h) % 8) ? 1 : 0);
    unpackbits(pBuff2,pBuff1,outLen,BASE64_BUFF_LEN);

    maskToArray(pBuff2,BASE64_BUFF_LEN,pArray,nArrayLen);

    return 0;
}

int ONVIF_MotionArrayToBase64Str(unsigned int *pArray, int nArrayLen, char *pStr,  int w, int h)
{
     unsigned char mask[BASE64_BUFF_LEN] = {0};
    unsigned char packed[BASE64_BUFF_LEN] = {0};
    int maskSize, packedLen;

    if (!pArray || !pStr) return -1;

    // 1. 计算掩码所需字节数（每个元素对应1bit）
    maskSize = (w * h + 7) / 8; // 例如：22x18=396 bits → 50 bytes
    arrayToMask(pArray, nArrayLen, mask, maskSize);

    // 2. RLE压缩（注意传入原始掩码长度maskSize）
    packedLen = packbits(mask, packed, maskSize);

    // 3. Base64编码（传入压缩后的长度packedLen）
    s2base64(packed, pStr, packedLen);

    return 0;
}

int convert_tamper_rect(Common::Rect_S *pRect,OnvifPoint_S *pOnvifPoint,bool isOnvifToCommon)
{
     /* 参数有效性检查 */ 
    if (!pRect || !pOnvifPoint) 
    {
        return -1;
    }
    typedef struct { int x, y; } TempPoint;
    /* Onvif(704x576) -> Common(1920x1080) */ 
    if (isOnvifToCommon) 
    { 
         /*  计算缩放比例（ONVIF->Common） */
        const double x_scale = (double)COMMON_WIDTH / ONVIF_TAMPER_WIDTH;
        const double y_scale = (double)COMMON_HEIGHT / ONVIF_TAMPER_HEIGHT;

         // 转换左上顶点坐标
        OnvifPoint_S onvif_top_left = pOnvifPoint[3];
        pRect->nX = round(onvif_top_left.x * x_scale);
        pRect->nY = round((ONVIF_TAMPER_HEIGHT - onvif_top_left.y) * y_scale);
        
        int original_width = pOnvifPoint[1].x - pOnvifPoint[0].x;
        int original_height = pOnvifPoint[3].y - pOnvifPoint[0].y;

        pRect->nWidth = round(original_width * x_scale);
        pRect->nHeight = round(original_height * y_scale);

        dlog_debug("ONVIF->Common 左上角顶点坐标onvif(%d,%d)->common(%d,%d)",pOnvifPoint[3].x,pOnvifPoint[3].y,pRect->nX,pRect->nY);
    } 
     /* Common(1920x1080) -> Onvif(704x576)  */ 
    else 
    { 
         /* 计算缩放比例（Common->ONVIF） */ 
        const double x_scale = (double)ONVIF_TAMPER_WIDTH / COMMON_WIDTH;
        const double y_scale = (double)ONVIF_TAMPER_HEIGHT / COMMON_HEIGHT;

        /* 转换左上顶点 */ 
        pOnvifPoint[3].x = round(pRect->nX * x_scale);
        pOnvifPoint[3].y = round(ONVIF_TAMPER_HEIGHT - (pRect->nY * y_scale));

        /*  重建其他顶点（带缩放） */
        pOnvifPoint[0].x = pOnvifPoint[3].x; 
        pOnvifPoint[0].y = round(pOnvifPoint[3].y - (pRect->nHeight * y_scale));
        
        pOnvifPoint[1].x = round(pOnvifPoint[3].x + (pRect->nWidth * x_scale));
        pOnvifPoint[1].y = pOnvifPoint[0].y;
        
        pOnvifPoint[2].x = pOnvifPoint[1].x;
        pOnvifPoint[2].y = pOnvifPoint[3].y;

      
        dlog_debug("Common->ONVIF 左上角顶点坐标comon(%d,%d)->onvif(%d,%d)",pRect->nX,pRect->nY,pOnvifPoint[3].x,pOnvifPoint[3].y);
    }

    return 0;
}

int convert_osd_pos(OnvifOsdPos_S *pOnvifOsdPos,CommomOsdPos_S *pCommomOsdPos,bool isOnvifToComm)
{
    if(isOnvifToComm)
    {
        /* 自定义坐标 */
        if(!strcmp(pOnvifOsdPos->achTpye, "Custom"))
        {
            pCommomOsdPos->x = static_cast<int>(std::round((pOnvifOsdPos->x + 1.0) * OSD_WIDTH / 2.0));
            pCommomOsdPos->y = static_cast<int>(std::round((1.0 - pOnvifOsdPos->y) * OSD_HEIGHT / 2.0));

            dlog_debug("坐标转换:onvif(%f,%f)->common(%d,%d)",pOnvifOsdPos->x,pOnvifOsdPos->y,pCommomOsdPos->x,pCommomOsdPos->y);
        }
        /*左上 */
        else if(!strcmp(pOnvifOsdPos->achTpye, "UpperLeft"))
        {
            pCommomOsdPos->x = 0;
            pCommomOsdPos->y = 0;
        }
        /* 右上 */
        else if(!strcmp(pOnvifOsdPos->achTpye, "UpperRight"))
        {
            pCommomOsdPos->x = OSD_WIDTH - 1;  
            pCommomOsdPos->y = 0;
        }
        /* 左下 */
        else if(!strcmp(pOnvifOsdPos->achTpye, "LowerLeft"))
        {
            pCommomOsdPos->x = 0;
            pCommomOsdPos->y = OSD_HEIGHT - 1;  
        }
        /* 右下 */
        else if(!strcmp(pOnvifOsdPos->achTpye, "LowerRight"))
        {
            pCommomOsdPos->x = OSD_WIDTH - 1; 
            pCommomOsdPos->y = OSD_HEIGHT - 1;  
        }
        else
        {
            pCommomOsdPos->x = 0;
            pCommomOsdPos->y = 0;
        }
       
        
    }
    else
    {
        pOnvifOsdPos->x = (2.0 * pCommomOsdPos->x ) / OSD_WIDTH - 1.0;
        pOnvifOsdPos->y = 1.0 - (2.0 * pCommomOsdPos->y) / OSD_HEIGHT;
        dlog_debug("坐标转换:common(%d,%d)->onvif(%f,%f)",pCommomOsdPos->x,pCommomOsdPos->y,pOnvifOsdPos->x,pOnvifOsdPos->y);
    }

    return 0;
}