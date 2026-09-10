/**
 * @FilePath     : peripheral_manage.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-07-17 11:39:41
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-07-20 15:34:05
 * @Description  : 外设业务门面及补光一级总控生命周期声明
 */

#pragma once

#include <mutex>

#include "Singleton.h"
#include "fill_light_gate_sink.h"
#include "fill_light_gate_controller.h"
#include "system_define.h"

/**
 * @brief 外设业务共享门面。
 */
class CPeripheralManage : public CSingleton<CPeripheralManage>
{
public:
    /**
     * @brief   : 构造外设配置与gate组合门面
     * @return   {void}
     */
    CPeripheralManage();
    /**
     * @brief   : 停止外设gate生命周期
     * @return   {void}
     */
    ~CPeripheralManage();
    friend class CSingleton<CPeripheralManage>;

    /**
     * @brief   : 加载外设配置并启动补光准入检查
     * @return   {int} OK：成功，非OK：配置读取、校验或线程启动失败
     */
    int init();

    /**
     * @brief   : 停止补光准入检查并释放已注册sink
     * @return   {int} OK：成功
     */
    int deinit();

    /**
     * @brief   : 设置外设补光配置
     * @param    {const System::Peripheral_S&} stConfig：网页协议配置
     * @return   {int} OK：成功，非OK：校验、存储或gate应用失败
     */
    int set_fill_light_config(const System::Peripheral_S &stConfig);

    /**
     * @brief   : 读取外设补光配置
     * @param    {System::Peripheral_S&} stConfig：配置输出
     * @return   {int} OK：成功，非OK：读取失败
     */
    int get_fill_light_config(System::Peripheral_S &stConfig) const;

    /**
     * @brief   : 恢复外设补光默认配置
     * @return   {int} OK：成功，非OK：失败
     */
    int restore_fill_light_default_config();

    /**
     * @brief   : 在系统校时或时区变化后立即重算补光准入状态
     * @param    {const char*} pszTrigger：时间变化来源，仅用于日志
     * @return   {int} OK：无变化或硬件已收敛，ERR_UNINIT：未初始化，其他：下游执行失败
     */
    int refresh_fill_light_gate_after_time_change(const char *pszTrigger);

    /**
     * @brief   : 注册补光gate消费者并立即重放当前状态
     * @param    {IFillLightGateSink*} pSink：非拥有sink指针
     * @return   {int} OK：成功，非OK：注册或重放失败
     */
    int set_fill_light_gate_sink(IFillLightGateSink *pSink);

    /**
     * @brief   : 清除补光gate消费者并等待正在进行的回调结束
     * @param    {IFillLightGateSink*} pSink：待清除的非拥有sink指针
     * @return   {int} OK：成功，非OK：参数或指针不匹配
     */
    int clear_fill_light_gate_sink(IFillLightGateSink *pSink);

private:
    /**
     * @brief   : 读取并转换外设补光配置，非法时恢复协议默认值
     * @param    {Peripheral_NS::FillLightGlobalConfig_S&} stBusinessConfig：强类型配置输出
     * @return   {int} OK：成功，非OK：默认值转换或持久化失败
     */
    int load_or_restore_default_config(Peripheral_NS::FillLightGlobalConfig_S &stBusinessConfig);

    /* 补光gate控制器。 */
    CFillLightGateController m_stGateController;
    /* lock: 保护门面初始化状态及控制器生命周期。 */
    mutable std::mutex m_mtxLifecycle;
    /* 是否完成初始化。 */
    bool m_bInitialized;
};
