/*
 * @FilePath     : main.cpp
 * @Author       : yanzeh yanzeh@kfb.cn
 * @Date         : 2024-01-12 14:50:33
 * @LastEditors  : yanzeh yanzeh@kfb.cn
 * @LastEditTime : 2024-01-18 16:15:45
 * @Description  :
 */
#include "TrackerAlgorithm.hpp"

using namespace TA_NS;

int main(int argc, char const* argv[])
{

    TrackerAlgorithmInParam_S stInfo;

    stInfo.stNeedParam.enType              = TrackerAlgorithmType_E::TA_V1;
    stInfo.stNeedParam.strFeatureModelPath = "/opt/bl/model/Deepsort_facenet.rknn";
    stInfo.stNeedParam.strHeadModelPath    = "/opt/bl/model/HeadDetect.rknn";

    CTrackerAlgorithmBase* pTrackerAlgorithmBase = CTrackerAlgorithm::createTrackerAlgorithm(stInfo);

    delete pTrackerAlgorithmBase;
    pTrackerAlgorithmBase = nullptr;

    return 0;
}
