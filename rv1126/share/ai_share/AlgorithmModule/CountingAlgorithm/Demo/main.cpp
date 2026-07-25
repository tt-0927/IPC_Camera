/*
 * @FilePath     : main.cpp
 * @Author       : yanzeh yanzeh@kfb.cn
 * @Date         : 2024-01-12 14:50:33
 * @LastEditors  : yanzeh yanzeh@kfb.cn
 * @LastEditTime : 2024-01-23 15:06:18
 * @Description  :
 */
#include "CountingAlgorithm.hpp"

using namespace CA_NS;

int main(int argc, char const* argv[])
{

    CountingAnalyzerInParam_S stInfo;

    stInfo.stNeedParam.enType       = CountingAlgorithmType_E::CA_V1;
    stInfo.stNeedParam.strModelPath = "/opt/rk/modle/HR_human768x640_NWPU.rknn";

    CCountingAlgorithmBase* pCountingAlgorithmBase1 = CCountingAlgorithm::createCountingAlgorithm(stInfo);

    stInfo.stNeedParam.enType       = CountingAlgorithmType_E::CA_V2;
    stInfo.stNeedParam.strModelPath = "/opt/rk/modle/HR_human768x640_NWPU.rknn";

    CCountingAlgorithmBase* pCountingAlgorithmBase2 = CCountingAlgorithm::createCountingAlgorithm(stInfo);


    delete pCountingAlgorithmBase1;
    pCountingAlgorithmBase1 = nullptr;

    delete pCountingAlgorithmBase2;
    pCountingAlgorithmBase2 = nullptr;

    return 0;
}
