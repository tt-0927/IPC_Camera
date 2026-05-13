/*
 * @Author: lianghy lianghy@kfb.cn
 * @Date: 2026-02-04 15:43:27
 * @LastEditors: lianghy lianghy@kfb.cn
 * @LastEditTime: 2026-02-05 09:58:34
 * @FilePath: /1126/share/ai_share/AiModules/Modules/NonMotorizedAttribute/V2_0/NonMotorizedAttributeV2_0.hpp
 * @Description: 非机动车属性检测
 */

#pragma once

#include <unordered_map>
// #include "VehicleAttribute.hpp"
#include "Attribute.hpp"
#include "NonMotorizedAttributeExt.hpp"

namespace NonMotorizedAttribute_NS {
class CNonMotorizedAttributeV2_0 {
  public:
    CNonMotorizedAttributeV2_0(InParam_S stInParam);
    ~CNonMotorizedAttributeV2_0();

    /**
     * @brief 初始化
     * @return true
     * @return false
     */
    bool init();

    /**
     * @brief 反初始化
     * @return true
     * @return false
     */
    bool unInit();

    /**
     * @brief 处理数据
     * @param stInData 传入的视频数据
     * @param nResult 分析的参数
     * @return true
     * @return false
     */
    bool process(InData_S stInData, std::vector<Result_S> &nResult);
    bool resizeAndPadImage(cv::Mat inputImage, cv::Mat &outputImage);

    enum class NonMotorType : int
    {
        BICYCLE = 0,   // 自行车
        TWO_WHEELER,   // 二轮车（电动车/摩托）
        THREE_WHEELER  // 三轮车
    };

    enum class NonMotorColor : int
    {
        WHITE = 3,  // 白色
        ORANGE,     // 橙色
        PINK,       // 粉色
        BLACK,      // 黑色
        RED,        // 红色
        YELLOW,     // 黄色
        GRAY,       // 灰色
        BLUE,       // 蓝色
        GREEN,      // 绿色
        BROWN,      // 棕色
        PURPLE      // 紫色
    };

  private:
    void PrintNonVehicleAttribute(const Result_S &stResult);

    /* 初始化参数 */
    InParam_S m_stInParam;

    Inference_NS::CAttribute *m_pNonMotorizedAttribute = nullptr;

    /* 算法输入参数限制 */
    int m_nLimitWidth  = 256;
    int m_nLimitHeight = 192;

    int m_nLimitChannel = 3;
};

}  // namespace NonMotorizedAttribute_NS
