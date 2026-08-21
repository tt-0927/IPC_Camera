/**
 * @FilePath     : isp_scene_provider_hi3516.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-07-13 14:13:58
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-07-27 15:35:33
 * @Description  : Hi3516 ISP场景适配端口实现声明
 */

#pragma once

#include "isp_platform_adapters.h"
#include "isp_tuning_profile.h"

/**
 * @brief Hi3516 ISP场景适配端口。
 * @note  内部持有Hi3516TuningProfile_S引用和配置路径，调用CSceneParamManager。
 *        平台类型不出现在共享头文件。
 */
class CIspSceneProviderHi3516 : public IIspSceneProvider
{
public:
    /**
     * @brief   : 构造Hi3516场景provider
     * @param    {const Hi3516TuningProfile_S&} stTuningProfile：机型调参画像
     */
    explicit CIspSceneProviderHi3516(const Hi3516TuningProfile_S &stTuningProfile);
    /**
     * @brief   : 销毁场景 provider；资源释放由显式 deinit 完成。
     * @return   {void}
     */
    ~CIspSceneProviderHi3516() = default;

    /**
     * @brief   : 加载场景配置并初始化MPP Scene模块。
     * @return   {int} OK：成功，非OK：加载失败
     */
    int init() override;
    /**
     * @brief   : 停止并释放MPP Scene模块。
     * @return   {int} OK：成功，非OK：释放失败
     */
    int deinit() override;
    /**
     * @brief   : 用调参画像切换MPP Scene模式。
     * @param    {ISP::IspRuntimeScene_E} enRuntimeScene：目标内部运行场景
     * @return   {int} OK：成功，非OK：映射或设置失败
     */
    int apply_scene(ISP::IspRuntimeScene_E enRuntimeScene) override;

private:
    /* memory: 调参画像由业务服务持有，本类只保存非拥有引用。 */
    const Hi3516TuningProfile_S &m_rstTuningProfile;
    /* 初始化状态 */
    bool m_bInitialized;
};
