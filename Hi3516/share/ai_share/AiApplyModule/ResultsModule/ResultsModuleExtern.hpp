/*
 * @FilePath     : ResultsModuleExtern.hpp
 * @Author       : 严泽辉 yanzeh@kfb.cn
 * @Date         : 2024-03-28 09:22:15
 * @LastEditors: weigl 308241951@qq.com
 * @LastEditTime: 2025-02-27 09:24:15
 * @Description  :
 */
#pragma once

#include <iostream>
#include <list>
#include <map>

#include "AiManageExtern.hpp"
#include "BlError.h"

namespace ResultsModule_NS
{

    /* 置信度阈值配置文件 */
    #define CONFIDENCE_TH_INFO_JSON ("/opt/bl/.config/user_data/confidence_threshold_info.json")

    /* 结果处理类型 */
    typedef enum _Type_
    {
        SAVE_JSON = 0, /* 保存成Json数据 */
    } Type_E;

    /* 行为类型枚举 */
    typedef enum _PlatformBehaviorType_
    {
        LISTEN_TO_TALK                   = 1, /* 听讲 */
        PRAXIS                           = 2, /* 实践 */
        DEMONSTRATION                    = 3, /* 演示 */
        READ                             = 4, /* 阅读 */
        DISCUSSION                       = 5, /* 讨论 */
        MOOD_SWING                       = 6, /* 情绪波动 */
        MOOD_CURVE                       = 7, /* 情绪曲线 */
        TEACH_BY_TEACHER                 = 8, /* 教师讲授 */
        INSTRUCT_STUDENTS                = 9, /* 指导学生 */
        WRITING_ON_BOARD                 = 10, /* 书写板书 */
        TEA_STU_INTERACTION              = 11, /* 师生互动 */
        TEACHER_PATROL                   = 12, /* 教师巡视 */
    } PlatformBehaviorType_E;

    /* 预警信息类型枚举 */
    typedef enum _AlertType_
    {
        TEACHER_ABSENCE                  = 1, /* 教师未到达课堂 */
        TEACHER_DELAY                    = 2, /* 教师拖堂 */
        TEACHER_NO_LOOK_OVER             = 3, /* 教师上课未巡视 */
        TEACHER_LEAVE_EARLY              = 4, /* 教师早退 */
        TEACHER_MAKEING_PHONE_CALL       = 5, /* 教师接打电话 */
        STUDENT_ABSENCE                  = 6, /* 学生缺勤 */
        STUDENT_LEAVE_CLASSROOM          = 7, /* 学生中途离课 */
        STUDENT_LOW_CLASS_PARTICIPATION  = 8, /* 学生课堂参与度 */
        STUDENT_POOR_DISCIPLINE          = 9, /* 学生课堂纪律差 */
        STUDENT_GATHER                   = 10, /* 学生聚集 */
        STUDENT_DOWN_DESK                = 11, /* 学生趴桌 */
        STUDENT_PLAYING_PHONE            = 12, /* 学生玩手机 */
    } AlertType_E;

    /* Ai置信度阈值 */
    typedef struct _AiConfidenceTh_
    {
        double fStuPhoneCofid;   /* 学生玩手机 */
        double fTePhoneCofid;    /* 教师接打电话 */
        double fTeBoardCofid;        /* 教师板书 */
        double fStuDclCofid;     /* 学生课堂纪律 */
        double fGatherCofid;     /* 学生聚集 */
        double fRatio;           /* 玩手机系数比（系数比 = 手腕的距离/肩膀的距离） */

        _AiConfidenceTh_()
        {
            fStuPhoneCofid = 0;
            fTePhoneCofid  = 0;
            fStuDclCofid   = 0;
            fGatherCofid   = 0;
            fRatio         = 0;
        }

        void clear()
        {
            fStuPhoneCofid = 0;
            fTePhoneCofid  = 0;
            fStuDclCofid   = 0;
            fGatherCofid   = 0;
            fRatio         = 0;
        }
    } AiConfidenceTh_S;

    /* 人员类型枚举 */
    typedef enum _PersonType_
    {
        TEACHER                  = 1, /* 老师 */
        STUDENT                  = 2, /* 学生 */
    } PersonType_E;

    /* 行为开始结束标志 */
    typedef enum _BehaviorFlag_
    {
        START_BEHAVIOR                  = 1, /* 开始 */
        STOP_BEHAVIOR                   = 2, /* 结束 */
    } BehaviorFlag_E;

    /* 处理类型 */
    typedef enum _NotEmoType_
    {
        NOTEMO = -1, /* 不是表情类型 */
    } NotEmoType_E;

    /* 百分比结构体 */
    typedef struct _ResultsRate_
    {
        double fHeadUpRate;
        double fHeadDownRate;

        _ResultsRate_()
        {
            fHeadUpRate = 0;
            fHeadDownRate = 0;
        }

        void clear()
        {
            fHeadUpRate = 0;
            fHeadDownRate = 0;
        }
    } ResultsRate_S;

    /* 必需参数 */
    typedef struct _NeedParam_
    {
        Type_E enType;                                        /* 使用的类型 */
        AiManage_NS::getRecordStateFunc    getRecordState;    /* 获取录制状态 */
        AiManage_NS::getRecordTimeFunc     getRecordTime;     /* 获取录制时长 */
        AiManage_NS::isTeacherPodiumFunc   isTeacherPodium;   /* 判断老师是否在讲台 */
        AiManage_NS::isStudentCloseUpFunc  isStudentCloseUp;  /* 判断是否在学生特写 */
        AiManage_NS::sendStuPanoSSFunc     sendStuPanoSS;     /* 发送学生全景截图命令 */
        AiManage_NS::sendStuSpecSSFunc     sendStuSpecSS;     /* 发送学生特写截图命令 */
        AiManage_NS::sendStreamGetDataFunc sendStreamGetData; /* 发送stream获取分析数据 */
        AiManage_NS::isLocalModeFunc       isLocalMode;       /* 判断当前是否为本地分析 */
        AiManage_NS::platformTeaEventFunc  platformTeaEvent;  /* 处理上报教师预警事件 */
        AiManage_NS::platformBehaviorEventFunc platformBehaviorEvent; /* 处理上报是否开始结束行为函数 */
        AiManage_NS::platformAtStuAlertFunc platformAtStuAlert; /* 处理上报学生考勤预警信息给教育云平台函数指针 */
        AiManage_NS::platformSdStuAlertFunc platformSdStuAlert; /* 处理上报学生趴桌预警信息给教育云平台函数指针 */
        AiManage_NS::platformPushAlertTypeFunc platformPushAlertType;  /* 上报教育云平台预警信息函数指针 */
        AiManage_NS::isPlatformClassTimeFunc isPlatformClassTime; /* 是否是教育云平台上课时间函数指针 */
        AiManage_NS::platformPushStatusFunc platformPushStatus;   /* 上报教育云平台缺勤人数和录制时长函数指针 */
        AiManage_NS::getClassTimeFunc      getClassTime;      /* 获取教育云平台已经上课了多少时间函数指针 */
        AiManage_NS::pfPushEmoticonTypeFunc pfPushEmoticonType; /* 上报学生表情给教育云平台函数指针 */
        AiManage_NS::isPlatformAiTimeFunc  isPlatformAiTime;  /* 是否是教育云平台AI分析时间函数指针 */
        AiManage_NS::getPlatformSwitchFunc getPlatformSwitch; /* 获取教育云平台预警开关状态函数指针 */
        
        void clear()
        {
            enType           = SAVE_JSON;
            getRecordState   = nullptr;
            getRecordTime    = nullptr;
            isTeacherPodium  = nullptr;
            isStudentCloseUp = nullptr;
            platformTeaEvent = nullptr;
            platformBehaviorEvent = nullptr;
            platformAtStuAlert = nullptr;
            platformSdStuAlert = nullptr;
            platformPushAlertType = nullptr;
            isPlatformClassTime = nullptr;
            platformPushStatus = nullptr;
            getClassTime = nullptr;
            pfPushEmoticonType = nullptr;
            isPlatformAiTime = nullptr;
            getPlatformSwitch=nullptr;
        }

        _NeedParam_()
        {
            clear();
        }
    } NeedParam_S;

    /* 额外参数 */
    typedef struct _ExParam_
    {
        bool Reserved; /* 预留位，没用 */

        _ExParam_()
        {
            Reserved = false;
        }

        void clear()
        {
            Reserved = false;
        }

    } ExParam_S;

    /* 参数结构体 */
    typedef struct _InParam_
    {
        NeedParam_S stNeedParam; /* 必需参数 */
        ExParam_S   stExParam;   /* 额外参数 */

        void clear()
        {
            stNeedParam.clear();
            stExParam.clear();
        }
    } InParam_S;

    /* 坐标 */
    typedef struct _CoordInfo_
    {
        int nX; /* x坐标 */
        int nY; /* y坐标 */

        void clear()
        {
            nX = 0;
            nY = 0;
        }
    } CoordInfo_S;

    /* 时间段 */
    typedef struct _TimeSlotInfo_
    {
        int nStart;     /* 开始 */
        int nEnd;       /* 结束 */
        int nUserParam; /* 自定义属性 */

        void clear()
        {
            nStart     = 0;
            nEnd       = 0;
            nUserParam = 0;
        }

        void print()
        {
            std::cout << "开始:" << nStart << std::endl;
            std::cout << "结束:" << nEnd << std::endl;
            std::cout << "自定义属性:" << nUserParam << std::endl;
            std::cout << "==========" << std::endl;
        }
    } TimeSlotInfo_S;

    /* 考勤学生信息 */
    typedef struct _StAttendanceInfo_
    {
        int            nId;            /* 学生ID */
        std::string    strName;        /* 学生名字 */
        long long      nFirstTime;     /* 第一次出现的时间戳 */
        long long      nLastTime;      /* 最后一次出现的时间戳 */
        long long      nNumber;        /* 识别到的次数 */
        std::list<int> listAnswerTime; /* 回答问题时刻时间点数组 */

        void clear()
        {
            nId = 0;
            strName.clear();
            nFirstTime = 0;
            nLastTime  = 0;
            nNumber    = 0;
            listAnswerTime.clear();
        }
    } StAttendanceInfo_S;

    /* 考勤教师信息 */
    typedef struct _TeAttendanceInfo_
    {
        int         nId;        /* 教师ID */
        std::string strName;    /* 教师名字 */
        long long   nFirstTime; /* 第一次出现的时间戳 */
        long long   nLastTime;  /* 最后一次出现的时间戳 */
        long long   nNumber;    /* 识别到的次数 */

        void clear()
        {
            nId = 0;
            strName.clear();
            nFirstTime = 0;
            nLastTime  = 0;
            nNumber    = 0;
        }
    } TeAttendanceInfo_S;

    /* 考勤信息 */
    typedef struct _AttendanceInfo_
    {
        std::map<int, StAttendanceInfo_S> mapStudentInfo; /* 学生考勤 1*/
        std::map<int, TeAttendanceInfo_S> mapTeacherInfo; /* 教师考勤 1*/

        void clear()
        {
            mapStudentInfo.clear();
            mapTeacherInfo.clear();
        }
    } AttendanceInfo_S;

    /* 学生表情信息 */
    typedef struct _StudentEmoInfo_
    {
        int nInterval;      /* 表情数据的间隔， 单位/s 1*/
        int nLastCountTime; /* 最后一次统计表情时间 1*/

        /* 用于定时计算 */
        int nAngerTotal;    /* 愤怒的总次数 1*/
        int nDisgustTotal;  /* 厌恶的总次数 1*/
        int nFearTotal;     /* 恐惧的总次数 1*/
        int nJoyTotal;      /* 快乐的总次数 1*/
        int nNeutralTotal;  /* 中性的总次数 1*/
        int nSadnessTotal;  /* 悲伤的总次数 1*/
        int nSurpriseTotal; /* 惊喜的总次数 1*/


        std::list<AiManage_NS::Emotion_E> listEmotion; /* 表情链表 1*/

        void clear()
        {
            nInterval      = 60;
            nLastCountTime = 0;

            nAngerTotal    = 0;
            nDisgustTotal  = 0;
            nFearTotal     = 0;
            nJoyTotal      = 0;
            nNeutralTotal  = 0;
            nSadnessTotal  = 0;
            nSurpriseTotal = 0;

            listEmotion.clear();
        }

    } StudentEmoInfo_S;

    /* 学生行为信息 */
    typedef struct _StudentBehaviorInfo_
    {
        std::list<TimeSlotInfo_S> listListenTime;      /* 听讲时间段，抬头 */
        std::list<TimeSlotInfo_S> listPracticeTime;    /* 实践时间段，举手 */
        std::list<TimeSlotInfo_S> listDemonstrateTime; /* 演示时间段，站立 */
        std::list<TimeSlotInfo_S> listReadTime;        /* 阅读时间段，低头 */
        std::list<TimeSlotInfo_S> listDiscussTime;     /* 讨论时间段，转头，转身 */
        std::list<TimeSlotInfo_S> listDownDeskTime;    /* 趴桌时间段，趴桌 */
        std::list<TimeSlotInfo_S> listPlayPhoneTime;   /* 玩手机时间段，玩手机 */

        void clear()
        {
            listListenTime.clear();
            listPracticeTime.clear();
            listDemonstrateTime.clear();
            listReadTime.clear();
            listDiscussTime.clear();
            listDownDeskTime.clear();
            listPlayPhoneTime.clear();
        }

    } StudentBehaviorInfo_S;

    /* 专注度信息 */
    typedef struct _FocusScoreInfo_
    {
        int nAverageScore;  /* 专注度均值 1*/
        int nInterval;      /* 表情数据的间隔， 单位/s 1*/
        int nLastCountTime; /* 最后一次统计表情时间 1*/

        /* 用于定时计算 */
        int nListenTotal;      /* 听讲的总次数，抬头 */
        int nPracticeTotal;    /* 实践的总次数，举手 */
        int nDemonstrateTotal; /* 演示的总次数，站立 */
        int nReadTotal;        /* 阅读的总次数，低头 */
        int nDiscussTotal;     /* 讨论的总次数，转头 */
        int nDownDeskTotal;    /* 趴桌的总次数，趴桌 */
        int nPlayPhoneTotal;    /* 玩手机的总次数，玩手机 */


        std::list<int> listScore; /* 专注度得分链表 */

        void clear()
        {
            nAverageScore  = 0;
            nInterval      = 60;
            nLastCountTime = 0;

            nListenTotal      = 0;
            nPracticeTotal    = 0;
            nDemonstrateTotal = 0;
            nReadTotal        = 0;
            nDiscussTotal     = 0;
            nDownDeskTotal    = 0;

            listScore.clear();
        }

    } FocusScoreInfo_S;

    /* 学生信息 */
    typedef struct _StudentInfo_
    {
        /* 学生总结信息 */
        int                nFrameNum;           /* 一共分析了多少帧人数数据 1*/
        int                nAverageHumanCount;  /* 平均人数，出席人数 1*/
        std::map<int, int> mapHumanCount;       /* 映射 <人数，出现次数> */
        int                nListenTime;         /* 学生听讲时长，单位/s 1*/
        int                nPracticeTime;       /* 学生实践（练习）时长，单位/s 1*/
        int                nDemonstrateTime;    /* 学生演示时长，单位/s 1*/
        int                nReadTime;           /* 学生阅读时长，单位/s 1*/
        int                nDiscussTime;        /* 学生讨论时长，单位/s 1*/
        int                nStudyTime;          /* 学生学习总时长 实践+讨论，单位/s 1*/
        int                nConcentrationPct;   /* 注意力集中占比，单位% 1*/
        int                nDistractionPct;     /* 注意力涣散占比，单位% 1*/

        int nConcentrationStageStart;           /* 注意力集中阶段-开始 1*/
        int nConcentrationStageEnd;             /* 注意力集中阶段-结束 1*/
        int nConcentrationStageAverageScore;    /* 注意力集中阶段-专注度均值 1*/

        int nDistractionStageStart;             /* 注意力涣散阶段-开始 1*/
        int nDistractionStageEnd;               /* 注意力涣散阶段-结束 1*/
        int nDistractionStageAverageScore;      /* 注意力涣散阶段，专注度均值 1*/

        int nExcitementStageStart;              /* 学生情绪较兴奋阶段-开始时间点，单位/s 1*/
        int nExcitementStageEnd;                /* 学生情绪较兴奋阶段-结束时间点，单位/s 1*/

        int nLowStageStart;                     /* 学生情绪较低落阶段-开始时间点，单位/s 1*/
        int nLowStageEnd;                       /* 学生情绪较低落阶段-结束时间点，单位/s 1*/

        int nAngerTotal;                        /* 愤怒的总次数 1*/
        int nDisgustTotal;                      /* 厌恶的总次数 1*/
        int nFearTotal;                         /* 恐惧的总次数 1*/
        int nJoyTotal;                          /* 快乐的总次数 1*/
        int nNeutralTotal;                      /* 中性的总次数 1*/
        int nSadnessTotal;                      /* 悲伤的总次数 1*/
        int nSurpriseTotal;                     /* 惊喜的总次数 1*/

        int nListenTotal;                       /* 听讲的总次数，抬头 */
        int nPracticeTotal;                     /* 实践的总次数，举手 */
        int nDemonstrateTotal;                  /* 演示的总次数，站立 */
        int nReadTotal;                         /* 阅读的总次数，低头 */
        int nDiscussTotal;                      /* 讨论的总次数，转头 */
        int nDownDeskTotal;                     /* 趴桌的总次数，趴桌 */
        int nPlayPhoneTotal;                    /* 玩手机的总次数，玩手机 */

        StudentEmoInfo_S      stEmoInfo;        /* 表情信息 1*/
        StudentBehaviorInfo_S stBehaviorInfo;   /* 学生行为时间段信息 1*/
        FocusScoreInfo_S      stFocusScoreInfo; /* 专注度信息 1*/

        void clear()
        {
            nFrameNum          = 0;
            nAverageHumanCount = 0;
            mapHumanCount.clear();
            nListenTime       = 0;
            nPracticeTime     = 0;
            nDemonstrateTime  = 0;
            nReadTime         = 0;
            nDiscussTime      = 0;
            nStudyTime        = 0;
            nConcentrationPct = 0;
            nDistractionPct   = 0;

            nConcentrationStageStart        = 0;
            nConcentrationStageEnd          = 0;
            nConcentrationStageAverageScore = 0;

            nDistractionStageStart        = 0;
            nDistractionStageEnd          = 0;
            nDistractionStageAverageScore = 0;

            nExcitementStageStart = 0;
            nExcitementStageEnd   = 0;

            nLowStageStart = 0;
            nLowStageEnd   = 0;

            nAngerTotal    = 0;
            nDisgustTotal  = 0;
            nFearTotal     = 0;
            nJoyTotal      = 0;
            nNeutralTotal  = 0;
            nSadnessTotal  = 0;
            nSurpriseTotal = 0;

            nListenTotal      = 0;
            nPracticeTotal    = 0;
            nDemonstrateTotal = 0;
            nReadTotal        = 0;
            nDiscussTotal     = 0;
            nDownDeskTotal    = 0;

            stEmoInfo.clear();
            stBehaviorInfo.clear();
            stFocusScoreInfo.clear();
        }


    } StudentInfo_S;

    /* 教师行为信息 */
    typedef struct _TeacherBehaviorInfo_
    {

        std::list<TimeSlotInfo_S> listTaughtTime;      /* 教师讲授时间段-1 1*/
        std::list<TimeSlotInfo_S> listDirectingTime;   /* 指导学生时间段-x 1*/
        std::list<TimeSlotInfo_S> listBoardTime;       /* 书写板书时间段-x 1*/
        std::list<TimeSlotInfo_S> listInteractionTime; /* 师生互动时间段-3 1*/
        std::list<TimeSlotInfo_S> listTourTime;        /* 教室巡视时间段-2 1*/

        void clear()
        {
            listTaughtTime.clear();
            listDirectingTime.clear();
            listBoardTime.clear();
            listInteractionTime.clear();
            listTourTime.clear();
        }

    } TeacherBehaviorInfo_S;

    /* 教师信息 */
    typedef struct _TeacherInfo_
    {
        int nTaughtTime;                       /* 讲授时长，单位/s 1*/
        int nInteractionTime;                  /* 互动时长，单位/s 1*/
        int nDirectingTime;                    /* 指导时长，单位/s 1*/
        int nBoardTime;                        /* 板书时长，单位/s 1*/
        int nPodiumTime;                       /* 讲台时长 = 讲授时长，单位/s 1*/
        int nTourTime;                         /* 巡视时长，单位/s 1*/
        int nTourNumber;                       /* 巡视次数 1*/

        TeacherBehaviorInfo_S  stBehaviorInfo; /* 教师行为信息 1*/
        std::list<CoordInfo_S> listTrackInfo;  /* 教师轨迹信息 1*/

        void clear()
        {
            nTaughtTime      = 0;
            nInteractionTime = 0;
            nDirectingTime   = 0;
            nBoardTime       = 0;
            nPodiumTime      = 0;
            nTourTime        = 0;
            nTourNumber      = 0;

            stBehaviorInfo.clear();
            listTrackInfo.clear();
        }

    } TeacherInfo_S;

    /* 课堂总结 */
    typedef struct _ClassSummaryInfo_
    {
        int nClassTime;                                /* 课堂时长/s */
        int nClassScore;                               /* 课堂得分 = 专注度均值 1*/

        std::string strAiClassAdviceSumUp;             /* 课堂教学分析报告、单堂报告下载,AI课堂建议 1*/
        std::string strClassConclusionSumUp;           /* 课堂对比分析报告、对比分析报告下载,课堂类型分析结论 1*/
        std::string strClassEvaluateSumUp;             /* 教师教学报告（单节课）,课堂总体评价 1*/
        std::string strFocusScoreSumUp;                /* 教师教学报告（单节课）重点学情回顾 学生专注度 1*/
        std::string strEmoSumUp;                       /* 教师教学报告（单节课）重点学情回顾 学生情绪指标 1*/

        int nTeacherTime;                              /* 老师授课时间，百分数 1*/
        int nStudentTime;                              /* 学生活动时间，百分数 1*/
        int nTaughtPct;                                /* 教师讲授时间占比，单位% 1*/
        int nInteractionPct;                           /* 师生互动时间占比，单位% 1*/
        int nStudyPct;                                 /* 学生学习时间占比，单位% 1*/
        int nParticipationRate;                        /* 参与率 1*/
        int nHeadUpRate;                               /* 抬头率 1*/
        int nHeadDownRate;                             /* 低头率 1*/

        float fRtChX;                                  /* RT-CH曲线-x 1*/
        float fRtChY;                                  /* RT-CH曲线-y 1*/
        int   nStTeCutNum;                             /* 学生行为转老师行为次数 */

        int                nPPTInterval;               /* PPT翻页间隔，单位/s 1*/
        std::list<int64_t> listPPTSwitchTime;          /* PPT翻页时间戳链表 1*/

        int                    nStInterval;            /* ST分析间隔/s 1*/
        std::list<CoordInfo_S> listSTInfo;             /* ST曲线 1*/

        std::list<TimeSlotInfo_S> listQuizTime;        /* 提问时间点 = 师生互动时间段 */
        std::list<TimeSlotInfo_S> listInteractionTime; /* 互动时间点 = 指导学生时间段 */

        void clear()
        {
            nClassScore = 0;

            strAiClassAdviceSumUp.clear(); /* 清空字符串 */
            strClassConclusionSumUp.clear();
            strClassEvaluateSumUp.clear();
            strFocusScoreSumUp.clear();
            strEmoSumUp.clear();

            nTeacherTime       = 0;
            nStudentTime       = 0;
            nTaughtPct         = 0;
            nInteractionPct    = 0;
            nStudyPct          = 0;
            nParticipationRate = 0;
            nHeadUpRate        = 0;
            nHeadDownRate      = 0;

            fRtChX      = 0.0;
            fRtChY      = 0.0;
            nStTeCutNum = 0;

            nPPTInterval = 0;
            listPPTSwitchTime.clear();

            nStInterval = 10;
            listSTInfo.clear();          /* 清空listSTInfo中的所有元素 */

            listQuizTime.clear();        /* 清空listQuizTime中的所有元素 */
            listInteractionTime.clear(); /* 清空listInteractionTime中的所有元素 */
        }

    } ClassSummaryInfo_S;

    /* 词语 */
    typedef struct _WordInfo_
    {
        std::string strWord;        /* 词语 */
        int nCount;                 /* 词语的使用次数 */

        void clear()
        {
            strWord.clear();
            nCount = 0;
        }
    }WordInfo_S;
    
    /* 热词提取 */
    typedef struct _HotwordExtInfo_
    {
        std::list<WordInfo_S> listWordInfo;

        void clear()
        {
            listWordInfo.clear();
        }
    }HotwordExtInfo_S;

}    // namespace ResultsModule_NS
