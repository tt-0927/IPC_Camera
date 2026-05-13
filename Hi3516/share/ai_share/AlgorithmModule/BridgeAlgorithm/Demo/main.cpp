/*
 * @FilePath     : main.cpp
 * @Author       : yanzeh yanzeh@kfb.cn
 * @Date         : 2024-01-12 14:50:33
 * @LastEditors  : 严泽辉 yanzeh@kfb.cn
 * @LastEditTime : 2024-03-08 11:26:52
 * @Description  :
 */
#include "BridgeAlgorithm.hpp"

using namespace BDGA_NS;

int main(int argc, char const* argv[])
{

    InParam_S stInfo;

    stInfo.stNeedParam.enType = AnalyzerType_E::BDGA_V1;

    CBridgeAlgorithmBase* pBridgeAlgorithmBase = CBridgeAlgorithm::createBridgeAlgorithm(stInfo);

    delete pBridgeAlgorithmBase;
    pBridgeAlgorithmBase = nullptr;


    return 0;
}
