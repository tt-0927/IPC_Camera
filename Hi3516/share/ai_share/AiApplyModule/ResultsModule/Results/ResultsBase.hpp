/*
 * @FilePath     : ResultsBase.hpp
 * @Author       : 严泽辉 yanzeh@kfb.cn
 * @Date         : 2024-03-28 09:21:51
 * @LastEditors: weigl 308241951@qq.com
 * @LastEditTime: 2025-02-27 09:50:53
 * @Description  :
 */
#pragma once

#include <atomic>
#include <mutex>

#include "AiManageExtern.hpp"
#include "ResultsModuleExtern.hpp"

namespace ResultsModule_NS
{
    class CResultsBase
    {

    public:

        CResultsBase(InParam_S stInfo);

        virtual ~CResultsBase();

    protected:

        /** 
         * @brief 结束处理-热词提取信息
         * @param [void*] pParam: 不定参数，不同子类可能需要不同的参数类型
         * @return [*] 
         */
        virtual BlError_E endDeal_hotwordExtInfo(const void* pParam) = 0;

        /**
         * @brief 结束处理-课堂信息
         * @param [void*] pParam: 不定参数，不同子类可能需要不同的参数类型
         * @return [*]
         * @note
         */
        virtual BlError_E endDeal_classSummaryInfo(const void* pParam) = 0;

        /**
         * @brief 结束处理-教师信息
         * @param [void*] pParam: 不定参数，不同子类可能需要不同的参数类型
         * @return [*]
         * @note
         */
        virtual BlError_E endDeal_teacherInfo(const void* pParam) = 0;

        /**
         * @brief 结束处理-学生信息
         * @param [void*] pParam: 不定参数，不同子类可能需要不同的参数类型
         * @return [*]
         * @note
         */
        virtual BlError_E endDeal_studentInfo(const void* pParam) = 0;

        /**
         * @brief 结束处理-考勤信息
         * @param [void*] pParam: 不定参数，不同子类可能需要不同的参数类型
         * @return [*]
         * @note
         */
        virtual BlError_E endDeal_attendanceInfo(const void* pParam) = 0;

        /**
         * @brief 读取置信度阈值
         * @param [AiConfidenceTh_S&] stAiConfidenceTh: 需要读取的结构体
         * @return [*]
         * @note 
         */
        virtual BlError_E readConfidenceTh(char *pchFilePath, AiConfidenceTh_S& stAiConfidenceTh) = 0;

    public:

        /**
         * @brief 结束AI分析
         * @param [const void*] pParam: 不定参数，不同子类可能需要不同的参数类型
         * @return [*]
         * @note
         */
        BlError_E end_aiAnalysis(const void* pParam);
        
/*+++++++++++++++++++++++++++教育云平台member++++++++++++++++++++++++++++*/

        /**
         * @brief 结束教育云平台AI分析
         * @param [const void*] pParam: 不定参数，不同子类可能需要不同的参数类型
         * @return [*]
         * @note
         */
        BlError_E end_platformAiAnalysis(const void* pParam);

        /**
         * @brief 下课处理函数
         * @param [const void*] pParam: 不定参数，不同子类可能需要不同的参数类型
         * @return [*]
         * @note
         */
        BlError_E processClassExit(const void* pParam);

        /**
         * @brief 获取当前时间戳
         * @param [*] 
         * @return [当前时间戳]
         * @note
         */
        long getSecondsTimestamp();

/*+++++++++++++++++++++++++++教育云平台member++++++++++++++++++++++++++++*/

        /**
         * @brief 处理板书识别分析数据
         * @param [BehaviorInfo_S] stInfo: 分析后的数据
         * @return [*]
         * @note
         */
        BlError_E deal_boardDetecr(AiManage_NS::BehaviorInfo_S stInfo);

        /**
         * @brief 处理表情识别分析数据
         * @param [EmoInfo_S] stInfo: 分析后的数据
         * @return [*]
         * @note
         */
        BlError_E deal_emoDetecr(AiManage_NS::EmoInfo_S stInfo);

        /**
         * @brief 处理学生人脸识别分析数据
         * @param [FaceInfo_S] stInfo: 分析后的数据
         * @return [*]
         * @note
         */
        BlError_E deal_stFaceDetecr(AiManage_NS::FaceInfo_S stInfo);

        /**
         * @brief 处理学生回答问题人脸识别分析数据
         * @param [FaceInfo_S] stInfo: 分析后的数据
         * @return [*]
         * @note
         */
        BlError_E deal_stAsFaceDetecr(AiManage_NS::FaceInfo_S stInfo);

        /**
         * @brief 处理教师人脸识别分析数据
         * @param [FaceInfo_S] stInfo: 分析后的数据
         * @return [*]
         * @note
         */
        BlError_E deal_teFaceDetecr(AiManage_NS::FaceInfo_S stInfo);

        /**
         * @brief 处理轨迹识别分析数据
         * @param [TrackInfo_S] stInfo: 分析后的数据
         * @return [*]
         * @note
         */
        BlError_E deal_trackTeacher(AiManage_NS::TrackInfo_S stInfo);

        /**
         * @brief 处理教师接打电话分析数据
         * @param [BehaviorInfo_S] stInfo: 分析后的数据
         * @return [*]
         * @note
         */
        BlError_E deal_CallPhone(AiManage_NS::BehaviorInfo_S stInfo);

        /**
         * @brief 处理学生玩手机分析数据
         * @param [BehaviorInfo_S] stInfo: 分析后的数据
         * @return [*]
         * @note
         */
        BlError_E deal_PlayPhone(AiManage_NS::BehaviorInfo_S stInfo);

        /**
         * @brief 处理人数统计分析数据
         * @param [NumberInfo_S] stInfo: 分析后的数据
         * @return [*]
         * @note
         */
        BlError_E deal_countStudents(AiManage_NS::NumberInfo_S stInfo);

        /**
         * @brief 处理课堂纪律分析数据
         * @param [MoveProbability_S] stMoveProbability: 计算的混乱度
         * @return [*]
         * @note
         */
         BlError_E deal_discipline(AiManage_NS::MoveProbability_S stMoveProbability);

        /**
         * @brief 处理学生行为分析数据
         * @param [BehaviorInfo_S] stInfo: 分析后的数据
         * @return [*]
         * @note
         */
        BlError_E deal_studentBehavior(AiManage_NS::BehaviorInfo_S stInfo);

        /**
         * @brief 切换老师画面
         * @return [*]
         * @note
         */
        BlError_E switchTeacherScreen();

        /**
         * @brief 切换学生画面
         * @return [*]
         * @note
         */
        BlError_E switchStudentScreen();

        /**
         * @brief 切换学生特写
         * @param [bool]  bValue: 是否学生特写
         * @return [*]
         * @note
         */
        BlError_E switchStudentCloseUp(bool bValue);

        /**
         * @brief 切换老师是否在讲台
         * @param [bool]  bValue: 是否老师在讲台
         * @return [*]
         * @note
         */
        BlError_E switchTeacherPodium(bool bValue);

        /**
         * @brief 开始互动行为
         * @return [*]
         * @note
         */
        BlError_E startInteraction();

        /**
         * @brief 开始巡视行为
         * @return [*]
         * @note
         */
        BlError_E startTour();

        /**
         * @brief 开始教授行为
         * @return [*]
         * @note
         */
        BlError_E startTaught();

        /**
         * @brief 教师指导
         * @return [*]
         * @note
         */
        BlError_E teacherDirecting(bool bValue);

        /**
         * @brief 教师板书
         * @param [bool] bValue: true-板书 false-不在板书
         * @return [*]
         * @note
         */
        BlError_E teacherBoard(bool bValue);

        /**
         * @brief 处理教育云平台教师板书
         * @param [bool] bValue:是否板书
         * @return [*]
         * @note
         */
        BlError_E teacherPlatformBoard(bool bValue);

        /**
         * @brief PPT切换
         * @return [*]
         * @note
         */
        BlError_E pptSwitch();

        /**
         * @brief 添加提取的热词数据
         * @param listWords 
         * @return BlError_E 
         */
        BlError_E addAudioWordsResult(std::list<std::pair<std::string, int>> listWords);

        /**
         * @brief 设置本地老师人脸识别-结果
         * @param [std::list<std::pair<int, std::string>>] listName: 老师信息
         * @return [*]
         */
        BlError_E setLocalTeFaceRecResult(std::list<std::pair<int, std::string>> listName);

        /**
         * @brief 获取当前学生考勤人数
         * @return [*]
         * @note
         */
        int getStAttendanceSize();

        /**
         * @brief 获取当前老师考勤人数
         * @return [*]
         * @note
         */
        int getTeAttendanceSize();

        /**
         * @brief 获取当前识别到的人数
         * @return [*]
         * @note
         */
        int getCurPeopleSize();

    protected:

        /**
         * @brief 开始处理-课堂信息
         * @return [*]
         * @note
         */
        BlError_E beginDeal_classSummaryInfo();

        /**
         * @brief 开始处理-教师信息
         * @return [*]
         * @note
         */
        BlError_E beginDeal_teacherInfo();

        /**
         * @brief 开始处理-学生信息
         * @return [*]
         * @note
         */
        BlError_E beginDeal_studentInfo();

        /**
         * @brief 开始处理-考勤信息
         * @return [*]
         * @note
         */
        BlError_E beginDeal_attendanceInfo();

        /**
         * @brief 处理板书识别分析数据后的处理
         * @param [BehaviorInfo_S] stInfo: 处理的数据
         * @return [*]
         * @note 默认什么都不处理，如果子类需要，直接重写函数
         */
        virtual BlError_E endDeal_boardDetecr(AiManage_NS::BehaviorInfo_S stInfo);

        /**
         * @brief 处理表情识别分析数据的处理
         * @param [EmoInfo_S] stInfo: 分析后的数据
         * @return [*]
         * @note 默认什么都不处理，如果子类需要，直接重写函数
         */
        virtual BlError_E endDeal_emoDetecr(AiManage_NS::EmoInfo_S stInfo);

        /**
         * @brief 处理学生人脸识别分析数据的处理
         * @param [FaceInfo_S] stInfo: 分析后的数据
         * @return [*]
         * @note 默认什么都不处理，如果子类需要，直接重写函数
         */
        virtual BlError_E endDeal_stFaceDetecrDetecr(AiManage_NS::FaceInfo_S stInfo);

        /**
         * @brief 处理学生回答问题人脸识别数据的处理
         * @param [FaceInfo_S] stInfo: 分析后的数据
         * @return [*]
         * @note 默认什么都不处理，如果子类需要，直接重写函数
         */
        virtual BlError_E endDeal_stAsFaceDetecrDetecr(AiManage_NS::FaceInfo_S stInfo);

        /**
         * @brief 处理教师人脸识别分析数据的处理
         * @param [FaceInfo_S] stInfo: 分析后的数据
         * @return [*]
         * @note 默认什么都不处理，如果子类需要，直接重写函数
         */
        virtual BlError_E endDeal_teFaceDetecrDetecr(AiManage_NS::FaceInfo_S stInfo);

        /**
         * @brief 处理轨迹识别分析数据的处理
         * @param [TrackInfo_S] stInfo: 分析后的数据
         * @return [*]
         * @note 默认什么都不处理，如果子类需要，直接重写函数
         */
        virtual BlError_E endDeal_trackTeacher(AiManage_NS::TrackInfo_S stInfo);

        /**
         * @brief 处理教师接打电话分析数据的处理
         * @param [BehaviorInfo_S] stInfo: 分析后的数据
         * @return [*]
         * @note 默认什么都不处理，如果子类需要，直接重写函数
         */
        virtual BlError_E endDeal_CallPhone(AiManage_NS::BehaviorInfo_S stInfo);

        /**
         * @brief 处理学生玩手机分析数据的处理
         * @param [BehaviorInfo_S] stInfo: 分析后的数据
         * @return [*]
         * @note 默认什么都不处理，如果子类需要，直接重写函数
         */
        virtual BlError_E endDeal_PlayPhone(AiManage_NS::BehaviorInfo_S stInfo);

        /**
         * @brief 处理人数统计分析数据的处理
         * @param [NumberInfo_S] stInfo: 分析后的数据
         * @return [*]
         * @note 默认什么都不处理，如果子类需要，直接重写函数
         */
        virtual BlError_E endDeal_countStudents(AiManage_NS::NumberInfo_S stInfo);

        /**
         * @brief 处理学生行为分析数据的处理
         * @param [BehaviorInfo_S] stInfo: 分析后的数据
         * @return [*]
         * @note 默认什么都不处理，如果子类需要，直接重写函数
         */
        virtual BlError_E endDeal_studentBehavior(AiManage_NS::BehaviorInfo_S stInfo);

    private:

        /**
         * @brief 添加学生行为
         * @param [std::list<TimeSlotInfo_S>&] listTime: 学生行为时间段
         * @param [bool] bBehavior: 是否发生该行为
         * @param [int] nRecordTime: 录制时长
         * @return [*]
         * @note
         */
        BlError_E add_studentBehavior(
            std::list<TimeSlotInfo_S>& listTime,
            bool                       bBehavior,
            int                        nRecordTime);

        /**
         * @brief 保存学生全景截图
         * @param [int] nRecordTime: 录制时长
         * @return [*]
         * @note
         */
        BlError_E save_stFullView(int nRecordTime);

/*+++++++++++++++++++++++++++教育云平台Func++++++++++++++++++++++++++++*/

        /**
         * @brief 添加教育云平台学生行为
         * @param [std::list<TimeSlotInfo_S>&] listTime: 学生行为时间段
         * @param [bool] bBehavior: 是否发生该行为
         * @param [int] nRecordTime: 录制时长
         * @param [int] nBehaviorType: 行为类型
         * @return [*]
         * @note
         */
        BlError_E add_platformStudentBehavior(
            std::list<TimeSlotInfo_S>& listTime,
            bool                       bBehavior,
            int                        nRecordTime,
            int                        nBehaviorType);

        /**
         * @brief 处理教育平台表情信息
         * @param [EmoInfo_S] stInfo：表情信息
         * @return [*]
         * @note
         */
        BlError_E deal_platformEmo(AiManage_NS::EmoInfo_S stInfo);

        /**
         * @brief 处理教育云平台学生行为分析数据
         * @param [BehaviorInfo_S] stInfo：行为信息
         * @return [*]
         * @note
         */
        BlError_E deal_platformStudentBehavior(AiManage_NS::BehaviorInfo_S stInfo);

        /**
         * @brief 处理教育云平台轨迹识别分析数据
         * @param [TrackInfo_S] stInfo：轨迹信息
         * @return [*]
         * @note
         */
        BlError_E deal_platformTrackTeacher(AiManage_NS::TrackInfo_S stInfo);

        /**
         * @brief 教育云平台开始互动行为
         * @param [*] 
         * @return [*]
         * @note
         */
        BlError_E   startPlatformInteraction();

        /**
         * @brief 教育云平台开始巡视行为
         * @param [*] 
         * @return [*]
         * @note
         */
        BlError_E startPlatformTour();

        /**
         * @brief 教育云平台开始教授行为
         * @param [*] 
         * @return [*]
         * @note
         */
        BlError_E startPlatformTaught();

        /**
         * @brief 处理教育云平台教师指导
         * @param [bool] bValue:是否指导
         * @return [*]
         * @note
         */
        BlError_E teacherPlatformDirecting(bool bValue);

        /**
         * @brief 开始处理教育云平台教师信息
         * @param [*] 
         * @return [*]
         * @note
         */
        BlError_E beginDeal_platformTeacherInfo();

        /**
         * @brief 开始处理教育云课堂信息
         * @param [*] 
         * @return [*]
         * @note
         */
        BlError_E beginDeal_platformClassSummaryInfo();

        /**
         * @brief 教育云平台开始处理-学生信息
         * @param [*] 
         * @return [*]
         * @note
         */
        BlError_E beginDeal_PlatformStudentInfo();

/*+++++++++++++++++++++++++++教育云平台Func++++++++++++++++++++++++++++*/
    public:
        AiConfidenceTh_S m_stAiConfidenceTh;

    protected:

        /* 传入参数 */
        InParam_S m_stParamInfo;

        /* 课堂总结信息 */
        std::mutex         m_mtxClassSummary;
        ClassSummaryInfo_S m_stClassSummaryInfo;

        /* 教师信息 */
        std::mutex    m_mtxTeacher;
        TeacherInfo_S m_stTeacherInfo;

        /* 学生信息 */
        std::mutex    m_mtxStudent;
        StudentInfo_S m_stStudentInfo;

        /* 考勤信息 */
        std::mutex       m_mtxAttendance;
        AttendanceInfo_S m_stAttendanceInfo;

        /* 热词提取信息 */
        std::mutex       m_mtxHotwordExtInfo;
        HotwordExtInfo_S m_stHotwordExtInfo;

        /* 当前分析人数 */
        int m_nCurPeopleNum = 0;

        /* 当前是否为学生画面 */
        std::atomic_bool m_bStudentScreen;

        /* 老师下讲台的轨迹样本坐标, 用于学生行为分析处理 */
        AiManage_NS::BoxInfo_S m_stCurSampleBoxInfo;
        AiManage_NS::BoxInfo_S m_stLastSampleBoxInfo;

/*+++++++++++++++++++++++++++教育云平台member++++++++++++++++++++++++++++*/

        /* 教育平台课堂总结信息 */
        std::mutex         m_mtxPlatformClassSummary;
        ClassSummaryInfo_S m_stPlatformClassSummaryInfo;

        /* 教育平台教师信息 */
        std::mutex    m_mtxPlatformTeacher;
        TeacherInfo_S m_stPlatformTeacherInfo;

        /* 教育平台学生信息 */
        std::mutex    m_mtxPlatformStudent;
        StudentInfo_S m_stPlatformStudentInfo;

        /* 教育平台考勤信息 */
        std::mutex       m_mtxPlatformAttendance;
        AttendanceInfo_S m_stPlatformAttendanceInfo;

        /* 当前是否为教育云平台学生画面 */
        std::atomic_bool m_bPlatformStudentScreen;
        
        /* 老师下讲台的轨迹样本坐标, 用于教育云平台学生行为分析处理 */
        AiManage_NS::BoxInfo_S m_stPfCurSampleBoxInfo;
        AiManage_NS::BoxInfo_S m_stPfLastSampleBoxInfo;

        /* 百分比结构体 */
        ResultsRate_S m_stResultsRate;
    
        /* 识别到的总人数 */
        int m_nTotal = 0;
        std::mutex    m_mtxCurrTime;
/*+++++++++++++++++++++++++++教育云平台member++++++++++++++++++++++++++++++*/
    };

}    // namespace ResultsModule_NS
