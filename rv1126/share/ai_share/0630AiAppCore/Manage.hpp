
#pragma once

#include "0630AppExtern.hpp"
#include "CtrlComm.hpp"
#include "DataComm.hpp"
#include "DevComm.hpp"
#include "FaceManage.hpp"
#include "GetClassInfo.hpp"
#include "ResultModule.hpp"
#include "singleton.h"
#include "TimerManager.hpp"

namespace Ai0630_NS
{
    class Manage : public CSingleton<Manage>
    {
    public:

        Manage();
        ~Manage() = default;
        friend class CSingleton<Manage>;

        /**
         * @brief 设置回调函数
         * @param func 外部传入的回调函数
         */
        void setNotifyGetSteamData(notifyGetSteamDataFunc func);
        void setPTZControl(PTZControlFunc func);

        /**
         * @brief 发送流数据
         * @param pchData
         * @param nDataLen
         * @param nCode
         */
        void sendSteamData(char* pchData, int nDataLen, int nCode, int nRecordTime);

        /**
         * @brief 整合
         */
        void finalize(const void* pParam);

        /**
         * @brief PPT切换
         * @return BlError_E
         */
        BlError_E pptSwitch();

        /**
         * @brief 切换老师画面
         * @return BlError_E
         */
        BlError_E switchTeacherScreen();

        /**
         * @brief 切换学生画面
         * @return BlError_E
         */
        BlError_E switchStudentScreen();

        /**
         * @brief 切换学生特写
         * @param [bool]  bValue: 是否学生特写
         * @return BlError_E
         */
        BlError_E switchStudentCloseUp(bool bValue);

        /**
         * @brief 切换老师是否在讲台
         * @param [bool]  bValue: 是否老师在讲台
         * @return BlError_E
         */
        BlError_E switchTeacherPodium(bool bValue);


        /**
         * @brief 开始互动
         * @return BlError_E
         */
        BlError_E startInteraction();

        /**
         * @brief 开始巡视行为
         * @return BlError_E
         */
        BlError_E startTour();

        /**
         * @brief 开始教授行为
         * @return BlError_E
         */
        BlError_E startTaught();

        /**
         * @brief 老师板书切换
         * @param bValue 是否板书
         * @return BlError_E
         */
        BlError_E teacherBoard(bool bValue);

        /**
         * @brief 设置录制时间
         * @param nRecordTime
         * @return BlError_E
         */
        BlError_E setRecordTime(int nRecordTime);

        /**
         * @brief 设置录制状态
         * @param nStatus 录制状态
         * @return BlError_E
         */
        BlError_E setRecordStatus(int nStatus);


        /**
         * @brief 设置服务器IP
         * @param strIp IP
         * @return BlError_E
         */
        BlError_E setServerIp(std::string strIp);

        /**
         * @brief 添加课堂分贝值
         * @param nDb 分贝值
         * @return
         */
        void addClassDb(int nDb);

        /**
         * @brief 获取并更新班级信息
         * @return BlError_E
         */
        BlError_E update_classInfo()
        {
            return m_pGetClassInfo->update_classInfo();
        }

        /**
         * @brief 获取班级信息
         * @return m_stClassInfo
         */
        ClassInfo_S getClassInfo();

        /**
         * @brief 获取AI服务器信息
         * @return AiServerInfo_S
         */
        AiServerInfo_S getAiServerInfo()
        {
            return m_pCtrlComm->getAiServerInfo();
        }

        /**
         * @brief 获取AI功能开关信息
         * @return AiSwitchInfo_S
         */
        AiSwitchInfo_S getAiSwitchInfo()
        {
            return m_stAiSwitchInfo;
        }

        /**
         * @brief 设置AI功能开关信息
         * @param stInfo 信息
         * @return
         */
        void setAiServerInfo(AiSwitchInfo_S stInfo)
        {
            m_stAiSwitchInfo = stInfo;
        }

    private:

        /**
         * @brief 初始化定时任务
         */
        void initTask();

        /**
         * @brief 检查状态，判断是否开启定时器
         */
        void checkTask();

        /**
         * @brief 获取stream数据定时函数
         * @param param
         */
        void taskGetSteam(std::shared_ptr<void> param);

        /**
         * @brief 获取stream数据定时函数
         * @param param
         */
        void taskGetStudentSteam(std::shared_ptr<void> param);

        /**
         * @brief 接受班级信息
         * @param stClassInfo 班级信息
         */
        void recvClassData(ClassInfo_S stClassInfo);

    private:

        std::shared_ptr<DataComm>     m_pDataComm;
        std::shared_ptr<CtrlComm>     m_pCtrlComm;
        std::shared_ptr<DevComm>      m_pDevComm;
        std::shared_ptr<FaceManage>   m_pFaceManage;
        std::shared_ptr<ResultModule> m_pResultModule;
        std::shared_ptr<GetClassInfo> m_pGetClassInfo;

        notifyGetSteamDataFunc m_notifyGetSteamData = nullptr;
        PTZControlFunc         m_setPTZControl      = nullptr;

        int              m_nRecordTime = 0;
        std::atomic_bool m_bRecord     = false;     /* 是否在录制 */

        AiSwitchInfo_S m_stAiSwitchInfo;            /* AI功能开关 */

        std::atomic_bool m_bStudentScreen  = false; /* 是否学生画面 */
        std::atomic_bool m_bStudentCloseUp = false; /* 是否学生特写 */
        std::atomic_bool m_bTeacherPodium  = true;  /* 是否老师在讲台 */


        /* 定时器句柄 */
        uint64_t m_nFaceTaskId     = 0; /* 人脸识别 */
        uint64_t m_nHeadTaskId     = 0; /* 人头识别 */
        uint64_t m_nEmoTaskId      = 0; /* 班级表情识别 */
        uint64_t m_nBehaviorTaskId = 0; /* 班级行为分析 */
        uint64_t m_nStTaskId       = 0; /* 学生个人分析 */
        uint64_t m_nTeTaskId       = 0; /* 老师个人分析 */
    };

}    // namespace Ai0630_NS
