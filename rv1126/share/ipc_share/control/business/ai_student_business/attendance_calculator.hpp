/*
 * @Author: lianghy lianghy@kfb.cn
 * @Date: 2026-04-08 09:12:00
 * @LastEditors: lianghy lianghy@kfb.cn
 * @LastEditTime: 2026-04-17 10:50:48
 * @FilePath: /1126/share/ipc_share/control/business/ai_student_business/attendance_calculator.hpp
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
/***
 * @FilePath     : attendance_calculator.hpp
 * @Author       : zhengxh (zhengxh@kfb.cn)
 * @Date         : 2026-04-03 10:00:00
 * @LastEditors  : zhengxh
 * @LastEditTime : 2026-04-03 10:00:00
 * @Description  : 出勤情况计算器，基于视频帧人头数样本统计出勤、缺勤、迟到、早退人数。
 *                 所有公共接口均线程安全。
 */
#pragma once
#ifdef ENABLE_AI_STUDENT
#include "ai_student_define.h"
#include <map>
#include <shared_mutex>
#include <functional>
#include <string>
#include <vector>

namespace AiStudentBusiness_NS {

class CAttendanceCalculator {
public:
    using AttendanceCallback = std::function<void(const AttendanceSummary &)>;

    CAttendanceCalculator() = default;

    /**
     * @brief      : 设置班级应到总人数（含教师），班级信息变更时需重新调用。
     * @author     : zhengxh (zhengxh@kfb.cn)
     * @param [in] : nTotal  应到总人数，必须大于 0
     */
    void setTotal(int nTotal);

    /**
     * @brief      : 投入一帧人头数样本，内部积累到阈值后自动刷新出勤汇总缓存。
     * @param [in] : vecFaceeatures  当前帧检测到的人脸信息
     * @param [in] : stStudentsInfo 已经录入的人脸数据
     * @return     : true 表示成功，false 表示参数非法
     */
    bool addSample(std::vector<AiStudentBusiness_NS::FaceInfo_t> &vecFaceeatures, std::vector<Student> &stStudentsInfo);

    /**
     * @brief       : 获取最新出勤汇总（读取缓存，线程安全）。
     * @author      : zhengxh (zhengxh@kfb.cn)
     * @param [out] : stOut  出勤汇总结构体
     * @return      : true 表示成功
     */
    bool getSummary(AttendanceSummary &stOut) const;

    /**
     * @brief  : 重置所有统计状态，用于新课开始时清零。
     * @author : zhengxh (zhengxh@kfb.cn)
     */
    void reset();

    void setOnAttendanceCallback(AttendanceCallback fn);

private:
    /* 刷新内部缓存，调用方必须持有写锁 */
    void refreshSummary();

    /* 从当前样本中估算出勤人数（众数优先，否则取加权均值） */
    int calcPresent() const;
    /* 计算缺勤人数：应到人数 - 出勤人数 */
    int calcAbsent() const;
    /* 计算迟到人数：课程开始后新增进入的人数 */
    int calcLate() const;
    /* 计算早退人数：出勤峰值与当前出勤的差值 */
    int calcEarlyLeave() const;

private:
    mutable std::shared_mutex m_mutex;

    /* 应到总人数，由外部通过 setTotal 注入 */
    int m_nTotal = 0;
    /* 当前周期内已累积的样本帧数 */
    int m_nSampleCount = 0;
    /* 每帧人头数 -> 出现次数，用于众数/均值估算 */
    std::map<int, int> m_mapHumanCount;

    /* 记录哪一帧识别到哪些学生 */
    std::map<int, std::vector<Student>> m_mapFrameStudentInfo;

    /* 最新出勤汇总缓存，由 refreshSummary 写入 */
    AttendanceSummary m_stCachedSummary;

    /* 通知函数 */
    AttendanceCallback m_fnNotify;
};

} // namespace AiStudentBusiness_NS

#endif