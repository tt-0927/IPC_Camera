/*** 
 * @FilePath     : event_task.h
 * @Author       : huangjunda
 * @Date         : 2025-04-29 09:54:20
 * @LastEditors  : cyc
 * @LastEditTime : 2025-09-26 15:07:01
 * @Description  : 检索事件处理
 */

#pragma once

#ifdef SCENE_INTELLIGENT_ANALYSIS
#include <vector>
#include <string>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <atomic>
#include <cstdio>
#endif

#include "task_sub_class.h"

namespace Task
{
    namespace Event
    {

        /**
         * @brief   : 普通事件
         */
        /* 移动侦测 */
        TaskSubClass(GetMotionDetectionInfo)
        TaskSubClass(SetMotionDetectionInfo)
        /* 遮挡侦测 */
        TaskSubClass(GetHideAlarmInfo)
        TaskSubClass(SetHideAlarmInfo)
        /* 异常报警 */
        TaskSubClass(GetAbnormalAlarmInfo)
        TaskSubClass(SetAbnormalAlarmInfo)
        /* 声音报警 */
        TaskSubClass(GetAudioAlarmInfo)
        TaskSubClass(SetAudioAlarmInfo)
        TaskSubClass(EditAudioAlarmCustomInfo)
        TaskSubClass(GetAudioAlarmCustomInfo)
        TaskSubClass(SetAudioAlarmCustomInfo)
        /* 报警输入 */
        TaskSubClass(GetIoInputInfo)
        TaskSubClass(SetIoInputInfo)
        /* 报警输出 */
        TaskSubClass(GetIoOutputInfo)
        TaskSubClass(SetIoOutputInfo)
        /* 闪光报警 */
        TaskSubClass(GetFlashAlarmInfo)
        TaskSubClass(SetFlashAlarmInfo)
        /* PIR报警 */
        TaskSubClass(GetPirAlarmInfo)
        TaskSubClass(SetPirAlarmInfo)

        /**
         * @brief   : 周界事件
         */
         
        /* 越界侦测 */
        TaskSubClass(GetBoundaryDetectionInfo)
        TaskSubClass(SetBoundaryDetectionInfo)
        /* 区域入侵 */
        TaskSubClass(GetFieldDetectionInfo)
        TaskSubClass(SetFieldDetectionInfo)
        /* 进入区域 */
        TaskSubClass(GetEnterRegionDetectInfo)
        TaskSubClass(SetEnterRegionDetectInfo)
        /* 离开区域 */
        TaskSubClass(GetLeaveRegionDetectInfo)
        TaskSubClass(SetLeaveRegionDetectInfo)

        /**
         * @brief   : smart事件
         */
        /* 音频异常侦测 */
        TaskSubClass(GetAudioAnomalyInfo)
        TaskSubClass(SetAudioAnomalyInfo)
        TaskSubClass(GetAudioAnomalyCurrentDb)
        /* 场景变更侦测 */
        TaskSubClass(GetSceneChangeInfo)
        TaskSubClass(SetSceneChangeInfo)
        /* 人脸侦测 */
        TaskSubClass(GetFaceDetectionInfo)
        TaskSubClass(SetFaceDetectionInfo)
        /* 徘徊侦测 */
        TaskSubClass(GetLoiteringDetectionInfo)
        TaskSubClass(SetLoiteringDetectionInfo)
        /* 人员聚集侦测 */
        TaskSubClass(GetCrowdGatheringInfo)
        TaskSubClass(SetCrowdGatheringInfo)
        /* 停车侦测 */
        TaskSubClass(GetParkDetectionInfo)
        TaskSubClass(SetParkDetectionInfo)
        /* 物品遗留侦测 */
        TaskSubClass(GetUnattendedObjectInfo)
        TaskSubClass(SetUnattendedObjectInfo)
        /* 物品拿取侦测 */
        TaskSubClass(GetObjectRemovalInfo)
        TaskSubClass(SetObjectRemovalInfo)
        /* 宠物识别 */
        TaskSubClass(GetPetRecognitionInfo)
        TaskSubClass(SetPetRecognitionInfo)
        /* 人脸比对 */
        TaskSubClass(GetFaceCompareInfo)
        TaskSubClass(SetFaceCompareInfo)
        /* 人脸抓拍 */
        TaskSubClass(GetFaceCaptureInfo)
        TaskSubClass(SetFaceCaptureInfo)
        /* 人脸抓拍叠加信息 */
        TaskSubClass(GetFaceCaptureOverlayInfo)
        TaskSubClass(SetFaceCaptureOverlayInfo)
        /*人脸名单库操作*/
        TaskSubClass(AddTargetLib)
        TaskSubClass(DelTargetLib)
        TaskSubClass(SetTargetLib)
        TaskSubClass(GetTargetLib)
        TaskSubClass(AddFaceInfo)
        TaskSubClass(DelFaceInfo)
        TaskSubClass(SetFaceInfo)
        TaskSubClass(GetFaceInfo)

#ifdef SCENE_INTELLIGENT_ANALYSIS
        /**
        * @brief   : 场景智能分析
        */
        /* 画面分析 */
        TaskSubClass(GetImageAnalysisInfo)
        TaskSubClass(SetImageAnalysisInfo)
        TaskSubClass(RtImageAnalysisInfoResult)
        TaskSubClass(CtrlImageAnalysisStop)
        TaskSubClass(OperateImageAnalysisRecord)
        /* 文字预设任务 */
        TaskSubClass(GetTextPresetTaskInfo)
        TaskSubClass(SetTextPresetTaskInfo)
        /* 实时预警推送 */
        TaskSubClass(GetRealAlarmPushInfo)
        TaskSubClass(GetRealAlarmProcessInfo)
        TaskSubClass(SetRealAlarmPushInfo)
#endif

#ifdef SCENE_INTELLIGENCE
        /**
        * @brief   : 场景智能
        */

        /* 预览页面智能属性识别开关信息 */
        TaskSubClass(SetAttributeInfo)
        TaskSubClass(GetAttributeInfo)

        /* 推送人脸抓拍信息 */
        TaskSubClass(PushFaceCaptureInfo)
        TaskSubClass(SetPushFaceCaptureInfo)

        /* 推送行人抓拍信息 */
        TaskSubClass(PushPersonCaptureInfo)
        /* 推送机动车抓拍信息 */
        TaskSubClass(PushMotorVehicleCaptureInfo)
        /* 推送非机动车抓拍信息 */
        TaskSubClass(PushNonMotorVehicleCaptureInfo)

        // 翻越围栏
        TaskSubClass(GetFenceClimbingInfo)
        TaskSubClass(SetFenceClimbingInfo)

        // 离岗
        TaskSubClass(GetLeavePostInfo)
        TaskSubClass(SetLeavePostInfo)

        // 违规变道
        TaskSubClass(GetIllegalLaneChangeInfo)
        TaskSubClass(SetIllegalLaneChangeInfo)

        // 逆行
        TaskSubClass(GetReverseDirectionInfo)
        TaskSubClass(SetReverseDirectionInfo)

        // 非机动车闯入
        TaskSubClass(GetNonMotorVehicleIntrusionInfo)
        TaskSubClass(SetNonMotorVehicleIntrusionInfo)

        // 应急车道占用
        TaskSubClass(GetEmergencyLaneOccupancyInfo)
        TaskSubClass(SetEmergencyLaneOccupancyInfo)

        // 行人闯入检测
        TaskSubClass(GetPedestrianIntrusionInfo)
        TaskSubClass(SetPedestrianIntrusionInfo)

        // 烟火识别
        TaskSubClass(GetSmokeFireInfo)
        TaskSubClass(SetSmokeFireInfo)

        // 道路积水检测
        TaskSubClass(GetRoadPondingInfo)
        TaskSubClass(SetRoadPondingInfo)

        // 井盖异常检测
        TaskSubClass(GetManholeCoverAbnormalInfo)
        TaskSubClass(SetManholeCoverAbnormalInfo)

        // 睡岗识别
        TaskSubClass(GetSleepOnDutyInfo)
        TaskSubClass(SetSleepOnDutyInfo)

        // 电瓶车进电梯识别
        TaskSubClass(GetElectricVehicleInElevatorInfo)
        TaskSubClass(SetElectricVehicleInElevatorInfo)

        // 人员倒地识别
        TaskSubClass(GetPersonFallDownInfo)
        TaskSubClass(SetPersonFallDownInfo)

        // 施工占道识别
        TaskSubClass(GetConstructionOccupyRoadInfo)
        TaskSubClass(SetConstructionOccupyRoadInfo)

        // 拥堵识别
        TaskSubClass(GetCongestionInfo)
        TaskSubClass(SetCongestionInfo)

        // 车牌识别
        TaskSubClass(GetLicensePlateRecognitionInfo)
        TaskSubClass(SetLicensePlateRecognitionInfo)

        // 高空安全带识别
        TaskSubClass(GetHighAltitudeSeatbeltInfo)
        TaskSubClass(SetHighAltitudeSeatbeltInfo)

        // 安全帽识别
        TaskSubClass(GetSafetyHelmetInfo)
        TaskSubClass(SetSafetyHelmetInfo)

        // 摔倒识别
        TaskSubClass(GetPersonFallInfo)
        TaskSubClass(SetPersonFallInfo)

        // 玩手机识别
        TaskSubClass(GetPhoneUsageInfo)
        TaskSubClass(SetPhoneUsageInfo)

        // 抽烟识别
        TaskSubClass(GetSmokingInfo)
        TaskSubClass(SetSmokingInfo)

        // 明火识别
        TaskSubClass(GetOpenFlameInfo)
        TaskSubClass(SetOpenFlameInfo)

        // 黄土裸露识别
        TaskSubClass(GetBareSoilInfo)
        TaskSubClass(SetBareSoilInfo)

        // 洞口防护栏识别
        TaskSubClass(GetHoleProtectionBarInfo)
        TaskSubClass(SetHoleProtectionBarInfo)

        // 反光衣识别
        TaskSubClass(GetReflectiveClothingInfo)
        TaskSubClass(SetReflectiveClothingInfo)
#endif

#if defined(SCENE_INTELLIGENCE) || CAP_AI_GARBAGE_DETECT
        // 垃圾暴露识别
        TaskSubClass(GetGarbageExposureInfo)
        TaskSubClass(SetGarbageExposureInfo)

        // 垃圾满溢识别
        TaskSubClass(GetGarbageOverflowInfo)
        TaskSubClass(SetGarbageOverflowInfo)
#endif

#if CAP_AI_PEOPLE_STATISTICS
        /* 人流统计 */
        TaskSubClass(GetPeopleFlowStatisticsInfo)
        TaskSubClass(SetPeopleFlowStatisticsInfo)
        TaskSubClass(ClearPeopleFlowStatisticsResult)
        /* 人员密度检测 */
        TaskSubClass(GetPeopleDensityDetectionInfo)
        TaskSubClass(SetPeopleDensityDetectionInfo)
#endif
    } /* namespace Event end */
} /* namespace Task end */





#ifdef SCENE_INTELLIGENT_ANALYSIS

/**
  * @brief : 后台文件清理器 (单例模式)
  * 功能：维护唯一的一个后台线程，串行处理文件删除请求
*/
class AnalysisFileCleaner {
public:
    /*获取单例实例*/
    static AnalysisFileCleaner& instance() {
        static AnalysisFileCleaner instance;
        return instance;
    }

    /*提交需要删除的文件路径列表 */
    void pushTasks(const std::vector<std::string>& filePaths) {
        if (filePaths.empty()) return;
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            for (const auto& path : filePaths) {
                if (!path.empty()) {
                    m_tasks.push(path);
                }
            }
        }
        /*唤醒后台线程*/
        m_cv.notify_one();
    }

    /*析构函数：确保程序退出时线程结束*/
    ~AnalysisFileCleaner() {
        m_stop = true;
        m_cv.notify_all(); // 唤醒线程让它退出
        if (m_worker.joinable()) {
            m_worker.join();
        }
    }

    /* 禁止拷贝*/
    AnalysisFileCleaner(const AnalysisFileCleaner&) = delete;
    AnalysisFileCleaner& operator=(const AnalysisFileCleaner&) = delete;

private:
    std::mutex m_mutex;
    std::condition_variable m_cv;
    std::queue<std::string> m_tasks;
    std::atomic<bool> m_stop;
    std::thread m_worker;

    /* 私有构造，启动线程*/
    AnalysisFileCleaner() : m_stop(false) {
        m_worker = std::thread([this] {
            while (true) {
                std::string filePath;
                {
                    std::unique_lock<std::mutex> lock(m_mutex);
                    /* 等待条件：任务队列不为空 或 收到停止信号*/
                    m_cv.wait(lock, [this] { return m_stop || !m_tasks.empty(); });

                    if (m_stop && m_tasks.empty()) return; // 退出线程

                    if (!m_tasks.empty()) {
                        filePath = m_tasks.front();
                        m_tasks.pop();
                    }
                }

                /* 在锁外执行删除，避免阻塞任务添加*/
                if (!filePath.empty()) {
                    std::remove(filePath.c_str());
                }
            }
        });
    }
};

#endif
