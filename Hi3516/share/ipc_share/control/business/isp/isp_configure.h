/*** 
 * @FilePath     : isp_configure.h
 * @Author       : cyc
 * @Date         : 2025-07-17 10:57:00
 * @LastEditors  : cyc
 * @LastEditTime : 2025-08-27 10:19:42
 * @Description  : 图像配置
 */

#pragma once

#include <memory>
#include "Singleton.h"
#include "config_storage.h"
#include "isp_define.h"


class CIspConfigure : public CSingleton<CIspConfigure>
{
    CIspConfigure();

public:
    ~CIspConfigure();
    friend class CSingleton<CIspConfigure>;

    /**
     * @brief   : 设置场景
     * @param    {SceneType_E} &data：场景类型
     * @return   {int}0：成功，非0：失败
     */
    int set_configure(const ISP::SceneType_E &data);

    /**
     * @brief   : 获取场景
     * @param    {SceneType_E} &data：场景类型
     * @return   {int}0：成功，非0：失败
     */
    int get_configure(ISP::SceneType_E &data) const;

    /**
     * @brief   : 设置图像参数
     * @param    {ImageParam_S} &data：图像参数配置
     * @return   {int}0：成功，非0：失败
     */
    int set_configure(const ISP::ImageParam_S &data);

    /**
     * @brief   : 获取图像参数
     * @param    {ImageParam_S} &data：图像参数配置
     * @return   {int}0：成功，非0：失败
     */
    int get_configure(ISP::ImageParam_S &data) const;

    /**
     * @brief   : 设置曝光参数
     * @param    {ExposureAttr_S} &data：图像曝光配置
     * @return   {int}0：成功，非0：失败
     */
    int set_configure(const ISP::ExposureAttr_S &data);

    /**
     * @brief   : 获取曝光参数
     * @param    {ExposureAttr_S} &data：图像曝光配置
     * @return   {int}0：成功，非0：失败
     */
    int get_configure(ISP::ExposureAttr_S &data) const;

    /**
     * @brief   : 设置日夜切换
     * @param    {DayNightAttr_S} &data：日夜切换配置
     * @return   {int}0：成功，非0：失败
     */
    int set_configure(const ISP::DayNightAttr_S &data);

    /**
     * @brief   : 获取日夜切换
     * @param    {DayNightAttr_S} &data：日夜切换配置
     * @return   {int}0：成功，非0：失败
     */
    int get_configure(ISP::DayNightAttr_S &data) const;

    /**
     * @brief   : 设置背光参数
     * @param    {BackLightArrt_S} &data：图像背光配置
     * @return   {int}0：成功，非0：失败
     */
    int set_configure(const ISP::BackLightArrt_S &data);

    /**
     * @brief   : 获取背光参数
     * @param    {BackLightArrt_S} &data：图像背光配置
     * @return   {int}0：成功，非0：失败
     */
    int get_configure(ISP::BackLightArrt_S &data) const;

    /**
     * @brief   : 设置白平衡参数
     * @param    {AwbAttr_S} &data：图像白平衡配置
     * @return   {int}0：成功，非0：失败
     */
    int set_configure(const ISP::AwbAttr_S &data);

    /**
     * @brief   : 获取白平衡参数
     * @param    {AwbAttr_S} &data：图像白平衡配置
     * @return   {int}0：成功，非0：失败
     */
    int get_configure(ISP::AwbAttr_S &data) const;

    /**
     * @brief   : 设置降噪参数
     * @param    {DnrAttr_S} &data：图像降噪配置
     * @return   {int}0：成功，非0：失败
     */
    int set_configure(const ISP::DnrAttr_S &data);

    /**
     * @brief   : 获取降噪参数
     * @param    {DnrAttr_S} &data：图像降噪配置
     * @return   {int}0：成功，非0：失败
     */
    int get_configure(ISP::DnrAttr_S &data) const;

    /**
     * @brief   : 设置镜像参数
     * @param    {VideoAdjust_S} &data：图像镜像配置
     * @return   {int}0：成功，非0：失败
     */
    int set_configure(const ISP::VideoAdjust_S &data);

    /**
     * @brief   : 获取镜像参数
     * @param    {VideoAdjust_S} &data：图像镜像配置
     * @return   {int}0：成功，非0：失败
     */
    int get_configure(ISP::VideoAdjust_S &data) const;

    /**
     * @brief   : 设置图像计划配置参数
     * @param    {SceneSchedule_S} &data：图像计划配置参数
     * @return   {int}0：成功，非0：失败
     */
    int set_configure(const ISP::SceneSchedule_S &data);

    /**
     * @brief   : 获取图像计划配置参数
     * @param    {SceneSchedule_S} &data：图像计划配置参数
     * @return   {int}0：成功，非0：失败
     */
    int get_configure(ISP::SceneSchedule_S &data) const;


    /*** 
     * @description : 获取所有图像参数配置
     * @author      : cyc
     * @param        {AllSceneParams_S} &data
     * @return       {*}
     */ 
    int get_configure(ISP::AllSceneParams_S &data) const;

    /*** 
     * @description : 设置所有图像参数配置
     * @author      : cyc
     * @param        {AllSceneParams_S} &data
     * @return       {*}
     */ 
    int set_configure(const ISP::AllSceneParams_S &data);

    /*** 
     * @description : 恢复默认当前场景参数
     * @author      : cyc
     * @return       {*}
     */ 
     int set_configure();

private:
    /* 场景配置 */
    ConfigStorage<ISP::SceneType_E, StorageType_E::Single> m_picScene;

    /* 镜像配置 */
    ConfigStorage<ISP::VideoAdjust_S, StorageType_E::Single> m_picMirror;
    /* 图像计划配置参数 */
    ConfigStorage<ISP::SceneSchedule_S, StorageType_E::Single> m_picSchedule;
    /* 场景容器配置参数 */
    ConfigStorage<ISP::AllSceneParams_S, StorageType_E::Single> m_picSceneParam;
};


