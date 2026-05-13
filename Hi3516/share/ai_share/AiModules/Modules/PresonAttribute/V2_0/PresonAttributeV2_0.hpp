/*
 * @Author: lianghy lianghy@kfb.cn
 * @Date: 2026-02-02 15:33:26
 * @LastEditors: lianghy lianghy@kfb.cn
 * @LastEditTime: 2026-02-04 10:02:10
 * @FilePath: /1126/share/ai_share/AiModules/Modules/PresonAttribute/V2_0/PresonAttributeV2_0.hpp
 * @Description: 行人属性分析
 */

#pragma once

#include <unordered_map>
#include "Attribute.hpp"
#include "PresonAttributeExt.hpp"

namespace PresonAttribute_NS {
class CPresonAttributeV2_0 {
  public:
    CPresonAttributeV2_0(InParam_S stInParam);
    ~CPresonAttributeV2_0();

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
     * @param vecResult 分析的参数
     * @return true
     * @return false
     */
    bool process(InData_S stInData, std::vector<Result_S> &vecResult);

  private:
    /**
     * @brief 等比例缩放图片
     * @param [cv::Mat] inputImage: 传入的图片数据
     * @param [cv::Mat&] pchOutData: 输出的缩放后的图片
     * @return [*]
     * @note
     */
    bool resizeAndPadImage(cv::Mat inputImage, cv::Mat &outputImage);

  private:
    /* 性別标签枚举 */
    typedef enum class GenderLabel
    {
        UNKNOWN = -1,
        MALE    = 6,
        FEMALE  = 28,
    } GenderLabel_E;

    /* 年龄标签枚举 */
    typedef enum class AgeLabel
    {
        UNKNOWN     = -1,
        AGE_15_29   = 0,
        AGE_30_44   = 1,
        AGE_45_59   = 2,
        AGE_60_PLUS = 3,
        AGE_0_14    = 26,
    } AgeLabel_E;

    /* 上装类型标签枚举 */
    typedef enum class TopTypeLabel
    {
        UNKNOWN      = -1,
        SHORT_SLEEVE = 10, /* 短袖 */
        LONG_SLEEVE  = 30, /* 长袖 */
        VEST         = 31, /* 背心 */
    } TopTypeLabel_E;

    /* 上身颜色标签枚举 */
    typedef enum class TopColorLabel
    {
        UNKNOWN = -1,
        BLACK   = 12, /* 上身黑色 */
        GRAY    = 13, /* 上身灰色 */
        ORANGE  = 14, /* 上身橙色 */
        PINK    = 15, /* 上身粉色 */
        RED     = 16, /* 上身红色 */
        WHITE   = 17, /* 上身白色 */
        YELLOW  = 18, /* 上身黄色 */
    } TopColorLabel_E;

    /* 下装类型标签枚举 */
    typedef enum class BottomTypeLabel
    {
        UNKNOWN     = -1,
        SHORT_PANTS = 9,  /* 短裤 */
        LONG_PANTS  = 11, /* 长裤 */
        LONG_SKIRT  = 29, /* 长裙 */
    } BottomTypeLabel_E;

    /* 下身颜色标签枚举 */
    typedef enum class BottomColorLabel
    {
        UNKNOWN = -1,
        BLACK   = 19, /* 下身黑色 */
        GRAY    = 20, /* 下身灰色 */
        ORANGE  = 21, /* 下身橙色 */
        PINK    = 22, /* 下身粉色 */
        RED     = 23, /* 下身红色 */
        WHITE   = 24, /* 下身白色 */
        YELLOW  = 25, /* 下身黄色 */
    } BottomColorLabel_E;

    /* 物品标签枚举 */
    typedef enum class ItemLabel
    {
        UNKNOWN     = -1,
        BACKPACK    = 4,  /* 双肩包 */
        HAT         = 5,  /* 帽子 */
        POSTMAN_BAG = 7,  /* 邮差包 */
        HAND_BAG    = 8,  /* 手提袋 */
        UMBRELLA    = 27, /* 雨伞 */
    } ItemLabel_E;

    /* 初始化参数 */
    InParam_S m_stInParam;

    Inference_NS::CAttribute *m_pPedestrianAttribute = nullptr;

    /* 算法输入参数限制 */
    int m_nLimitHeight  = 0;
    int m_nLimitWidth   = 0;
    int m_nLimitChannel = 0;

    /* 置信度阈值 */
    const float CONF_THRESHOLD = 0.5f;

    /* 胡子置信度阈值 */
    const float CONF_BEARD_THRESHOLD = 0.2f;
    /* 眼镜置信度阈值 */
    const float CONF_GLASS_THRESHOLD = 0.02f;
};

}  // namespace PresonAttribute_NS
