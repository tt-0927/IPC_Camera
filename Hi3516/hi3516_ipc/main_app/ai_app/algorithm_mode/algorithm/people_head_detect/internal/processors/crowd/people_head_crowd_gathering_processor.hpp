/**
 * @FilePath     : people_head_crowd_gathering_processor.hpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-04-23 17:05:08
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-04-28 20:32:09
 * @Description  : 人头检测-人员聚集处理器
 */

#ifndef PEOPLE_HEAD_CROWD_GATHERING_PROCESSOR_HPP
#define PEOPLE_HEAD_CROWD_GATHERING_PROCESSOR_HPP

#include "algorithm.hpp"
#include "people_head_detect_common.hpp"

namespace PeopleHeadDetectInternal
{
class CPeopleHeadCrowdGatheringProcessor
{
public:
    /**
     * @brief   : 设置人员聚集使能状态
     * @param    {bool} bEnable：true：使能 false：关闭
     * @return   {void}
     */
    void setEnabled(bool bEnable);

    /**
     * @brief   : 设置人员聚集参数
     * @param    {CrowdGathering_S} &stAlgoCfg：人员聚集配置
     * @param    {int} nWidth：算法分辨率宽度
     * @param    {int} nHeight：算法分辨率高度
     * @return   {void}
     */
    void setAlgoParamCfg(const Alarm::CrowdGathering_S &stAlgoCfg, int nWidth, int nHeight);

    /**
     * @brief   : 处理单帧人员聚集检测结果
     * @param    {SPeopleHeadProcessContext &} stContext：单帧处理上下文
     * @return   {void}
     */
    void process(SPeopleHeadProcessContext &stContext);

    /**
     * @brief   : 获取当前是否使能
     * @return   {bool} true：使能 false：关闭
     */
    bool isEnabled() const;

private:
    /* 人员聚集配置 */
    Alarm::CrowdGathering_S m_stAlgoCfg;
    /* 人员聚集报警状态机 */
    CAlarmStateMachine m_alarmStateMachine;
};
} // namespace PeopleHeadDetectInternal

#endif
