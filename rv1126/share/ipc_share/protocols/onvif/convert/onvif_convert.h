/**
 * @file onvif_convert.h
 * @author tianl (tianl@kfb.cn)
 * @date 2025-10-09
 * 
 * @brief onvif类型转换
 */
#pragma once
#include "onvif_type.h"
#include "common_define.h"
#ifdef __cplusplus
extern "C"
{
#endif
/**
 * @brief 移动侦测布局Base64字符串编码转换布局数组
 * @param pStr Base64字符
 * @param pArray 布局数组
 * @param nArrayLen 数组长度
 * @param w 布局宽
 * @param h 布局高
 * @return int 非0转换失败
 */
int ONVIF_MotionBase64StrToArray(char *pStr, unsigned int *pArray, int nArrayLen, int w, int h);
int ONVIF_MotionArrayToBase64Str(unsigned int *pArray, int nArrayLen, char *pStr,  int w, int h);

/**
 * @brief 遮挡报警区域坐标转换
 * @param pRect 通用坐标
 * @param pOnvifPoint Onvif坐标
 * @param isOnvifToCommon 是否是onvif转通用坐标
 * @return int 
 */
int convert_tamper_rect(Common::Rect_S *pRect,OnvifPoint_S *pOnvifPoint,bool isOnvifToCommon);
/**
 * @brief OSD坐标转换
 * @param pOnvifOsdPos onvif坐标
 * @param pCommomOsdPos 通用坐标
 * @param isOnvifToComm 是否是onvif转通用坐标
 * @return int 
 */
int convert_osd_pos(OnvifOsdPos_S *pOnvifOsdPos,CommomOsdPos_S *pCommomOsdPos,bool isOnvifToComm);

#ifdef __cplusplus
}
#endif
