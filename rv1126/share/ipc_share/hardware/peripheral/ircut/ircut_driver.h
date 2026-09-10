/**
 * @FilePath     : ircut_driver.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-07-17 11:56:18
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-07-18 09:28:44
 * @Description  : 摄像机IR-CUT独立设备驱动声明
 */

#pragma once

#include <mutex>

#include "ircut_profile.h"

/**
 * @brief 摄像机IR-CUT唯一GPIO写入口。
 */
class CIrCutDriver
{
public:
    /**
     * @brief   : 构造IR-CUT设备驱动
     * @param    {const IrCutProfile_S&} stProfile：稳定设备画像
     * @return   {void}
     */
    explicit CIrCutDriver(const IrCutProfile_S &stProfile);
    /**
     * @brief   : 销毁驱动，析构不执行新的IR-CUT脉冲
     * @return   {void}
     */
    ~CIrCutDriver() = default;

    CIrCutDriver(const CIrCutDriver &) = delete;
    CIrCutDriver &operator=(const CIrCutDriver &) = delete;

    /**
     * @brief   : 将IR-CUT切换至指定位置
     * @param    {IrCutTarget_E} enTarget：DAY、NIGHT或NONE
     * @return   {int} OK：成功或无需动作，非OK：参数、能力或GPIO失败
     */
    int switch_target(IrCutTarget_E enTarget);

private:
    /**
     * @brief   : 执行一次双GPIO IR-CUT脉冲
     * @param    {unsigned int} nPin1Value：第一路脉冲电平
     * @param    {unsigned int} nPin2Value：第二路脉冲电平
     * @param    {const char*} pTargetName：日志目标名称
     * @return   {int} OK：成功，非OK：GPIO失败
     */
    int pulse_locked(unsigned int nPin1Value, unsigned int nPin2Value, const char *pTargetName);

    /* 稳定GPIO脉冲画像副本；运行中禁止修改方向、电平和脉冲宽度。 */
    IrCutProfile_S m_stProfile;
    /* lock: 串行化两路GPIO申请、脉冲和释放，禁止并发电平组合。 */
    std::mutex m_mtxDevice;
    /* 是否已经完成过一次可复用的目标切换，用于跳过重复线圈脉冲。 */
    bool m_bHasLastTarget;
    /* 最近一次完整成功的物理位置；失败动作不得覆盖该记录。 */
    IrCutTarget_E m_enLastTarget;
};
