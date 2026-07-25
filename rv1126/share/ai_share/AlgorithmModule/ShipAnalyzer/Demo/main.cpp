/*
 * @FilePath     : main.cpp
 * @Author       : yanzeh yanzeh@kfb.cn
 * @Date         : 2024-01-12 14:50:33
 * @LastEditors  : 严泽辉 yanzeh@kfb.cn
 * @LastEditTime : 2024-03-14 19:27:19
 * @Description  :
 */
#include "ShipAnalyzer.hpp"

using namespace ShipAnalyzer_NS;

int main(int argc, char const* argv[])
{

    InParam_S stInfo;

    stInfo.stNeedParam.enType = AnalyzerType_E::SA_LINE_V1;

    CShipAnalyzerBase* pBase = CShipAnalyzer::createShipAnalyzer(stInfo);

    delete pBase;
    pBase = nullptr;


    return 0;
}
