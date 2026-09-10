/**
 * @FilePath     : face_config_adapter.hpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-08-28
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-08-28
 * @Description  : 人脸侦测配置适配器，1920×1080 像素坐标归一化到 [0,1]
 */

#pragma once

#if CAP_UNIFIED_EVENT_PIPELINE

#include "face_types.hpp"

namespace AiPipeline_NS
{
namespace Face_NS
{
/**
 * @brief   : 人脸侦测配置适配器：只在配置边界做一次坐标转换
 */
class CFaceConfigAdapter
{
public:
    /**
     * @brief   : 转换并校验人脸配置
     * @param    {const RawFaceConfig_S &} stRawConfig：1920×1080 像素坐标配置（平台无关）
     * @param    {FaceConfig_S &} stOut：输出归一化内部配置
     * @return   {int} OK：成功，ERR_PARAM：配置非法（输出配置保持禁用状态）
     */
    int adapt(const RawFaceConfig_S &stRawConfig, FaceConfig_S &stOut) const;

private:
    /**
     * @brief   : 校验并归一化单区域多边形
     * @param    {const RawFaceConfig_S &} stRawConfig：原始配置
     * @param    {FaceConfig_S &} stOutConfig：输出归一化配置
     * @return   {bool} true：有效
     */
    bool adapt_polygon(const RawFaceConfig_S &stRawConfig, FaceConfig_S &stOutConfig) const;
};
} // namespace Face_NS
} // namespace AiPipeline_NS

#endif // CAP_UNIFIED_EVENT_PIPELINE
