/**
 * @FilePath     : isp_manage.h
 * @Author       : cyc
 * @Date         : 2025-08-27 09:51:40
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-08-12 14:36:04
 * @Description  : ISP共享层调用入口
 */

#pragma once

#include <condition_variable>
#include <memory>
#include <mutex>

#include "Singleton.h"
#include "isp_define.h"
#include "isp_service_interface.h"
#include "isp_config_command_service.h"
#include "isp_runtime_intent.h"

/**
 * @brief ISP共享层调用入口。
 * @note  ipc_share 只保存业务服务指针；场景、日夜、补光和硬件操作由业务仓实现。
 */
class CIspManage : public CSingleton<CIspManage>
{
public:
    CIspManage();
    ~CIspManage();
    friend class CSingleton<CIspManage>;

    /**
     * @brief   : 图像管理初始化
     * @return   {int} OK：成功，非OK：失败
     */
    int init();

    /**
     * @brief   : 图像管理去初始化
     * @return   {int} OK：成功，非OK：失败
     */
    int deinit();

    /**
     * @brief   : 强制重新应用当前持久化的全部ISP配置
     * @return   {int} OK：成功，非OK：业务服务未注册或硬件重放失败
     * @note    : 由平台视频链路完成可能覆盖硬件状态的初始化边界后调用，确保启动阶段的
     *            ISP参数通过同一个共享reconciler再次串行下发；共享层不假设具体媒体平台。
     */
    int reconcile_all();

    /**
     * @brief   : 更新并应用图像配置
     * @param    {ISP::PicConfigureType_E} enConfigType：配置类型
     * @return   {int} OK：成功，非OK：失败
     */
    int update_config(const ISP::PicConfigureType_E &enConfigType);

    /**
     * @brief   : 更新日夜切换配置
     * @param    {ISP::DayNightAttr_S} stOldDayNightAttr：更新前配置
     * @param    {ISP::DayNightAttr_S} stNewDayNightAttr：更新后配置
     * @return   {int} OK：成功，非OK：失败
     */
    int update_daynight(const ISP::DayNightAttr_S &stOldDayNightAttr, const ISP::DayNightAttr_S &stNewDayNightAttr);

    /**
     * @brief   : 加载并应用指定网页配置场景的所有参数
     * @param    {ISP::SceneType_E} enSceneType：目标网页配置场景
     * @return   {int} OK：成功，非OK：失败
     */
    int apply_scene_all_params(ISP::SceneType_E enSceneType);

    /**
     * @brief   : 场景计划配置变化通知
     * @return   {int} OK：成功，非OK：失败
     */
    int on_schedule_changed();

    /**
     * @brief   : 注册ISP业务服务
     * @param    {ISP::IIspBusinessService*} pService：业务服务，调用方负责生命周期
     * @return   {int} OK：成功，ERR_PARAM_NULL：参数为空，ERR：重复注册或正在注销
     */
    int set_business_service(ISP::IIspBusinessService *pService);

    /**
     * @brief   : 清理ISP业务服务
     * @param    {ISP::IIspBusinessService*} pService：期望清理的业务服务
     * @return   {int} OK：成功，ERR_PARAM_NULL：参数为空，ERR_PARAM：指针不匹配
     * @note    : 返回前会等待已取得服务调用句柄的转发完成，调用方随后可安全销毁服务实例。
     */
    int clear_business_service(ISP::IIspBusinessService *pService);

    /**
     * @brief   : 按当前能力画像校验并归一化图像基础参数
     * @param    {ISP::ImageParam_S} stConfig：待校验的图像参数
     * @return   {int} OK：成功，非OK：失败
     */
    int validate_image_param_config(ISP::ImageParam_S &stConfig) const;

    /**
     * @brief   : 按当前能力画像校验并归一化日夜补光配置
     * @param    {ISP::DayNightAttr_S} stConfig：待校验的日夜配置
     * @return   {int} OK：成功，非OK：失败
     */
    int validate_daynight_config(ISP::DayNightAttr_S &stConfig) const;

    /**
     * @brief   : 设置图像参数（统一事务：校验→持久化→应用→失败恢复）
     * @return   {int} OK：成功，非OK：失败
     */
    int set_image_config(const ISP::ImageParam_S &stConfig);

    /**
     * @brief   : 设置曝光参数（统一事务）
     * @return   {int} OK：成功，非OK：失败
     */
    int set_exposure_config(const ISP::ExposureAttr_S &stConfig);

    /**
     * @brief   : 设置日夜参数（统一事务）
     * @return   {int} OK：成功，非OK：失败
     */
    int set_daynight_config(const ISP::DayNightAttr_S &stConfig);

    /**
     * @brief   : 设置背光参数（统一事务）
     * @return   {int} OK：成功，非OK：失败
     */
    int set_backlight_config(const ISP::BackLightArrt_S &stConfig);

    /**
     * @brief   : 设置白平衡参数（统一事务）
     * @return   {int} OK：成功，非OK：失败
     */
    int set_awb_config(const ISP::AwbAttr_S &stConfig);

    /**
     * @brief   : 设置降噪参数（统一事务）
     * @return   {int} OK：成功，非OK：失败
     */
    int set_nr_config(const ISP::DnrAttr_S &stConfig);

    /**
     * @brief   : 设置镜像参数（统一事务）
     * @return   {int} OK：成功，非OK：失败
     */
    int set_mirror_config(const ISP::VideoAdjust_S &stConfig);

    /**
     * @brief   : 设置场景计划（统一事务）
     * @return   {int} OK：成功，非OK：失败
     */
    int set_scene_schedule(const ISP::SceneSchedule_S &stConfig);

    /**
     * @brief   : 设置用户场景（统一事务）
     * @return   {int} OK：成功，非OK：失败
     */
    int set_user_scene(ISP::SceneType_E enScene);

    /**
     * @brief   : 恢复显示设置默认配置（组合事务）
     * @return   {int} OK：成功，非OK：失败
     * @note    : 仅恢复图像显示相关配置，不修改外设补光配置或peripheral_info.json。
     */
    int restore_default_config();

    /**
     * @brief   : 开始临时灯光抢占
     * @param    {const ISP::IspLightOverride_S&} stOverride：抢占配置
     * @param    {uint64_t&} u64Token：输出抢占token
     * @return   {int} OK：成功，ERR_NOT_ENABLED：外设总控禁止，其他：校验或硬件应用失败
     * @note    : 请求通过arbiter进入统一reconciler，不能绕过外设补光gate。
     */
    int begin_light_override(const ISP::IspLightOverride_S &stOverride, uint64_t &u64Token);

    /**
     * @brief   : 结束临时灯光抢占
     * @param    {uint64_t} u64Token：抢占token
     * @return   {int} OK：成功
     */
    int end_light_override(uint64_t u64Token);

private:
    /**
     * @brief   : 保护一次业务服务调用
     * @note    : 构造时增加调用数，析构时减少；清除服务会等待所有调用结束。
     */
    class CServiceCallGuard
    {
    public:
        /**
         * @brief   : 获取当前已注册服务的受控调用句柄
         * @param    {const CIspManage&} rstManage：所属调用入口
         * @return   {void}
         */
        explicit CServiceCallGuard(const CIspManage &rstManage);
        /**
         * @brief   : 归还服务调用句柄
         * @return   {void}
         */
        ~CServiceCallGuard();
        CServiceCallGuard(const CServiceCallGuard &) = delete;
        CServiceCallGuard &operator=(const CServiceCallGuard &) = delete;

        /**
         * @brief   : 获取受保护的业务服务指针
         * @return   {ISP::IIspBusinessService*} 已注册服务，未注册返回nullptr
         */
        ISP::IIspBusinessService *get() const;

    private:
        const CIspManage &m_rstManage;
        ISP::IIspBusinessService *m_pstService;
    };

    /**
     * @brief   : 归还一次业务服务调用计数
     * @return   {void}
     */
    void release_service_call() const;

private:
    /* memory: 业务服务由业务仓库单例持有，CIspManage只保存非拥有指针。 */
    ISP::IIspBusinessService *m_pstService;
    /* 配置命令服务，service注册成功后构造，清理前销毁。 */
    std::unique_ptr<CIspConfigCommandService> m_pstCommandService;
    /* ISP配置仓储，包装CIspConfigure。 */
    CIspConfigRepository m_stIspRepository;
    /* lock: 保护service指针和command service生命周期，禁止并发取得已注销对象。 */
    mutable std::mutex m_mtxService;
    /* lock: clear_business_service 等待已取得服务句柄的调用全部完成后才允许业务对象销毁。 */
    mutable std::condition_variable m_stServiceIdleCv;
    /* 仅 clear_business_service 可置位，禁止其等待期间插入新的注册事务。 */
    bool m_bServiceClearing = false;
    mutable unsigned int m_nActiveServiceCalls = 0;
};
