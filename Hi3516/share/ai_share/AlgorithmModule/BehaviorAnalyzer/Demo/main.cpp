/*
 * @FilePath     : main.cpp
 * @Author       : yanzeh yanzeh@kfb.cn
 * @Date         : 2024-01-12 14:50:33
 * @LastEditors  : yanzeh yanzeh@kfb.cn
 * @LastEditTime : 2024-01-23 15:07:08
 * @Description  :
 */
#include "BehaviorAnalyzer.hpp"

using namespace BA_NS;

int main(int argc, char const* argv[])
{

    BehaviorAnalyzerInParam_S stInfo;

    stInfo.stNeedParam.enType       = BehaviorAnalyzerType_E::BA_STU_V1;
    stInfo.stNeedParam.strModelPath = "/opt/rk/modle/student_behavior.rknn";

    CBehaviorAnalyzerBase* pBehaviorAnalyzerBase1 = CBehaviorAnalyzer::createBehaviorAnalyzer(stInfo);

    stInfo.stNeedParam.enType       = BehaviorAnalyzerType_E::BA_STU_V2;
    stInfo.stNeedParam.strModelPath = "/opt/rk/modle/student_behavior.rknn";

    CBehaviorAnalyzerBase* pBehaviorAnalyzerBase2 = CBehaviorAnalyzer::createBehaviorAnalyzer(stInfo);


    delete pBehaviorAnalyzerBase1;
    pBehaviorAnalyzerBase1 = nullptr;

    delete pBehaviorAnalyzerBase2;
    pBehaviorAnalyzerBase2 = nullptr;

    return 0;
}
