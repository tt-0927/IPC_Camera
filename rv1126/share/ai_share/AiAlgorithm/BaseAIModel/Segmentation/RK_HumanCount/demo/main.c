/*
 * @FilePath     : main.cpp
 * @Author       : 严泽辉 yanzeh@kfb.cn
 * @Date         : 2024-01-29 17:05:21
 * @LastEditors  : 严泽辉 yanzeh@kfb.cn
 * @LastEditTime : 2024-01-29 17:12:19
 * @Description  : 
 */
#include "opencv_face_analyse.h"
#include "readImg.h"

int main(int argc, char const *argv[])
{
    CvFaceAnalyseNeedParam_S stNeedParam;
    stNeedParam.nWidth = 768;
    /* 图片高度 */
    stNeedParam.nHeight =640;
    /* 未使用，目前只支持RGB图片格式 */
    CvFaceAnalyseImageFormat_E enImageFormat;
    stNeedParam.enImageFormat =enImageFormat;
    /* 使用的人脸分析模型绝对路径 */
    stNeedParam.pModlePath="./weights/HR_human.rknn";
    /* 分析间隔 */
    stNeedParam.nInterval=1;    
    
    char* img= (char*)malloc(768*640*3+10);
    read_img("test.jpg",img);
    CvFaceAnlyseInputData_S *dataInfo;
    dataInfo->pImageData = img;
    dataInfo->nDataLen = 768*640*3;
    /* 用户自定义参数，会从输出数据中带回 */
    void *pUsrParam;
     dataInfo->pUsrParam = pUsrParam;
    /* 用户自定义需要释放的参数 */
    void *pFreeParam;
    dataInfo->pFreeParam = pFreeParam;
    /* 只送分析，不取回图像数据，标记位置1时OutputData为图像指针为NULL */
    int bAnalyseOnly=1;
    dataInfo->bAnalyseOnly=bAnalyseOnly;
    
    CvFaceAnalyse_S *ch = opencv_face_analyse_alloc(stNeedParam);
    ch->init(ch);
    ch->send_image(ch,dataInfo,100);
    
    return 0;
}



