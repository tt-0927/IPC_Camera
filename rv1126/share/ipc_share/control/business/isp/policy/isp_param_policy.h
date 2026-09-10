/**
 * @FilePath     : isp_param_policy.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-07-10 15:19:22
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-07-23 09:17:09
 * @Description  : ISP参数校验纯策略工具声明
 */

#pragma once

#include "isp_define.h"
#include "isp_capability_profile.h"

/**
 * @brief ISP参数能力校验纯策略集合。
 */
namespace IspParamPolicy_NS
{

/**
 * @brief   : 校验图像参数并修正到平台支持范围
 * @param    {ISP::ImageParam_S} stConfig：图像参数入出参
 * @param    {ISP::IspCapabilityProfile_S} stProfile：平台功能和参数范围
 * @return   {int} OK：成功，ERR_UNSUPPORT：能力不支持
 */
int normalize_image_param(ISP::ImageParam_S &stConfig, const ISP::IspCapabilityProfile_S &stProfile);

/**
 * @brief   : 校验日夜和补光参数并修正到平台支持范围
 * @param    {ISP::DayNightAttr_S} stConfig：日夜配置入出参
 * @param    {ISP::IspCapabilityProfile_S} stProfile：能力画像
 * @return   {int} OK：成功，ERR_PARAM：参数非法或定时模式开始时间不早于结束时间，ERR_UNSUPPORT：能力不支持
 */
int normalize_daynight(ISP::DayNightAttr_S &stConfig, const ISP::IspCapabilityProfile_S &stProfile);

/**
 * @brief   : 校验曝光参数并修正到平台支持范围
 * @param    {ISP::ExposureAttr_S&} stConfig：曝光配置入出参
 * @param    {const ISP::IspCapabilityProfile_S&} stProfile：能力画像
 * @return   {int} OK：成功，ERR_PARAM：曝光时间非法，ERR_UNSUPPORT：能力不支持
 */
int normalize_exposure(ISP::ExposureAttr_S &stConfig, const ISP::IspCapabilityProfile_S &stProfile);

/**
 * @brief   : 校验背光参数并修正到平台支持范围
 * @param    {ISP::BackLightArrt_S&} stConfig：背光配置入出参
 * @param    {const ISP::IspCapabilityProfile_S&} stProfile：能力画像
 * @return   {int} OK：成功，ERR_PARAM：WDR/HLC在不支持时被启用，ERR_UNSUPPORT：能力不支持
 */
int normalize_backlight(ISP::BackLightArrt_S &stConfig, const ISP::IspCapabilityProfile_S &stProfile);

/**
 * @brief   : 校验白平衡参数并修正到平台支持范围
 * @param    {ISP::AwbAttr_S&} stConfig：白平衡配置入出参
 * @param    {const ISP::IspCapabilityProfile_S&} stProfile：能力画像
 * @return   {int} OK：成功，ERR_PARAM：模式非法，ERR_UNSUPPORT：能力不支持
 */
int normalize_awb(ISP::AwbAttr_S &stConfig, const ISP::IspCapabilityProfile_S &stProfile);

/**
 * @brief   : 校验降噪参数并修正到平台支持范围
 * @param    {ISP::DnrAttr_S&} stConfig：降噪配置入出参
 * @param    {const ISP::IspCapabilityProfile_S&} stProfile：能力画像
 * @return   {int} OK：成功，ERR_PARAM：模式非法，ERR_UNSUPPORT：能力不支持
 */
int normalize_nr(ISP::DnrAttr_S &stConfig, const ISP::IspCapabilityProfile_S &stProfile);

/**
 * @brief   : 校验镜像参数并修正到平台支持范围
 * @param    {ISP::VideoAdjust_S&} stConfig：镜像配置入出参
 * @param    {const ISP::IspCapabilityProfile_S&} stProfile：能力画像
 * @return   {int} OK：成功，ERR_PARAM：模式非法，ERR_UNSUPPORT：能力不支持
 */
int normalize_mirror(ISP::VideoAdjust_S &stConfig, const ISP::IspCapabilityProfile_S &stProfile);

/**
 * @brief   : 校验平台是否支持场景类型
 * @param    {ISP::SceneType_E&} enScene：场景类型入出参
 * @param    {const ISP::IspCapabilityProfile_S&} stProfile：平台功能和参数范围
 * @return   {int} OK：成功，ERR_PARAM：场景不在支持集合，ERR_UNSUPPORT：能力不支持
 */
int normalize_scene(ISP::SceneType_E &enScene, const ISP::IspCapabilityProfile_S &stProfile);

} // namespace IspParamPolicy_NS
