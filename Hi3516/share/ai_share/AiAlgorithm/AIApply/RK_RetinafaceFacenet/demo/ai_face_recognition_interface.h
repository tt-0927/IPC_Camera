/*
 * @Author       : chenchl
 * @Date         : 2023-11-15 13:45:56
 * @LastEditors  : chenchl
 * @LastEditTime : 2023-11-15 08:58:03
 * @FilePath     : ai_face_recognition_interface.h
 * @Description  : 人脸识别的c调用c++接口
 */
#ifndef _AI_FACE_RECHNITION_INTERFACE_H_
#define _AI_FACE_RECHNITION_INTERFACE_H_
#include "ai_face_recognition.h"
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
    int face_recognition_init();

    /**
     * @brief  滑动窗口跟踪分析反初始化
     * @return [*]
     * @author chenchl
     * @note
     */
    int face_recognition_uninit();

    /**
     * @brief  获取AI学生行为分析数据
     * @param  [AiFaceFeaturesePos_S] ** pstPos - 坐标数组指针，长度为识别总数
     * @param  [int] *pTotal - 识别出总的方框数
     * @return [*]
     * @author EasonLu
     * @note   内部申请内存，*pstPos需要外部释放
     */
    int face_recognition_get_pos(AiFaceFeaturesePos_S **pstPos,int *pTotal);

    /**
     * @brief  保存人脸特征信息到数据库
     * @param  [*] 
     * @return [*]
     * @author chenchl
     * @note
     */
    void AISaveFeature();

    /**
     * @brief  人脸特征库初始化
     * @param  [*] 
     * @return [*]
     * @author chenchl
     * @note
     */
    int shareDBInit();

    /**
     * @brief  读取数据库的人脸特征
     * @param  [*] 
     * @return [*]
     * @author chenchl
     * @note
     */
    void GetAllFeature();

    /**
     * @brief  送图像数据进行分析
     * @param  [char] *pInputData 图像数据
     * @return [*]
     * @author chenchl
     * @note
     */
    int face_recognition_send_image(char *pImageData);

#ifdef __cplusplus
}
#endif
#endif /* _AI_FACE_RECHNITION_INTERFACE_H_ */