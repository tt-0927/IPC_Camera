/*
 * image_subtitle.h
 *
 *  Created on: 2018年1月30日
 *      Author: caiws
 */

#ifndef CODE_SHARE_IMAGE_IMAGE_SUBTITLE_H_
#define CODE_SHARE_IMAGE_IMAGE_SUBTITLE_H_
#include <pthread.h>
typedef void * Subtitle_Handle_t;
#define OSD_FONT_SIZE	20
#define OSD_POS_X		40
#define OSD_POS_Y		40
typedef enum
{
	GBK2312 = 0,
	UTF8
}Code_Format_t;
typedef enum
{
	SUBTITLE_FIXED = 0,
	SUBTITLE_CHANGE = 1,
}SUBTILE_FORMAT;
typedef enum
{
	RGB888 = 0,
	RGBA8888 = 1,
}SubTile_ImageFormat_t;
typedef struct _SubTitleResult
{
	unsigned char* pImageBuf;//图片内容
	int imageLen;//图片大小，图片地址的长度
	int stride;//行距
	int width;//图片的宽度
	int height;//图片的高度
}SubTitleResult_t;
typedef struct SubTitlteParam
{
	char msgtext[256];
	Code_Format_t format;
	SUBTILE_FORMAT subtitle_format;
	int width;//每个字符的宽，目前最小20 最大支持60
	int subtitlecolorkey;//RGB888或者RGB8888
	int backcolorkey;//RGB888或者RGB8888
	SubTile_ImageFormat_t imageformat;
	int add_picture;//不加图片直接和背景色运算
	int nCenterSubtile;//字幕在图片居中
}SubTitlteParam_t;


Subtitle_Handle_t sutitle_create(const char *fontName);
int  subtitle_strTo_image(SubTitleResult_t*pSubResult, SubTitlteParam_t * pSubTitle ,Subtitle_Handle_t pSubHandle);

int sutitle_destory(Subtitle_Handle_t pSubHandle);
#endif /* CODE_SHARE_IMAGE_IMAGE_SUBTITLE_H_ */
