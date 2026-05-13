/*
 * @Author       : EasonLu
 * @Date         : 2023-07-27 13:45:56
 * @LastEditors  : EasonLu
 * @LastEditTime : 2023-08-09 08:58:03
 * @FilePath     : opencv_face_analyse_interface.h
 * @Description  : 人脸分析的c调用c++接口
 */
#ifndef _OPENCV_FACE_ANALYSE_INTERFACE_H_
#define _OPENCV_FACE_ANALYSE_INTERFACE_H_
#include "opencv_face_analyse.h"
#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief  人脸分析初始化
     * @param  {char *} pModelPath 模型路径
     * @return [*]
     * @author EasonLu
     * @note
     */
    int face_analyse_init(char *pModelPath);

    /**
     * @brief  人脸分析反初始化
     * @return [*]
     * @author EasonLu
     * @note
     */
    int face_analyse_uninit();

    /**
     * @brief  设置人脸分析参数
     * @param  [TunableParam_S] stInfo 参数信息
     * @return [*]
     * @author EasonLu
     * @note
     */
    // int face_analyse_set_param(TunableParam_S stInfo);

    /**
     * @brief  人脸数据分析完毕后获取分析人数结果
     * @return [*]
     * @author EasonLu
     * @note   
     */
    int face_analyse_get_people();

    /**
     * @brief  获取人脸分析的坐标
     * @param  [CvFaceAnalysePos_S] **pstPos 存放坐标结构体指针的二级指针
     * @param  [int] pPeopleNum 重分析后计算的人数
     * @return [int] 已获取坐标组的个数
     * @author EasonLu
     * @note   内部申请内存，*pstPos需要外部释放
     */
    int face_analyse_get_pos(CvFaceAnalysePos_S **pstPos, int *pPeopleNum);

    /**
     * @brief  分析bgr图像格式的人脸
     * @param  [char] *pInputData 输入数据
     * @param  [int] nDataLen 数据长度
     * @param  [unsigned char] *pOutData 输出数据
     * @return [*]
     * @author EasonLu
     * @note
     */
    int face_analyse_bgr(char *pInputData, int nDataLen, unsigned char *pOutData);

#ifdef __cplusplus
}
#endif
#endif /* _OPENCV_FACE_ANALYSE_INTERFACE_H_ */