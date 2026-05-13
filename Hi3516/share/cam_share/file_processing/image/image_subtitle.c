/*
 * image_subtitle.c
 *
 *  Created on: 2018年1月30日
 *      Author: caiws
 */

#include "image_subtitle.h"
#include "image_freeType.h"
#include "nslog.h"
#include "image_iconv.h"
#include "freetype/ftoutln.h"
#include <arpa/inet.h>

typedef struct Subtitle_Operate_Handle
{

	pthread_mutex_t tiltle_mutex;
	Font_Handle_t font_handle;

}Subtitle_Operate_Handle_t;
static int str_to_image(const char *text_buff, int len, SubTitlteParam_t *text, Subtitle_Handle_t pSubHandle, SubTitleResult_t*pSubResult)
{
	Font_Handle_t pFontHandle =  pSubHandle;
	FT_Face face = NULL;
	int index;
	FT_Error error;
	//printf("text_buff:===========%s\n", text_buff);
	if(NULL == text_buff || text == NULL || pFontHandle == NULL || pSubResult == NULL || pSubResult->pImageBuf == NULL)
	{
		nslog(NS_ERROR, "display_text_view:NULL == text_buff\n");
		return -1;
	}
	face = (FT_Face)pFontHandle->face;

	if(text->width < 10)//20 )	//zhangjb modyfy 2018/8/17
	{
		text->width = 10;//20;//zhangjb modyfy 2018/8/17
	}
	if(text->width > 400 )
	{
		text->width = 400;
	}

	FT_Set_Char_Size(face, text->width << 6, 0,72, 72);

	int width, height,x,y, x_max,y_max;
	int i,j,p,q,sp;
	int srcValue,dstValue;
	unsigned char *src = NULL;
	unsigned char *ptr = NULL;
	unsigned char *rowPtr = NULL;
	FT_Vector pen;
	int inc, start;
	int nCharLen = 0;
	unsigned char v;
	int maxsize = 0;
	int numbit  = 3;
	if(text->imageformat == RGB888)
	{
		numbit  = 3;
	}
	else if(text->imageformat == RGBA8888)
	{
		numbit  = 4;
	}

	pSubResult->stride = face->size->metrics.x_ppem * len / 4 * 4 * numbit;

	//因为格式是RGB888
	pSubResult->height = (face->size->metrics.ascender >> 6)/4*4 + 16;
	maxsize = pSubResult->stride * pSubResult->height;
	if(maxsize > pSubResult->imageLen)
	{
		maxsize = pSubResult->imageLen;
		nslog(NS_ERROR, "maxsize:%d pSubResult->imageLen:%d\n", maxsize, pSubResult->imageLen);
	}

	//代表未设置，默认黑底白字
	if((text->backcolorkey == 0x00) && (text->subtitlecolorkey == 0x00))
	{
		text->subtitlecolorkey = 0xffffffff;
	}

	if(text->imageformat == RGB888)
	{
		for(i = 0; i < maxsize; i++)
		{
			pSubResult->pImageBuf[i] = (text->backcolorkey) & 0xff;
			i++;
			pSubResult->pImageBuf[i] = (text->backcolorkey >> 8) & 0xff;
			i++;
			pSubResult->pImageBuf[i] = (text->backcolorkey >> 16) & 0xff;
		}
	}
	else if(text->imageformat == RGBA8888)
	{
		for(i = 0; i < maxsize; i++)
		{
			pSubResult->pImageBuf[i] = (text->backcolorkey) & 0xff;
			i++;
			pSubResult->pImageBuf[i] = (text->backcolorkey >> 8) & 0xff;
			i++;
			pSubResult->pImageBuf[i] = (text->backcolorkey >> 16) & 0xff;
			i++;
			pSubResult->pImageBuf[i] = (text->backcolorkey >> 24) & 0xff;
		}
	}


	pen.x = 0;
	pen.y = 0;



	unsigned int TestCharSet = 0;
	unsigned int nTestFF = 0;
	unsigned int nTestDD = 0;
	int image_position = 0;
	wchar_t *wcTxt = NULL;
	//int *wcTxt = 0;
	memcpy((char *)&TestCharSet, text_buff, 4);
	nTestFF = TestCharSet & 0xFFFF0000;
	nTestDD = TestCharSet & 0x0000FFFF;
	if(nTestFF > 0 && nTestDD == 0)
	{
		//printf("has chinese\n");
		wcTxt = (wchar_t *)text_buff + 1;

	}
	unsigned long txtBak = 0;
	while(wcTxt ? *wcTxt : *text_buff)
	{
		if(wcTxt)
		{
			txtBak = *wcTxt;
			*wcTxt = htonl(txtBak);
		}

//		FT_Matrix matrix;
//		float lean = 1;
//		matrix.xx = 0x10000L;
//		matrix.xy = 0;
//		matrix.yx = 30;
//		matrix.yy = 0x10000L;
		FT_Set_Transform(face, NULL, &pen);

		index = FT_Get_Char_Index(face, wcTxt?*wcTxt:*text_buff);

		error = FT_Load_Glyph(face, index, 0);

		if(error)
		{
			nslog(NS_ERROR, "Failed to load glyph for character %x\n", wcTxt ? *wcTxt:*text_buff);
			break;
		}

		if(face->glyph->format != FT_GLYPH_FORMAT_BITMAP)
		{
			error = FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL);

			if(error) {
				nslog(NS_ERROR, "Failed to render glyph for character %x\n", wcTxt?*wcTxt:*text_buff);
				break;
			}
		}
		FT_BBox oldBox;
		FT_Outline_Get_CBox(&face->glyph->outline , &oldBox);
		//printf("face->glyph->outline:%p\n", face->glyph->outline);
		FT_Outline_Embolden( &face->glyph->outline,64 );
		width = face->glyph->bitmap.width;
		height = face->glyph->bitmap.rows;

		x = face->glyph->bitmap_left;

		y = (face->size->metrics.ascender >> 6) - face->glyph->bitmap_top;
		x_max = x + width;
		y_max = y + height;

		nCharLen = face->glyph->bitmap_left  + width + 10;
		src = face->glyph->bitmap.buffer;
//		//最大宽度为，设置的FT_Set_Char_Size的宽*（horz_resolution/72）
//		printf("x:%d y:%d text->width:%d y_max:%d width:%d height:%d error:%d "
//				"pen.x:%d pen.y:%d face->glyph->bitmap_top:%d heigh:%d face->size->metrics.ascender:%d"
//				"face->size->metrics.max_advance:%d x_ppem:%d text_buff:%c pSubResult->stride:%d\n",
//				x, y, text->width, y_max, width, height, error, pen.x, pen.y,
//				face->glyph->bitmap_top, face->height, face->size->metrics.ascender >> 6, face->size->metrics.max_advance,
//				face->size->metrics.x_ppem, *text_buff, pSubResult->stride);
		ptr = (unsigned char *)pSubResult->pImageBuf + y * pSubResult->stride;
		for(j = y, q = 0; j < y_max; j++, q++)
		{
			rowPtr = ptr + numbit * x;
			for(i = x, p = 0; i < x_max; i++, p++)
			{
				sp = q * width + p;
				if(src[sp] > 0x80 )
				{
					if((rowPtr - pSubResult->pImageBuf) < maxsize)
					{
						rowPtr[0] = (text->subtitlecolorkey) & 0xff;
						rowPtr[1] = (text->subtitlecolorkey >> 8) & 0xff;
						rowPtr[2] = (text->subtitlecolorkey >> 16) & 0xff;
						if(text->imageformat == RGBA8888)
						{
							rowPtr[3] = (text->subtitlecolorkey >> 24) & 0xff;
						}
					}

				}

				rowPtr += numbit;
			}

			ptr += pSubResult->stride;
		}
		if(wcTxt != NULL)
		{
			wcTxt++;
		}
		else
		{
			text_buff++;
		}
		pen.x += face->glyph->advance.x;
		pen.y += face->glyph->advance.y;
	}


	{
		pSubResult->width  =  nCharLen;



	}

	return 0;
}
Subtitle_Handle_t sutitle_create(const char *fontName)
{
	Subtitle_Operate_Handle_t *p_subtitleHandle = (Subtitle_Operate_Handle_t *)malloc(sizeof(Subtitle_Operate_Handle_t));
	if(p_subtitleHandle == NULL)
	{
		return NULL;
	}
	p_subtitleHandle->font_handle =  font_create(fontName);
	if(p_subtitleHandle->font_handle == NULL)
	{
		nslog(NS_ERROR, "sutitle_create is NULL\n");
		free(p_subtitleHandle);
		p_subtitleHandle = NULL;
		return NULL;
	}

	pthread_mutex_init(&(p_subtitleHandle->tiltle_mutex), NULL);
	return p_subtitleHandle;
}
static int check_char(char *text, int size)
{
	int i;

	for(i = 0; i < size; i++)
		if(text[i] > 127) {
			return 1;
		}

	return 0;
}

int  subtitle_strTo_image(SubTitleResult_t*pSubResult, SubTitlteParam_t * pSubTitle ,Subtitle_Handle_t pSubHandle)
{
	Subtitle_Operate_Handle_t *p_subtitleHandle = (Subtitle_Operate_Handle_t*)pSubHandle;
	int ret = 0;
	if(pSubTitle == NULL || p_subtitleHandle == NULL)
	{
		nslog(NS_ERROR, "subtitle_strTo_image is NULL\n");
		return -1;
	}
	int charlen;
	int outLen = 0;


	pthread_mutex_lock(&(p_subtitleHandle->tiltle_mutex));
	charlen = strlen(pSubTitle->msgtext);

	if(charlen != 0)
	{
		//nslog(NS_DEBUG, "add_text_info:display text\n");
		if(check_char(pSubTitle->msgtext, charlen))
		{
			char temp[512] = {0};
			//nslog(NS_DEBUG, "add_text_info:check_CHN is CHN!\n");
			outLen = sizeof(temp);
			if(pSubTitle->format == UTF8)
			{
				code_convert("utf-8", "utf-32", pSubTitle->msgtext, charlen, temp, &outLen);
			}
			else if(pSubTitle->format == GBK2312)
			{
				code_convert("gb2312", "utf-32", pSubTitle->msgtext, charlen, temp, &outLen);
			}
			else
			{
				ret = -1;
			}
			ret =  str_to_image(temp, charlen, pSubTitle, p_subtitleHandle->font_handle, pSubResult);
		}
		else
		{
			ret = str_to_image(pSubTitle->msgtext, charlen, pSubTitle, p_subtitleHandle->font_handle, pSubResult);
		}
	}
	pthread_mutex_unlock(&(p_subtitleHandle->tiltle_mutex));
	return 0;
}

int sutitle_destory(Subtitle_Handle_t pSubHandle)
{
	Subtitle_Operate_Handle_t *p_subtitleHandle = (Subtitle_Operate_Handle_t *)pSubHandle;
	if(p_subtitleHandle == NULL)
	{
		nslog(NS_ERROR, "sutitle_destory is NULL\n");
		return -1;
	}
	if(p_subtitleHandle->font_handle)
	{
		font_destory((Font_Handle_t)p_subtitleHandle->font_handle);
	}
	pthread_mutex_destroy(&(p_subtitleHandle->tiltle_mutex));
	free(p_subtitleHandle);
	return 0;

}
