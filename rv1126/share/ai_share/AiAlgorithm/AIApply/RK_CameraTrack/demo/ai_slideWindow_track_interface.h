/*
 * @Author       : chenchl
 * @Date         : 2023-10-28 13:45:56
 * @LastEditors  : chenchl
 * @LastEditTime : 2023-10-28 08:58:03
 * @FilePath     : ai_slideWindow_track_interface.h
 * @Description  : 滑动窗口跟踪的c调用c++接口
 */
#ifndef _AI_SLIDEWINDOW_TRACK_INTERFACE_H_
#define _AI_SLIDEWINDOW_TRACK_INTERFACE_H_
#include "ai_slideWindow_track.h"
#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief  滑动窗口跟踪分析初始化
     * @param  
     * @return [*]
     * @author chenchl
     * @note
     */
    int slideWindow_track_init();

    /**
     * @brief  滑动窗口跟踪分析反初始化
     * @return [*]
     * @author chenchl
     * @note
     */
    int slideWindow_track_uninit();

    /**
     * @brief  获取AI学生行为分析数据
     * @param  [AiSlideTacksePos_S] ** pstPos - 坐标数组指针，长度为识别总数
     * @param  [int] *pTotal - 识别出总的方框数
     * @return [*]
     * @author EasonLu
     * @note   内部申请内存，*pstPos需要外部释放
     */
    int slideWindow_track_get_pos(AiSlideTacksePos_S **pstPos,int *pTotal);

    /**
     * @brief  送图像数据进行分析
     * @param  [char] *pInputData 图像数据
     * @return [*]
     * @author chenchl
     * @note
     */
    int slideWindow_track_send_image(char *pImageData);

#ifdef __cplusplus
}
#endif
#endif /* _AI_SLIDEWINDOW_TRACK_INTERFACE_H_ */