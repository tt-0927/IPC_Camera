/*
 * image_bmp.c
 *
 *  Created on: 2022年8月11日
 *      Author: yanzh
 */
#include <malloc.h>
#include "image_bmp.h"

#define BM 19778        // 位图的标志


//判断是否是位图,在0-1字节
int image_bmp_IsBitMap(FILE *fp)
{
    unsigned short s;
    fread(&s, 1, 2, fp);
    return s == BM ? 1 : 0;
}

//获得图片的宽度,在18-21字节
int image_bmp_getWidth(FILE *fp)
{
    int width;
    fseek(fp, 18, SEEK_SET);
    fread(&width, 1, 4, fp);
    return width;
}

//获得图片的高度 ，在22-25字节
int image_bmp_getHeight(FILE *fp)
{
    int height;
    fseek(fp, 22, SEEK_SET);
    fread(&height, 1, 4, fp);
    return height;
}

//获得每个像素的位数,在28-29字节
unsigned short image_bmp_getBit(FILE *fp)
{
    unsigned short bit;
    fseek(fp, 28, SEEK_SET);
    fread(&bit, 1, 2, fp);
    return bit;
}

//获得数据的起始位置，在10-13字节
unsigned int image_bmp_getOffSet(FILE *fp)
{
    unsigned int OffSet;
    fseek(fp, 10L, SEEK_SET);
    fread(&OffSet, 1, 4, fp);
    return OffSet;
}
