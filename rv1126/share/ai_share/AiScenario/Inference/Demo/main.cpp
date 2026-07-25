/*
 * @FilePath     : main.cpp
 * @Author       : 严泽辉 yanzeh@kfb.cn
 * @Date         : 2024-05-21 19:16:25
 * @LastEditors  : 严泽辉 yanzeh@kfb.cn
 * @LastEditTime : 2024-08-02 17:48:32
 * @Description  :
 */

#include "HumanCount.hpp"
#include "KWS.hpp"

int main(int argc, char const* argv[])
{
    InferenceV1_0_NS::CCAInferenceBase* pBase1 = new InferenceV1_0_NS::CKWS("1","1","1","1","1");
    InferenceV1_0_NS::CCVInferenceBase* pBase2 = new InferenceV1_0_NS::CHumanCount("1111");

    return 0;
}
