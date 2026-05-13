/*
 * @Author       : EasonLu
 * @Date         : 2023-10-11 11:32:36
 * @LastEditors  : EasonLu
 * @LastEditTime : 2023-10-11 15:29:54
 * @FilePath     : ai_stu_behavior_interface.h
 * @Description  : AI学生行为分析c调用c++接口
 */
#ifndef _AI_STU_BEHAVIOR_INTERFACE_H_
#define _AI_STU_BEHAVIOR_INTERFACE_H_
#include "ai_stu_behavior_v1.h"
#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief  AI学生行为分析初始化
     * @param  [char] *pModelPath - 模型路径
     * @return [*]
     * @author EasonLu
     * @note
     */
    int stu_behavior_init(char *pModelPath);

    /**
     * @brief  AI学生行为分析反初始化
     * @return [*]
     * @author EasonLu
     * @note
     */
    int stu_behavior_uninit();

    /**
     * @brief  获取AI学生行为分析数据
     * @param  [AiStuBehaviorPos_S] ** pstPos - 坐标数组指针，长度为识别总数
     * @param  [int] *pTotal - 识别出总的方框数
     * @return [*]
     * @author EasonLu
     * @note   内部申请内存，*pstPos需要外部释放
     */
    int stu_behavior_get_pos(AiStuBehaviorPos_S **pstPos,int *pTotal);

    /**
     * @brief  送图像数据进行分析
     * @param  [char] *pImageData - 图像数据
     * @return [*]
     * @author EasonLu
     * @note   
     */
    int stu_behavior_send_image(char *pImageData);

#ifdef __cplusplus
}
#endif

#endif /* _AI_STU_BEHAVIOR_INTERFACE_H_ */