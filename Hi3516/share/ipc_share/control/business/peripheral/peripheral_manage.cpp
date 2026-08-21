/**
 * @FilePath     : peripheral_manage.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-07-17 11:39:41
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-07-20 15:34:05
 * @Description  : 外设业务门面及补光一级总控生命周期实现
 */

#include "peripheral_manage.h"

#include "IpcRet.h"
#include "dlog.h"
#include "peripheral_configure.h"
#include "peripheral_fill_light_config.h"

CPeripheralManage::CPeripheralManage() : m_bInitialized(false)
{
}

CPeripheralManage::~CPeripheralManage()
{
    deinit();
}

int CPeripheralManage::init()
{
    /* lock: 门面初始化与配置命令互斥，防止Gate尚未启动时接收外部写入。 */
    std::lock_guard<std::mutex> stLock(m_mtxLifecycle);
    if (m_bInitialized)
    {
        return OK;
    }

    Peripheral_NS::FillLightGlobalConfig_S stBusinessConfig;
    int nRet = load_or_restore_default_config(stBusinessConfig);
    if (nRet != OK)
    {
        return nRet;
    }

    /* 先恢复确定的配置，再启动秒级Gate worker，避免未知默认值短暂放行灯光。 */
    nRet = m_stGateController.init(stBusinessConfig);
    if (nRet != OK)
    {
        return nRet;
    }

    m_bInitialized = true;
    return OK;
}

int CPeripheralManage::load_or_restore_default_config(Peripheral_NS::FillLightGlobalConfig_S &stBusinessConfig)
{
    /* 文件成功不代表字段合法，必须经强类型转换校验后才允许启动Gate。 */
    System::Peripheral_S stProtocolConfig;
    int nRet = CPeripheralConfigure::instance()->get_configure(stProtocolConfig);
    if (nRet == OK)
    {
        nRet = Peripheral_NS::decode_fill_light_config(stProtocolConfig, stBusinessConfig);
        if (nRet == OK)
        {
            return OK;
        }
    }

    /* warn: 配置文件读取失败或字段越界时恢复协议默认值，避免gate以未知参数启动。 */
    dlog_warn("外设补光模块配置不可用，尝试恢复默认配置: %d", nRet);
    /* 使用协议默认DTO而非手工字段赋值，保持恢复结果与网页默认值一致。 */
    stProtocolConfig = System::Peripheral_S{};
    nRet = Peripheral_NS::decode_fill_light_config(stProtocolConfig, stBusinessConfig);
    if (nRet != OK)
    {
        dlog_error("外设补光协议默认配置非法: %d", nRet);
        return nRet;
    }

    nRet = CPeripheralConfigure::instance()->set_configure(stProtocolConfig);
    if (nRet != OK)
    {
        dlog_error("外设补光模块保存默认配置失败: %d", nRet);
    }
    return nRet;
}

int CPeripheralManage::deinit()
{
    /* lock: 停止期间禁止并发配置事务重新启动或替换Gate状态。 */
    std::lock_guard<std::mutex> stLock(m_mtxLifecycle);
    if (!m_bInitialized)
    {
        return OK;
    }

    const int nRet = m_stGateController.deinit();
    m_bInitialized = false;
    return nRet;
}

int CPeripheralManage::set_fill_light_config(const System::Peripheral_S &stConfig)
{
    /* 未完成Gate初始化时拒绝写入，避免文件状态领先于硬件运行态。 */
    std::lock_guard<std::mutex> stLock(m_mtxLifecycle);
    if (!m_bInitialized)
    {
        return ERR_UNINIT;
    }
    Peripheral_NS::FillLightGlobalConfig_S stBusinessConfig;
    int nRet = Peripheral_NS::decode_fill_light_config(stConfig, stBusinessConfig);
    if (nRet != OK)
    {
        dlog_warn("外设补光配置校验失败, mode:%d, level:%u, ret:%d", stConfig.nLightMode, stConfig.nLevel, nRet);
        return nRet;
    }

    /* 先更新运行态，避免gate应用失败时持久化尚未生效的配置。 */
    nRet = m_stGateController.update_config(stBusinessConfig);
    if (nRet != OK)
    {
        dlog_error("应用外设补光配置失败: %d", nRet);
        return nRet;
    }

    nRet = CPeripheralConfigure::instance()->set_configure(stConfig);
    if (nRet != OK)
    {
        dlog_error("持久化外设补光配置失败: %d", nRet);
        return nRet;
    }

    dlog_info("外设补光配置生效, enable:%d, mode:%d, limit:%u", stConfig.bEnable, stConfig.nLightMode, stConfig.nLevel);
    return OK;
}

int CPeripheralManage::get_fill_light_config(System::Peripheral_S &stConfig) const
{
    std::lock_guard<std::mutex> stLock(m_mtxLifecycle);
    return CPeripheralConfigure::instance()->get_configure(stConfig);
}

int CPeripheralManage::restore_fill_light_default_config()
{
    /* 默认值复用设置入口，避免重复校验、gate应用和持久化流程。 */
    return set_fill_light_config(System::Peripheral_S{});
}

int CPeripheralManage::refresh_fill_light_gate_after_time_change(const char *pszTrigger)
{
    /* lock: 校时重算必须与外设配置事务串行，防止新时间按旧配置发布gate。 */
    std::lock_guard<std::mutex> stLock(m_mtxLifecycle);
    if (!m_bInitialized)
    {
        return ERR_UNINIT;
    }
    return m_stGateController.refresh_after_time_change(pszTrigger);
}

int CPeripheralManage::set_fill_light_gate_sink(IFillLightGateSink *pSink)
{
    /* Gate控制器在注册时会重放当前快照，调用方必须已完成运行态对象构造。 */
    std::lock_guard<std::mutex> stLock(m_mtxLifecycle);
    return m_stGateController.set_sink(pSink);
}

int CPeripheralManage::clear_fill_light_gate_sink(IFillLightGateSink *pSink)
{
    /* clear_sink是回调生命周期屏障，返回后调用方才可安全析构sink。 */
    std::lock_guard<std::mutex> stLock(m_mtxLifecycle);
    return m_stGateController.clear_sink(pSink);
}
