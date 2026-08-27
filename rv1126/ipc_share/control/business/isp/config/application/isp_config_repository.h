/**
 * @FilePath     : isp_config_repository.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-07-13 11:53:59
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-07-17 13:40:47
 * @Description  : ISP配置仓储接口与CIspConfigure包装
 */

#pragma once

#include "isp_config_value.h"

/**
 * @brief ISP配置仓储抽象接口。
 * @note  供CIspConfigCommandService使用，实现方包装CIspConfigure typed overload。
 */
class IIspConfigRepository
{
public:
    virtual ~IIspConfigRepository() = default;

    /**
     * @brief   : 按variant当前类型读取配置
     * @param    {ISP::IspConfigValue_T&} stValue：配置variant，调用方预设类型
     * @return   {int} OK：成功，非OK：失败
     */
    virtual int load(ISP::IspConfigValue_T &stValue) const = 0;

    /**
     * @brief   : 按variant当前类型持久化配置
     * @param    {const ISP::IspConfigValue_T&} stValue：配置variant
     * @return   {int} OK：成功，非OK：失败
     */
    virtual int save(const ISP::IspConfigValue_T &stValue) = 0;

    /**
     * @brief   : 读取全量场景参数快照
     * @param    {ISP::AllSceneParams_S&} stConfig：全量场景参数输出
     * @return   {int} OK：成功，非OK：失败
     */
    virtual int load_all_scene_params(ISP::AllSceneParams_S &stConfig) const = 0;

    /**
     * @brief   : 持久化全量场景参数快照
     * @param    {const ISP::AllSceneParams_S&} stConfig：全量场景参数
     * @return   {int} OK：成功，非OK：失败
     */
    virtual int save_all_scene_params(const ISP::AllSceneParams_S &stConfig) = 0;

    /**
     * @brief   : 按明确的网页场景读取完整参数槽位
     * @param    {ISP::SceneType_E} enScene：目标网页配置场景
     * @param    {ISP::SceneParams_S&} stConfig：场景参数输出
     * @return   {int} OK：成功，ERR_PARAM：场景非法，其他：读取失败
     */
    virtual int load_scene_params(ISP::SceneType_E enScene, ISP::SceneParams_S &stConfig) const = 0;

    /**
     * @brief   : 恢复当前场景槽默认配置
     * @return   {int} OK：成功，非OK：失败
     */
    virtual int restore_defaults() = 0;
};

/**
 * @brief CIspConfigure的可测试仓储包装。
 * @note  使用std::visit转发到现有CIspConfigure typed overload，不修改底层配置文件格式。
 */
class CIspConfigRepository : public IIspConfigRepository
{
public:
    /**
     * @brief   : 按 variant 当前类型读取底层 ISP 配置。
     * @param    {ISP::IspConfigValue_T&} stValue：预设类型的配置输出
     * @return   {int} OK：成功，非OK：读取失败
     */
    int load(ISP::IspConfigValue_T &stValue) const override;
    /**
     * @brief   : 按 variant 当前类型保存底层 ISP 配置。
     * @param    {const ISP::IspConfigValue_T&} stValue：待持久化配置
     * @return   {int} OK：成功，非OK：写入失败
     */
    int save(const ISP::IspConfigValue_T &stValue) override;
    /**
     * @brief   : 读取包含所有场景槽位的一致性快照。
     * @param    {ISP::AllSceneParams_S&} stConfig：全量配置输出
     * @return   {int} OK：成功，非OK：读取失败
     */
    int load_all_scene_params(ISP::AllSceneParams_S &stConfig) const override;
    /**
     * @brief   : 原子语义地写回调用方已校验的全量场景快照。
     * @param    {const ISP::AllSceneParams_S&} stConfig：全量配置
     * @return   {int} OK：成功，非OK：写入失败
     */
    int save_all_scene_params(const ISP::AllSceneParams_S &stConfig) override;
    /**
     * @brief   : 从全量快照复制指定网页场景的参数槽位。
     * @param    {ISP::SceneType_E} enScene：目标网页配置场景
     * @param    {ISP::SceneParams_S&} stConfig：场景参数输出
     * @return   {int} OK：成功，ERR_PARAM：场景非法，其他：读取失败
     */
    int load_scene_params(ISP::SceneType_E enScene, ISP::SceneParams_S &stConfig) const override;
    /**
     * @brief   : 恢复当前场景槽位默认值。
     * @return   {int} OK：成功，非OK：底层恢复失败
     */
    int restore_defaults() override;
};
