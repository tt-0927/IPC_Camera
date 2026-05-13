/*
 * @FilePath     : main.cpp
 * @Author       : yanzeh yanzeh@kfb.cn
 * @Date         : 2024-01-12 14:50:33
 * @LastEditors  : 严泽辉 yanzeh@kfb.cn
 * @LastEditTime : 2024-01-27 16:02:27
 * @Description  :
 */
#include <unistd.h>

#include "FaceRecAlgorithm.hpp"

using namespace FR_NS;

int main(int argc, char const* argv[])
{

    FaceRecInParam_S stInfo;

    stInfo.stNeedParam.enType                            = FaceAlgorithmType_E::FR_V1;
    stInfo.stNeedParam.strFaceDetectionModelPath         = "/opt/bl/model/MB_FaceDetector1920x1024.rknn";
    stInfo.stNeedParam.strFaceFeatureExtractionModelPath = "/opt/bl/model/MB_facenet160x160.rknn";

    CFaceRecAlgorithmBase* pFaceRecAlgorithmBase = CFaceRecAlgorithm::createFaceRecAlgorithm(stInfo);

    // pFaceRecAlgorithmBase->addFaceImage("/opt/bl/pic/wcp.jpg","wcp");

    pFaceRecAlgorithmBase->uploadFaceImage(3, "/opt/bl/pic/wcp.jpg", "wcp1111");

    std::list<FaceDataInfo_S> listOutInfo;
    pFaceRecAlgorithmBase->getFaceImageInfo(listOutInfo);
    for (auto item : listOutInfo)
    {
        item.print();
    }

    while (1)
    {
        sleep(10);
    }


    delete pFaceRecAlgorithmBase;
    pFaceRecAlgorithmBase = nullptr;

    return 0;
}
