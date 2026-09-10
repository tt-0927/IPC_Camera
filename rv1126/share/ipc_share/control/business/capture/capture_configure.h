/**
 * @FilePath     : capture_configure.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2025-06-25 20:14:45
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2025-07-15 19:54:20
 * @Description  : 抓图配置
 */

#pragma once

#include <functional>
#include <memory>
#include "Singleton.h"
#include "config_storage.h"
#include "capture_define.h"

class CCaptureConfigure : public CSingleton<CCaptureConfigure>
{
    CCaptureConfigure();

public:
    ~CCaptureConfigure();
    friend class CSingleton<CCaptureConfigure>;

    /**
     * @brief   : 设置抓图计划
     * @param    {CapturePlan_S} &data：抓图配置
     * @return   {int}0：成功，非0：失败
     */
    int set_configure(const Capture_NS::CapturePlan_S &data);

    /**
     * @brief   : 获取抓图计划
     * @param    {CapturePlan_S} &data：抓图配置
     * @return   {int}0：成功，非0：失败
     */
    int get_configure(Capture_NS::CapturePlan_S &data) const;

    /**
     * @brief   : 设置抓图参数
     * @param    {CaptureParam_S} &data：抓图配置
     * @return   {int}0：成功，非0：失败
     */
    int set_configure(const Capture_NS::CaptureParam_S &data);

    /**
     * @brief   : 获取抓图参数
     * @param    {CaptureParam_S} &data：抓图配置
     * @return   {int}0：成功，非0：失败
     */
    int get_configure(Capture_NS::CaptureParam_S &data) const;

private:
    /* 抓图计划 */
    ConfigStorage<Capture_NS::CapturePlan_S, StorageType_E::Single> m_capturePlan;
    /* 抓图参数 */
    ConfigStorage<Capture_NS::CaptureParam_S, StorageType_E::Single> m_captureParam;
};
