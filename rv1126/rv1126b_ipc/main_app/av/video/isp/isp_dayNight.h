/**
 * @FilePath     : isp_dayNight.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-07-22 15:30:00
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-08-11 13:39:13
 * @Description  : RV1126B SmartIR自动日夜观测控制声明
 */

#pragma once

#include <atomic>
#include <condition_variable>
#include <functional>
#include <mutex>

#include "Singleton.h"
#include "isp_runtime_decision.h"
#include "rk_smart_ir_api.h"

/**
 * @brief RV1126B SmartIR自动观测控制器。
 * @note 只负责观测候选状态；手动、定时、过滤和硬件执行均属于共享ISP核心。
 *       SmartIR上下文借用CIspControl的RK AIQ上下文，调用方必须先停止本控制器再释放AIQ。
 */
class CDayNightController : public CSingleton<CDayNightController>
{
public:
    /** 自动观测回调只传递SmartIR候选状态，不代表共享层已经接受该状态。 */
    using ObservationCallback = std::function<void(bool bSuggestedNight)>;

    /**
     * @brief   : 构造SmartIR观测控制器
     * @return   {void}
     */
    CDayNightController();
    /**
     * @brief   : 停止SmartIR观测并销毁控制器
     * @return   {void}
     */
    ~CDayNightController();
    friend class CSingleton<CDayNightController>;

    /**
     * @brief   : 启动SmartIR自动观测
     * @return   {int} OK：成功，非OK：RK SmartIR初始化或回调注册失败
     */
    int start();
    /**
     * @brief   : 停止SmartIR自动观测
     * @return   {int} OK：成功，非OK：底层释放失败
     */
    int stop();
    /**
     * @brief   : 同步共享层最后成功运行态
     * @param    {const ISP::IspDayNightObservationContext_S&} stContext：最后成功运行态和实际灯型
     * @return   {int} OK：成功，非OK：SmartIR基线同步失败
     * @note    : 仅更新SmartIR观测基线，禁止切换IQ、白光、红外或IR-CUT。
     */
    int sync_runtime_context(const ISP::IspDayNightObservationContext_S &stContext);
    /**
     * @brief   : 设置SmartIR观测灵敏度
     * @param    {unsigned int} nLevel：共享层已校验的灵敏度等级[1,10]
     * @return   {int} OK：成功，非OK：SmartIR属性更新失败
     * @note    : 仅调整底层候选判定阈值；网页过滤时间由共享模式控制器独占。
     */
    int set_sensitivity(unsigned int nLevel);
    /**
     * @brief   : 获取共享层最后成功接受的夜晚状态
     * @return   {bool} true：夜晚，false：白天
     * @note    : 仅供仍需读取运行态的AI模块使用，不参与日夜裁决或硬件控制。
     */
    bool is_night_mode() const;
    /**
     * @brief   : 设置自动观测结果回调
     * @param    {ObservationCallback} stCallback：接收建议日夜状态的回调
     * @return   {void}
     */
    void set_observation_callback(ObservationCallback stCallback);

private:
    /**
     * @brief   : SmartIR静态回调入口
     * @param    {rk_smart_ir_result_t} stResult：SmartIR本轮观测结果
     * @return   {void}
     */
    static void smart_ir_callback(rk_smart_ir_result_t stResult);
    /**
     * @brief   : 处理SmartIR自动观测结果
     * @param    {rk_smart_ir_result_t} stResult：SmartIR本轮观测结果
     * @return   {void}
     */
    void handle_smart_ir_result(rk_smart_ir_result_t stResult);
    /**
     * @brief   : 释放一次正在执行的观测回调
     * @return   {void}
     * @note    : 回调在锁外执行，停止流程通过条件变量等待所有已取出的回调返回。
     */
    void release_observation_callback();
    /**
     * @brief   : 将缓存属性同步至运行中的SmartIR
     * @return   {int} OK：成功，非OK：属性设置失败
     * @note    : 调用方必须持有m_mtxObserver。
     */
    int apply_attr_locked();

    /* lock: 保护SmartIR生命周期、属性、最后成功运行态和回调，避免回调与停止并发访问失效上下文。 */
    std::mutex m_mtxObserver;
    /* memory: RK AIQ上下文由CIspControl拥有，仅在ISP生命周期内借用，禁止在本类释放。 */
    rk_aiq_sys_ctx_t *m_pstAiqContext;
    /* memory: SmartIR上下文由本类创建和销毁；释放失败时保留指针供下一次stop重试。 */
    rk_smart_ir_ctx_t *m_pstSmartIrContext;
    /* 写入SmartIR的观测配置；过滤时间不写入此结构，由共享模式控制器处理。 */
    rk_smart_ir_attr_t m_stSmartIrAttr;
    /* 共享层最后一次成功接受的日夜状态。 */
    std::atomic<bool> m_bAcceptedNight;
    /* 当前底层候选灵敏度，保证共享服务先设灵敏度后启动时不会被默认值覆盖。 */
    unsigned int m_nSensitivity;
    /* SmartIR是否已启动；false不代表释放失败的context可以被覆盖。 */
    bool m_bRunning;
    /* lock: 由m_mtxObserver保护，回调执行时复制后在锁外调用。 */
    ObservationCallback m_stObservationCallback;
    /* lock: 记录已从控制器取出的回调，保证stop返回后适配器可以安全销毁。 */
    std::condition_variable m_stObservationCallbackIdleCv;
    /* lock: 受m_mtxObserver保护的异步观测回调计数。 */
    unsigned int m_nActiveObservationCallbacks;
};
