/**
 * @FilePath     : ai_student_business.hpp
 * @Author       : zhengxh@kfb.cn
 * @Date         : 2026-04-02 20:24:10
 * @LastEditors  : zhengxh
 * @LastEditTime : 2026-04-03 10:00:00
 * @Description  : 学生行为分析事务类
 */

#pragma once
#ifdef ENABLE_AI_STUDENT
#include "Singleton.h"
#include <functional>
#include <memory>
#include "StudentBehaviorExt.hpp"
#include "ai_classroom.hpp"

namespace AiStudentBusiness_NS {
class CAiStudentBusiness : public CSingleton<CAiStudentBusiness> {
    CAiStudentBusiness() = default;

  public:
    ~CAiStudentBusiness() = default;
    friend class CSingleton<CAiStudentBusiness>;

    /**
     * @brief  : 初始化，创建班级管理对象。
     * @author : zhengxh (zhengxh@kfb.cn)
     * @return : true 表示成功，false 表示初始化失败
     */
    bool init();

    /**
     * @brief  : 去初始化，释放班级管理对象。
     * @author : zhengxh (zhengxh@kfb.cn)
     * @return : true 表示成功
     */
    bool deinit();

    /**
     * @brief      : 处理人脸特征结果
     * @param [in] : vecFaceeatures       每帧检测到的学生人脸特征
     * @param [in] : stOutData 行为分析输出数据
     */
    void Handle(std::vector<AiStudentBusiness_NS::FaceInfo_t> &vecFaceeatures);

    /**
     * @brief      : 处理学生行为分析检测结果
     * @author     : zhengxh (zhengxh@kfb.cn)
     * @param [in] : vst       每帧检测到的学生结果列表
     * @param [in] : stOutData 行为分析输出数据
     */

    void Handle(std::vector<StudentBehavior_NS::Result_S> vst, StudentBehavior_NS::OutData_S stOutData);
    /**
     * @brief  : 判断当前是否需要执行一次学生行为分析（节流控制，间隔 5 秒触发一次）。
     * @author : zhengxh (zhengxh@kfb.cn)
     * @return : true 表示需要分析，false 表示尚未到触发间隔
     */
    bool isDetect();

    /**
     * @brief       : 获取当前班级信息。
     * @author      : zhengxh (zhengxh@kfb.cn)
     * @param [out] : stClassInfo  班级信息
     * @return      : true 表示成功，false 表示班级管理对象未初始化
     */
    bool getClassInfo(ClassInfo &stClassInfo);

    /**
     * @brief       : 更新当前班级信息。
     * @param [in] : stClassInfo  班级信息
     * @return      : true 表示成功，false 表示班级管理对象未初始化
     */
    bool updateClassInfo(ClassInfo &stClassInfo);

    /**
     * @brief       : 获取本次课程的考勤记录，包含班级 ID、日期及出勤汇总。
     * @author      : zhengxh (zhengxh@kfb.cn)
     * @param [out] : stRecord  考勤记录
     * @return      : true 表示成功，false 表示班级管理对象未初始化
     */
    bool getAttendanceRecord(AttendanceRecord &stRecord);

    /**
     * @brief       : 获取本次课程的行为分析记录
     * @author      : zhengxh (zhengxh@kfb.cn)
     * @param [out] : stRecord  行为分析记录
     * @return      : true 表示成功，false 表示班级管理对象未初始化
     */
    bool getStudentBehaviorRecord(StudentBehaviorRecord &stRecord);

    /**
     * @brief       : 获取本次课程的课堂表现统计。
     * @author      : zhengxh (zhengxh@kfb.cn)
     * @param [out] : stRecord  课堂表现统计
     * @return      : true 表示成功，false 表示班级管理对象未初始化
     */
    bool getStudentPerformanceRecord(StudentPerformanceRecord &stRecord);

  private:
    /* 班级信息管理 */
    std::shared_ptr<CAiClassRoom> m_pClassRoom;
    /* 上一次分析时间戳 */
    int64_t m_nLastTime = 0;
};

}  // namespace AiStudentBusiness_NS

#endif