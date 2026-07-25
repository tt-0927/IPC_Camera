/*
 * @FilePath     : AiManage.hpp
 * @Author       : 严泽辉 yanzeh@kfb.cn
 * @Date         : 2024-03-25 10:27:43
 * @LastEditors: weigl 308241951@qq.com
 * @LastEditTime: 2025-03-10 14:12:29
 * @Description  :
 */
#pragma once

#include <atomic>

#include "AiManageExtern.hpp"
#include "AiPlatformFactory.hpp"
#include "CommModule.hpp"
#include "ParseData.hpp"
#include "ResultsModule.hpp"
#include "sem_timer.h"
#include "time_keep.h"

/* 不使用sem_timer.h可以注释 */
#define USE_SEM_TIMER 1

#ifdef USE_SEM_TIMER
    #include "sem_timer.h"
#endif

#ifdef USE_LOCAL_AI
    #include "AiLocal.hpp"
#endif

namespace AiManage_NS
{
/* AI服务器限制连接数量 */
#define DEV_LIMIT_NUM (3)

    class CAiManage
    {
    public:

        /*单例对外接口*/
        static CAiManage& get_instance()
        {
            static CAiManage* ms_pInstance = nullptr;
            static std::mutex mtx;
            if (ms_pInstance == nullptr)
            {
                std::lock_guard<std::mutex> lock(mtx);
                if (ms_pInstance == nullptr)
                {
                    ms_pInstance = new CAiManage();
                }
            }
            return *ms_pInstance;
        }

        /*禁止拷贝构造函数和赋值运算符*/
        CAiManage(const CAiManage&)            = delete;
        CAiManage& operator=(const CAiManage&) = delete;

    public:
        
        BlError_E Enable_local_analysis();

        BlError_E Disable_local_analysis();

        /**
         * @brief 设置AI管理模块的参数
         * @param [Param_S] stParam: 需要设置的参数
         * @return [*] >= BlError_E::OK 成功  其他失败
         * @note
         */
        BlError_E set_params(Param_S stParam);

        /**
         * @brief 更新AI管理模块的参数
         * @param [Param_S] stParam: 需要设置的参数
         * @return [*] >= BlError_E::OK 成功  其他失败
         * @note
         */
        BlError_E update_params(Param_S stParam);

        /**
         * @brief 获取AI管理模块的参数
         * @param [Param_S] stParam: 需要设置的参数
         * @return [*] >= BlError_E::OK 成功  其他失败
         * @note
         */
        BlError_E get_params(Param_S& stParam);

        /**
         * @brief 获取班级数据
         * @return [*] >= BlError_E::OK 成功  其他失败
         * @note
         */
        BlError_E get_classInfo();

        /**
         * @brief 设置学生人数统计的参数
         * @param [AlgorithmParam_S] stParam: 需要设置的参数
         * @return [*] >= BlError_E::OK 成功  其他失败
         * @note
         */
        BlError_E set_countStudentsParam(AlgorithmParam_S stParam);

        /**
         * @brief 设置教师接打电话识别的参数
         * @param [AlgorithmParam_S] stParam: 需要设置的参数
         * @return [*] >= BlError_E::OK 成功  其他失败
         * @note
         */
        BlError_E set_teTelephoneCallsParam(AlgorithmParam_S stParam);

        /**
         * @brief 设置学生玩手机识别的参数
         * @param [AlgorithmParam_S] stParam: 需要设置的参数
         * @return [*] >= BlError_E::OK 成功  其他失败
         * @note
         */
        BlError_E set_stPlayPhoneParam(AlgorithmParam_S stParam);

        /**
         * @brief 设置学生课堂纪律的参数
         * @param [AlgorithmParam_S] stParam: 需要设置的参数
         * @return [*] >= BlError_E::OK 成功  其他失败
         * @note
         */
        BlError_E set_stDisciplineParam(AlgorithmParam_S stParam);

        /**
         * @brief 设置学生行为分析的参数
         * @param [AlgorithmParam_S] stParam: 需要设置的参数
         * @return [*] >= BlError_E::OK 成功  其他失败
         * @note
         */
        BlError_E set_studentBehaviorParam(AlgorithmParam_S stParam);
        BlError_E set_studentBehaviorExParam(AiScenario_NS::BehaviorParam_S stExParam);

        /**
         * @brief 设置表情识别的参数
         * @param [AlgorithmParam_S] stParam: 需要设置的参数
         * @return [*] >= BlError_E::OK 成功  其他失败
         * @note
         */
        BlError_E set_emoDetecrParam(AlgorithmParam_S stParam);

        /**
         * @brief 设置教师人脸识别的参数
         * @param [AlgorithmParam_S] stParam: 需要设置的参数
         * @return [*] >= BlError_E::OK 成功  其他失败
         * @note
         */
        BlError_E set_teFaceDetecrParam(AlgorithmParam_S stParam);

        /**
         * @brief 设置学生人脸识别的参数
         * @param [AlgorithmParam_S] stParam: 需要设置的参数
         * @return [*] >= BlError_E::OK 成功  其他失败
         * @note
         */
        BlError_E set_stFaceDetecrParam(AlgorithmParam_S stParam);

        /**
         * @brief 设置老师轨迹检测的参数
         * @param [AlgorithmParam_S] stParam: 需要设置的参数
         * @return [*] >= BlError_E::OK 成功  其他失败
         * @note
         */
        BlError_E set_trackTeacherParam(AlgorithmParam_S stParam);

        /**
         * @brief 设置板书检测的参数
         * @param [AlgorithmParam_S] stParam: 需要设置的参数
         * @return [*] >= BlError_E::OK 成功  其他失败
         * @note
         */
        BlError_E set_boardDetecrParam(AlgorithmParam_S stParam);

        /**
         * @brief 结束AI分析
         * @param [void*] pParam: 不定参数，不同子类可能需要不同的参数类型
         * @return [*]
         * @note
         */
        BlError_E end_aiAnalysis(const void* pParam);

        /**
         * @brief 结束教育云平台AI分析
         * @param [void*] pParam: 不定参数，不同子类可能需要不同的参数类型
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
         * @brief 教师板书
         * @param [bool] bValue: true-板书 false-不在板书
         * @return [*]
         * @note
         */
        BlError_E teacherBoard(bool bValue);

        /**
         * @brief PPT切换
         * @return [*]
         * @note
         */
        BlError_E pptSwitch();

        /**
         * @brief 添加提取的热词
         * @param std::list<std::pair<std::string, int>> listWords：热词信息
         * @return [*]
         */
        BlError_E addAudioWordsResult(std::list<std::pair<std::string, int>> listWords);

        /**
         * @brief 设置本地老师人脸识别-结果
         * @param [std::list<std::pair<int, std::string>>] listName: 老师信息
         * @return [*]
         */
        BlError_E setLocalTeFaceRecResult(std::list<std::pair<int, std::string>> listName);

        /**
         * @brief 设置AI处理状态
         * @param [int] nStatus: 0-开始 1-暂停 2-恢复 3-结束
         * @return [*]
         * @note
         */
        BlError_E setAiProcessingStatus(int nStatus);

        /**
         * @brief 处理人数统计分析数据
         * @param [NumberInfo_S] stInfo: 分析后的数据
         * @return [*]
         * @note
         */
        BlError_E deal_countStudents(char* pchJson);

        /**
         * @brief 处理课堂纪律分析数据
         * @param [NumberInfo_S] stInfo: 分析后的数据
         * @return [*]
         * @note
         */
        BlError_E deal_discipline(char* pchJson);

        /**
         * @brief 处理学生行为分析数据
         * @param [BehaviorInfo_S] stInfo: 分析后的数据
         * @return [*]
         * @note
         */
        BlError_E deal_studentBehavior(char* pchJson);

        /**
         * @brief 设置平台IP地址
         * @param [string] strIp:
         * @return [*]
         * @note
         */
        BlError_E set_platformIp(std::string strIp);

        /**
         * @brief 获取AI服务器连接状态
         * @return [*] 1-连接 0-未连接
         * @note
         */
        int get_aiServerStatus();

        /**
         * @brief 更新stream获取回来的数据
         * @param [char*] pchData: 数据
         * @param [int] nLength: 数据长度
         * @param [int] nCode: 通信码
         * @return [*] 1-连接 0-未连接
         * @note
         */
        int updateStreamData(char* pchData, int nLength, int nCode);

        /**
         * @brief 发送AI通讯数据-分析数据
         * @param [int] nCode: 命令码
         * @return [*]
         * @note
         */
        BlError_E sendAi_analyseData(char* pchData, int nDataLen, int nCode, ProcessMode_E enMode);

        /* 使能语音转写-为了关闭某些功能，规避性能不够的问题 */
        BlError_E set_transcription(bool bEnabled);

    private:

        CAiManage();
        ~CAiManage();

        /**
         * @brief 初始化平台管理
         * @return [*]
         * @note
         */
        BlError_E initPlatForm();

        /**
         * @brief 初始化与AI服务器的连接
         * @return [*]
         * @note
         */
        BlError_E initComm_aiServer();

        /**
         * @brief 初始化本机服务器
         * @return [*]
         * @note
         */
        BlError_E initComm_localServer();

        /**
         * @brief 初始化处理结果类
         * @return [*]
         * @note
         */
        BlError_E init_results();

        /**
         * @brief 初始化解析类
         * @return [*]
         * @note
         */
        BlError_E init_parse();

        /**
         * @brief 班级信息回调函数
         * @param [PlatformManage_NS::DevInfo_S] stDevInfo: 获取设备信息
         * @param [BlError_E] enRetCode: 返回值
         * @param [void*] pHandle: 用户参数
         * @return [*]
         * @note
         */
        BlError_E classInfoCallback(PlatformManage_NS::DataInfo_S stDataInfo, BlError_E enRetCode, void* pHandle);

        /**
         * @brief 初始化解析类
         * @param [DevInfo_S&] stDevInfo: 获取设备信息
         * @return [*]
         * @note
         */
        BlError_E get_device_info(DevInfo_S& stDevInfo);



        BlError_E sendAi_Data(std::string strMsg, int nCode);




#ifdef USE_SEM_TIMER
        /**
         * @brief 设置定时器/计时器参数
         * @param [Timer_S**] pTimer: 定时器句柄
         * @param [TimeKeep_S**] pTimeKeep: 计时器句柄
         * @param [AlgorithmParam_S] stParam: 参数
         * @param [int (*)(void* pParam)] pUserFun: 回调函数
         * @return [*]
         * @note
         */
        BlError_E set_timerParam(Timer_S**        pTimer,
                                 TimeKeep_S**     pTimeKeep,
                                 AlgorithmParam_S stParam,
                                 int (*pUserFun)(void* pParam));
#else
        /**
         * @brief 设置定时器/计时器参数
         * @param [TimeKeep_S**] pTimeKeep: 计时器句柄
         * @param [AlgorithmParam_S] stParam: 参数
         * @param [int (*)(void* pParam)] pUserFun: 回调函数
         * @return [*]
         * @note
         */
        BlError_E set_timerParam(TimeKeep_S**     pTimeKeep,
                                 AlgorithmParam_S stParam,
                                 int (*pUserFun)(void* pParam));
#endif

        /**
         * @brief 发送stream数据给Ai
         * @param [const char*] pchMsg: 数据
         * @param [int] nMsgLength: 数据长度
         * @param [int] nCode: 通信码
         * @return [*]
         * @note
         */
        BlError_E send_streamData(const char* pchMsg, int nMsgLength, int nCode);

        /**
         * @brief 关闭人脸数据更新定时器
         * @return [*]
         * @note
         */
        static void close_updateFaceTimer(void* pParam);

        /**
         * @brief 获取文件后缀
         * @return [*]
         * @note
         */
        std::string getFileExtension(const std::string& strFilePath);

    private:

        /* 回调函数 */

        /**
         * @brief 与aiServer连接状态的数据回调函数
         * @param [StatusParam_S] stInfo: 数据参数
         * @return [*] >= BlError_E::OK 成功  其他失败
         * @note
         */
        BlError_E aiServer_statusCallback(COMM_NS::StatusParam_S stInfo);

        /**
         * @brief 与aiServer连接状态的心跳
         * @param [char*] pchMsg: 心跳数据
         * @return [*] >= BlError_E::OK 成功  其他失败
         * @note
         */
        BlError_E deal_aiServer_heartData(char* pchMsg);

        /**
         * @brief 与aiServer连接的数据回调函数
         * @param [DataParam_S] stInfo: 数据参数
         * @return [*] >= BlError_E::OK 成功  其他失败
         * @note
         */
        BlError_E aiServer_dataCallback(COMM_NS::DataParam_S stInfo);

        /**
         * @brief 与本地连接的数据回调函数
         * @param [DataParam_S] stInfo: 数据参数
         * @return [*] >= BlError_E::OK 成功  其他失败
         * @note
         */
        BlError_E localServer_dataCallback(COMM_NS::DataParam_S stInfo);

        /**
         * @brief 学生人数统计定时回调函数
         * @param [void*] pParam: 用户自定义参数
         * @return [*]
         * @note
         */
        static int countStudents_taskFun(void* pParam);

        /**
         * @brief 教师接打电话定时回调函数
         * @param [void*] pParam: 用户自定义参数
         * @return [*]
         * @note
         */
        static int teTelephoneCalls_taskFun(void* pParam);

        /**
         * @brief 学生玩手机定时回调函数
         * @param [void*] pParam: 用户自定义参数
         * @return [*]
         * @note
         */
        static int stPlayPhone_taskFun(void* pParam);

        /**
         * @brief 学生课堂纪律定时回调函数
         * @param [void*] pParam: 用户自定义参数
         * @return [*]
         * @note
         */
        static int stDiscipline_taskFun(void* pParam);

        /**
         * @brief 学生行为分析定时回调函数
         * @param [void*] pParam: 用户自定义参数
         * @return [*]
         * @note
         */
        static int studentBehavior_taskFun(void* pParam);

        /**
         * @brief 表情识别定时回调函数
         * @param [void*] pParam: 用户自定义参数
         * @return [*]
         * @note
         */
        static int emoDetecr_taskFun(void* pParam);

        /**
         * @brief 教师人脸识别定时回调函数
         * @param [void*] pParam: 用户自定义参数
         * @return [*]
         * @note
         */
        static int teFaceDetecr_taskFun(void* pParam);

        /**
         * @brief 学生人脸识别定时回调函数
         * @param [void*] pParam: 用户自定义参数
         * @return [*]
         * @note
         */
        static int stFaceDetecr_taskFun(void* pParam);

        /**
         * @brief 老师轨迹识别定时回调函数
         * @param [void*] pParam: 用户自定义参数
         * @return [*]
         * @note
         */
        static int trackTeacher_taskFun(void* pParam);

        /**
         * @brief 板书识别定时回调函数
         * @param [void*] pParam: 用户自定义参数
         * @return [*]
         * @note
         */
        static int boardDetecr_taskFun(void* pParam);

        /**
         * @brief 人脸数据更新定时回调函数
         * @param [void*] pParam: 用户自定义参数
         * @return [*]
         * @note
         */
        static int updateFace_taskFun(void* pParam);

        /**
         * @brief 1s定时回调函数
         * @param [void*] pParam: 用户自定义参数
         * @return [*]
         * @note
         */
        static int timer1s_taskFun(void* pParam);

        /**
         * @brief 获取当前录制时长
         * @return [*] 当前录制时长
         * @note
         */
        int getRecordTimeFunc();

        /**
         * @brief 获取是否在学生特写
         * @return [*]
         * @note
         */
        bool getStudentCloseUp();

        /**
         * @brief 获取是否老师上下讲台
         * @return [*]
         * @note
         */
        bool getTeacherPodium();

        /**
         * @brief 判断是否为本地分析模式
         * @return [*]
         * @note
         */
        bool isLocalMode();

        /**
         * @brief 获取本机mac地址
         * @param [char] *pMac: mac地址
         * @param [int] nLen: mac地址长度
         * @return [*] 获取到的mac地址
         * @note
         */
        int get_mac(char* pMac, int nLen);


    private:

        Param_S m_stParam;

        /* 通讯类 */
        COMM_NS::CCommBase* m_pAiServerComm    = nullptr;
        COMM_NS::CCommBase* m_pLocalServerComm = nullptr;

        /* 结果处理类 */
        ResultsModule_NS::CResultsBase* m_pResultsBase = nullptr;

        /* 解析处理类 */
        ParseData_NS::CParseBase* m_pParseBase = nullptr;

        /* 平台管理类 */
        PlatformManage_NS::CAiPlatformBase* m_pPlatformManage = nullptr;

#ifdef USE_SEM_TIMER
        /* 定时器 */
        Timer_S* m_pCountStudentsTimer    = NULL; /* 学生人数统计定时器 */
        Timer_S* m_pStudentBehaviorTimer  = NULL; /* 学生行为分析定时器 */
        Timer_S* m_pEmoDetecrTimer        = NULL; /* 表情识别定时器 */
        Timer_S* m_pTeFaceDetecrTimer     = NULL; /* 教师人脸识别定时器 */
        Timer_S* m_pTrackTeacherTimer     = NULL; /* 老师轨迹定时器 */
        Timer_S* m_pBoardDetecrTimer      = NULL; /* 板书识别定时器 */
        Timer_S* m_pTeTelephoneCallsTimer = NULL; /* 教师接打电话识别定时器 */
        Timer_S* m_pStPlayPhoneTimer      = NULL; /* 学生玩手机识别定时器 */
        Timer_S* m_pStDisciplineTimer     = NULL; /* 学生课堂纪律定时器 */
#endif

        /* 计时器 */
        TimeKeep_S* m_pCountStudentsTimeKeep    = NULL; /* 学生人数统计计时器 */
        TimeKeep_S* m_pStudentBehaviorTimeKeep  = NULL; /* 学生行为分析计时器 */
        TimeKeep_S* m_pEmoDetecrTimeKeep        = NULL; /* 表情识别计时器 */
        TimeKeep_S* m_pStFaceDetecrTimeKeep     = NULL; /* 学生人脸识别计时器 */
        TimeKeep_S* m_pStFaceDetecrTimeKeep5s   = NULL; /* 学生人脸识别计时器-5s */
        TimeKeep_S* m_pTeFaceDetecrTimeKeep     = NULL; /* 教师人脸识别计时器 */
        TimeKeep_S* m_pTeFaceDetecrTimeKeep5s   = NULL; /* 教师人脸识别计时器-5s */
        TimeKeep_S* m_pTrackTeacherTimeKeep     = NULL; /* 老师轨迹计时器 */
        TimeKeep_S* m_pBoardDetecrTimeKeep      = NULL; /* 板书识别计时器 */
        TimeKeep_S* m_pTeTelephoneCallsTimeKeep = NULL; /* 教师接打电话识别计时器 */
        TimeKeep_S* m_pStuPlayPhoneTimeKeep     = NULL; /* 学生玩手机识别计时器 */
        TimeKeep_S* m_pStuDisciplineTimeKeep    = NULL; /* 学生课堂纪律计时器 */


        TimeKeep_S* m_pUpdateFaceTimerKeep = NULL;      /* 人类信息更新计时器 */

        TimeKeep_S*      m_pCurRecordTimeKeep1s = NULL; /* 录制时长计时器-1s */
        std::atomic<int> m_nCurRecordTime;


        std::atomic_bool m_bStudentScreen;  /* 是否学生画面 */
        std::atomic_bool m_bStudentCloseUp; /* 是否学生特写 */
        std::atomic_bool m_bTeacherPodium;  /* 是否老师在讲台 */

        /* AI服务器已连接数量 */
        int m_nAiDevCount = DEV_LIMIT_NUM + 1;

#ifdef USE_LOCAL_AI
        /* 本地分析句柄 */
        AiLocal_NS::CAiLocal* m_pAiLocal = nullptr;
#endif

        /* AI服务器连接状态 */
        std::atomic<int> m_nAiServerStatus;

        std::atomic_bool m_bTtranscription;

        char m_achTemp[1024];
    };

}    // namespace AiManage_NS
