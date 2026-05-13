/*
 * image_bmp.h
 *
 *  Created on: 2022年8月11日
 *      Author: yanzh
 */

#ifndef OSSHARE_IMAGE_IMAGE_BMP_H_
#define OSSHARE_IMAGE_IMAGE_BMP_H_

#ifdef __cplusplus
extern "C"
{
#endif

# if 0

#define ushort unsigned short
#define uint unsigned int
#define uchar unsigned char

/*文件信息头结构体*/
typedef struct tagBITMAPFILEHEADER{
    unsigned short bfType;      //必为'BM'
    unsigned int   bfSize;      //文件字节数(2-5)
    unsigned int bfReserved;    //位图文件保留字，必为0(6-9)
    unsigned int   bfOffBits;   //像素数据偏移 (10-13)
} bmpHeader;

//图像信息头结构体
typedef struct tagBITMAPINFOHEADER{
    unsigned int    biSize;          // 结构体尺寸 (14-17)
    int     biWidth;         // 图像宽度  (18-21)
    int     biHeight;        // 图像高度  (22-25)
    ushort  biPlanes;        // 目标设备的级别，为1(26-27)
    ushort  biBitCount;      // 像素位数，为1、4、8或24(28-29)
    unsigned int    biCompression;   // 位图压缩类型，0为不压缩、1为BI_RLE8、2为BI_RLE4(30-33)
    unsigned int    biSizeImage;     // 单像素数据大小,等于bfSize-bfOffBits (34-37)
    int     biXPelsPerMeter; // 水平分辨率，一般为0 (38-41)
    int     biYPelsPerMeter; // 垂直分辨率，一般为0 (42-45)
    unsigned int    biClrUsed;       // 位图颜色表中的颜色数，0表示使用所有调色板项(46-49)
    unsigned int    biClrImportant;  // 重要颜色索引的数目，0表示都重要(50-53)
} infoHeader;

#endif

/*判断是否是位图,在0-1字节 */
int image_bmp_IsBitMap(FILE *fp);

/*获得图片的宽度,在18-21字节 */
int image_bmp_getWidth(FILE *fp);

/*获得图片的高度 ，在22-25字节 */
int image_bmp_getHeight(FILE *fp);

/*获得每个像素的位数,在28-29字节 */
unsigned short image_bmp_getBit(FILE *fp);

/*获得数据的起始位置，在10-13字节*/
unsigned int image_bmp_getOffSet(FILE *fp);

#ifdef __cplusplus
}
#endif

#endif /* OSSHARE_IMAGE_IMAGE_BMP_H_ */
