/**
 * @FilePath     : isp_config_command_service.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-07-13 12:24:26
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-07-20 17:48:54
 * @Description  : ISP固定命令配置应用事务服务
 */

#pragma once

#include "isp_config_value.h"
#include "isp_config_repository.h"
#include "isp_service_interface.h"

/**
 * @brief ISP固定命令配置应用事务服务。
 * @note  统一执行校验→快照→持久化→应用/恢复，所有SET命令经此入口。
 */
class CIspConfigCommandService
{
public:
    /**
     * @brief   : 构造配置命令服务
     * @param    {IIspConfigRepository&} stRepository：ISP配置仓储
     * @param    {ISP::IIspBusinessService&} stBusinessService：ISP业务服务
     */
    CIspConfigCommandService(IIspConfigRepository &stRepository, ISP::IIspBusinessService &stBusinessService);

    /**
     * @brief   : 设置图像参数
     * @return   {int} OK：成功，ERR_UNSUPPORT/ERR_PARAM：校验失败，非OK：应用失败
     */
    int set_image_config(const ISP::ImageParam_S &stConfig);

    /**
     * @brief   : 设置曝光参数
     * @return   {int} OK：成功，ERR_UNSUPPORT/ERR_PARAM：校验失败，非OK：应用失败
     */
    int set_exposure_config(const ISP::ExposureAttr_S &stConfig);

    /**
     * @brief   : 设置日夜参数
     * @return   {int} OK：成功，ERR_UNSUPPORT/ERR_PARAM：校验失败，非OK：应用失败
     */
    int set_daynight_config(const ISP::DayNightAttr_S &stConfig);

    /**
     * @brief   : 设置背光参数
     * @return   {int} OK：成功，ERR_UNSUPPORT/ERR_PARAM：校验失败，非OK：应用失败
     */
    int set_backlight_config(const ISP::BackLightArrt_S &stConfig);

    /**
     * @brief   : 设置白平衡参数
     * @return   {int} OK：成功，ERR_UNSUPPORT/ERR_PARAM：校验失败，非OK：应用失败
     */
    int set_awb_config(const ISP::AwbAttr_S &stConfig);

    /**
     * @brief   : 设置降噪参数
     * @return   {int} OK：成功，ERR_UNSUPPORT/ERR_PARAM：校验失败，非OK：应用失败
     */
    int set_nr_config(const ISP::DnrAttr_S &stConfig);

    /**
     * @brief   : 设置镜像参数
     * @return   {int} OK：成功，ERR_UNSUPPORT/ERR_PARAM：校验失败，非OK：应用失败
     */
    int set_mirror_config(const ISP::VideoAdjust_S &stConfig);

    /**
     * @brief   : 设置场景计划
     * @return   {int} OK：成功，ERR_UNSUPPORT/ERR_PARAM：校验失败，非OK：应用失败
     */
    int set_scene_schedule(const ISP::SceneSchedule_S &stConfig);

    /**
     * @brief   : 设置用户场景
     * @return   {int} OK：成功，ERR_UNSUPPORT/ERR_PARAM：校验失败，非OK：应用失败
     */
    int set_user_scene(ISP::SceneType_E enScene);

    /**
     * @brief   : 恢复显示设置默认配置（组合事务）
     * @return   {int} OK：成功，非OK：失败
     * @note    : 仅恢复当前场景槽和镜像，不修改场景计划、外设或peripheral_info.json。
     */
    int restore_default_config();

private:
    /**
     * @brief   : 执行单配置variant事务
     * @param    {ISP::IspConfigValue_T} stNewValue：新配置（按值传递，内部归一化）
     * @return   {int} OK：成功，非OK：失败
     * @note    : 流程：normalize → load old → save new → apply → 失败恢复
     */
    int set_config(ISP::IspConfigValue_T stNewValue);

    /**
     * @brief   : 按variant类型分派到对应policy校验函数
     * @param    {ISP::IspConfigValue_T&} stValue：配置入出参
     * @param    {const ISP::IspCapabilityProfile_S&} stProfile：能力画像
     * @return   {int} OK：成功，ERR_UNSUPPORT/ERR_PARAM：校验失败
     */
    int normalize(ISP::IspConfigValue_T &stValue, const ISP::IspCapabilityProfile_S &stProfile) const;

    /**
     * @brief   : 应用失败后恢复旧配置
     * @param    {const ISP::IspConfigValue_T&} stOldValue：旧配置快照
     * @param    {int} nOriginalRet：原始应用错误码
     * @return   {int} 始终返回nOriginalRet，恢复失败只记录不覆盖
     */
    int restore_after_apply_failure(const ISP::IspConfigValue_T &stOldValue, int nOriginalRet);

    /**
     * @brief   : 恢复显示设置默认事务失败后回滚图像配置域
     * @param    {const ISP::AllSceneParams_S&} stOldAllParams：旧场景快照
     * @param    {const ISP::VideoAdjust_S&} stOldMirror：旧镜像快照
     * @param    {int} nOriginalRet：最先发生的错误码
     * @return   {int} 始终返回nOriginalRet，回滚错误仅记录
     * @note    : 外设补光属于独立配置域，显示设置恢复默认不得读取、写入或应用该配置。
     */
    int restore_default_after_failure(const ISP::AllSceneParams_S &stOldAllParams,
                                      const ISP::VideoAdjust_S &stOldMirror,
                                      int nOriginalRet);

    /* ISP配置仓储引用 */
    IIspConfigRepository &m_rstRepository;
    /* ISP业务服务引用 */
    ISP::IIspBusinessService &m_rstBusinessService;
};
