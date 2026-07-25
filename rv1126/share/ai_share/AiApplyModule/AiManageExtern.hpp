/*
 * @FilePath     : AiManageExtern.hpp
 * @Author       : 严泽辉 yanzeh@kfb.cn
 * @Date         : 2024-03-25 10:33:58
 * @LastEditors: weigl 308241951@qq.com
 * @LastEditTime: 2025-03-07 16:28:32
 * @Description  :
 */
#pragma once

#include <functional>
#include <iostream>
#include <list>

// #include "share_define.h"
#include "BlError.h"
#include "CVExtern.hpp"
#include "PlatformExtern.hpp"

namespace AiManage_NS
{
    typedef enum
    {
        /* 算法分析 10000-10100 */
        AI_COM_BOARD       = 10000, /* 板书识别 */
        AI_COM_EMO         = 10001, /* 表情识别 */
        AI_COM_ST_FACE     = 10002, /* 学生人脸识别 */
        AI_COM_TE_FACE     = 10003, /* 老师人脸识别 */
        AI_COM_HEAD        = 10004, /* 轨迹/人头识别 */
        AI_COM_NUM_COUNTER = 10005, /* 人数统计 */
        AI_COM_ST_BEHAVIOR = 10006, /* 学生行为分析 */
        AI_COM_STAS_FACE   = 10007, /* 学生回答问题人脸识别 */
        AI_COM_TEA_CALLPHONE = 10008, /* 老师接打电话识别 */
        AI_COM_ST_PLAYPHONE = 10009, /* 学生玩手机 */
        AI_COM_ST_DISCIPLINE = 10010, /* 学生课堂纪律 */

        /* control与*0630通信码 */
        AI_GET_DEV_INFO        = 20101,  /* 获取设备信息 */
        AI_UPDATE_FACE         = 20103,  /* 更新人脸信息 */
        AI_SET_IP_INFO         = 20104,  /* 更新IP信息 */
        CMD_SCREENSHOT_COMMAND = 20102,  /* 截图命令 */
        AI_DELETE_DEV          = 20105,  /* AI删除设备 */

        PC_CMD_HEARTBEAT_STATUS = 30032, /* 心跳 */
    } CMD_CODE_E;

/*学生行为截图*/
#define STU_BEHAV_SCREENSHOT      (1)
/* 考勤图片临时路径 */
#define GANCIAN_PICTURE_TEMP_PATH ("/opt/course/Attendance")
/* 关键截图临时路径 */
#define KEY_SNAPS_TEMP_PATH       ("/opt/course/KeySnaps")
/* 学生全景截图 */
#define ST_FULL_VIEW              ("/opt/course/KeySnaps/StFullView")

    /* ===============AI 返回结果======================= */
#if 1
    /* 定义情绪枚举 */
    typedef enum _Emotion_
    {
        ANGER = 0, /* 愤怒 */
        DISGUST,   /* 厌恶 */
        FEAR,      /* 恐惧 */
        JOY,       /* 快乐 */
        NEUTRAL,   /* 中性 */
        SADNESS,   /* 悲伤 */
        SURPRISE,  /* 惊喜 */
    } Emotion_E;

    /* 定义动作枚举 */
    typedef enum _Action_
    {
        ACTION_NULL = -1, /* 未识别到 */
        LOWER_HEAD  = 0,  /* 0 低头 */
        LIFT_HEAD,        /* 1 抬头 */
        TURN_HEAD,        /* 2 转头 */
        RAISE_HAND,       /* 3 举手 */
        STAND,            /* 4 站立 */
        TURN,             /* 5 转身 */
        DOWN_DESK,        /* 6 趴桌 */
        PLAY_PHONE,       /* 7 玩手机 */
        CALL_PHONE,       /* 8 接打电话 */
        TEA_BOARD,        /* 9 教师板书 */
    } Action_E;

    /* 框坐标信息的结构体 */
    typedef struct _BoxInfo_
    {
        int nX1; /* 左上角 x 坐标 */
        int nY1; /* 左上角 y 坐标 */
        int nX2; /* 右下角 x 坐标 */
        int nY2; /* 右下角 y 坐标 */

        void clear()
        {
            nX1 = 0;
            nY1 = 0;
            nX2 = 0;
            nY2 = 0;
        }

        bool empty()
        {
            return (nX1 == 0) && (nY1 == 0) && (nX2 == 0) && (nY2 == 0);
        }

        _BoxInfo_()
        {
            clear();
        }
    } BoxInfo_S;

    /* 数据头信息 */
    typedef struct _HeadInfo_
    {
        int       nMode;       /* 算法类型/命令码 */
        int       nFrameID;    /* 累加的帧ID */
        int       nClassId;    /* 班级ID */
        int       nRecordTime; /* 当前录制时长 */
        long long lTimestamp;  /* 当前时间戳 */
    } HeadInfo_S;

    /* 表情信息 */
    typedef struct _EmoItemInfo_
    {
        BoxInfo_S stBoxInfo;   /* 表情框信息 */
        Emotion_E enEmotion;   /* 表情类型 */
        float     fConfidence; /* 置信度 */
    } EmoItemInfo_S;

    /* 识别到的表情信息 */
    typedef struct _EmoInfo_
    {
        HeadInfo_S               stHeadInfo;  /* 数据头信息 */
        std::list<EmoItemInfo_S> listEmoInfo; /* 表情信息列表 */
    } EmoInfo_S;

    /* 人脸信息 */
    typedef struct _FaceItemInfo_
    {
        int       nId;         /* 人脸ID */
        BoxInfo_S stBoxInfo;   /* 人脸框信息 */
        float     fConfidence; /* 置信度 */
    } FaceItemInfo_S;

    /* 识别到的人脸信息 */
    typedef struct _FaceInfo_
    {
        HeadInfo_S                stHeadInfo;   /* 数据头信息 */
        std::list<FaceItemInfo_S> listFaceInfo; /* 人脸信息列表 */
    } FaceInfo_S;

    /*轨迹信息 */
    typedef struct _TrackItemInfo_
    {
        BoxInfo_S stBoxInfo;   /* 轨迹框信息 */
        float     fConfidence; /* 置信度 */
    } TrackItemInfo_S;

    /* 识别到的轨迹信息 */
    typedef struct _TrackInfo_
    {
        HeadInfo_S                 stHeadInfo;    /* 数据头信息 */
        std::list<TrackItemInfo_S> listTrackInfo; /* 轨迹信息列表 */
    } TrackInfo_S;

    /*人数信息 */
    typedef struct _NumberItemInfo_
    {
        BoxInfo_S stBoxInfo; /* 人数框信息 */
    } NumberItemInfo_S;

    /* 识别到的人数信息 */
    typedef struct _NumberInfo_
    {
        HeadInfo_S                  stHeadInfo;     /* 数据头信息 */
        int                         nTotal;         /* 总人数 */
        std::list<NumberItemInfo_S> listNumberInfo; /* 人数信息列表 */
    } NumberInfo_S;

    /* 识别到课堂纪律信息 */
    typedef struct _MoveProbability_
    {
        HeadInfo_S                  stHeadInfo;         /* 数据头信息 */
        double                      fMoveProbability;   /* 混乱度 */
    } MoveProbability_S;

    /* 行为信息 */
    typedef struct _BehaviorItemInfo_
    {
        BoxInfo_S stBoxInfo;   /* 行为框信息 */
        Action_E  enAction;    /* 动作类型 */
        float     fConfidence; /* 置信度 */
    } BehaviorItemInfo_S;

    /* 识别到的行为信息 */
    typedef struct _BehaviorInfo_
    {
        HeadInfo_S                    stHeadInfo;       /* 数据头信息 */
        std::list<BehaviorItemInfo_S> listBehaviorInfo; /* 行为信息列表 */
    } BehaviorInfo_S;

    /*板书信息 */
    typedef struct _BoardItemInfo_
    {
        BoxInfo_S stBoxInfo;   /* 板书框信息 */
        float     fConfidence; /* 置信度 */
    } BoardItemInfo_S;

    /* 识别到的板书信息 */
    typedef struct _BoardInfo_
    {
        HeadInfo_S                 stHeadInfo;    /* 数据头信息 */
        std::list<BoardItemInfo_S> listBoardInfo; /* 板书信息列表 */
    } BoardInfo_S;

#endif
    /* ===============AI 返回结果======================= */

    /* 数据格式 */
    typedef enum _CodedFormat_
    {
        H264,
        H265,
        RGB888,
        BGR888,
        RGBA8888,
        JPEG,
    } CodedFormat_E;

    /* 算法处理模式 */
    typedef enum _ProcessMode_
    {
        LOCAL_MODE = 0, /* 本地分析 */
        SERVER_MODE,    /* 远端服务器分析 */
    } ProcessMode_E;

    /* 算法参数 */
    typedef struct _AlgorithmParam_
    {
        bool          bOpen;      /* 是否开启 */
        ProcessMode_E enMode;     /* 算法处理模式 */
        int           nIntervals; /* 分析间隔 单位/ms */
        void clear()
        {
            bOpen      = false;
            enMode     = SERVER_MODE;
            nIntervals = 0;
        }
    } AlgorithmParam_S;

    /* 班级成员信息 */
    typedef struct _ClassMemberInfo_
    {
        char achName[32];        /* 成员名称 */
        int  nCardId;            /* 成员身份 */
        int  nUserId;            /* 用户Id */
        char achFilePath[256];   /* 成员图片网络路径 */
        char achActualPath[512]; /* 成员图片实际路径 */
    } ClassMemberInfo_S;

    /* 班级信息 */
    typedef struct _ClassInfo_
    {
        int                nTotal;            /* 班级人数 */
        int                nClassId;          /* 班级ID */
        char               achClassName[256]; /* 班级名称 */
        ClassMemberInfo_S* pstMemberInfo;     /* 成员信息 -- 哪里声明哪里释放*/
    } ClassInfo_S;

    /**
     * @brief 获取录制时长函数
     * @return [int] 返回录制时长
     * @note
     */
    typedef std::function<int(void)> getRecordTimeFunc;

    /**
     * @brief 获取录制状态函数
     * @return [int] 返回录制状态
     * @note
     */
    typedef std::function<int(void)> getRecordStateFunc;

    /**
     * @brief 获取AI服务器IP的函数函数
     * @return [std::string] 返回AI服务器IP
     * @note
     */
    typedef std::function<std::string(void)> getAiServerIpFunc;

    /**
     * @brief 设置AI服务器IP的函数函数
     * @param [std::string] : AI服务器IP
     * @return [BlError_E] >= BlError_E::OK 成功  其他失败
     * @note
     */
    typedef std::function<BlError_E(std::string)> setAiServerIpFunc;


    /**
     * @brief 设置AI服务器连接状态的函数函数
     * @param [int] : AI服务器连接状态
     * @return [BlError_E] >= BlError_E::OK 成功  其他失败
     * @note
     */
    typedef std::function<BlError_E(int)> setAiServerNetFunc;

    /**
     * @brief 获取绑定的课室ID的函数函数
     * @return [int] 返回绑定的课室ID
     * @note
     */
    typedef std::function<int(void)> getClassIdFunc;

    /**
     * @brief 设置绑定的课室ID的函数函数
     * @param [int] : 绑定的课室ID
     * @return [BlError_E] >= BlError_E::OK 成功  其他失败
     * @note
     */
    typedef std::function<BlError_E(int)> setClassIdFunc;

    /**
     * @brief 判断老师是否在讲台
     * @return [bool] true-在讲台 false-不在讲台
     * @note
     */
    typedef std::function<bool(void)> isTeacherPodiumFunc;

    /**
     * @brief 判断是否在学生特写
     * @return [bool] true-在学生特写 false-不在学生特写
     * @note
     */
    typedef std::function<bool(void)> isStudentCloseUpFunc;

    /**
     * @brief 学生特写人脸考勤云台控制
     * @return [bool] true-成功 false-本次考勤结束
     * @note
     */
    typedef std::function<bool(void)> PTZControlFunc;

    /**
     * @brief 发送截图命令函数指针
     * @param [std::string] : 截图路径
     * @param [int] : 通道号
     * @return [bool] 成功
     * @note
     */
    typedef std::function<bool(std::string, int)> sendScreenshotFunc;

    /**
     * @brief 发送stream获取分析数据函数指针
     * @param [int] : 命令吗
     * @return
     * @note
     */
    typedef std::function<BlError_E(int)> sendStreamGetDataFunc;

    /**
     * @brief AI服务器状态改变通知函数指针
     * @return
     * @note
     */
    typedef std::function<BlError_E(void)> aiServerStatusChangeFunc;

    /**
     * @brief 判断当前是否为本地分析函数指针
     * @return
     * @note
     */
    typedef std::function<bool(void)> isLocalModeFunc;

    /**
     * @brief 设置班级数据信息
     * @return
     * @note
     */
    typedef std::function<BlError_E(ClassInfo_S)> setClassInfoFunc;

    /**
     * @brief 获取平台
     * @return
     * @note
     */
    typedef std::function<std::string(void)> getPlatformIpFunc;

    /**
     * @brief 获取steam数据
     * @return
     * @note
     */
    typedef std::function<BlError_E(int)> notifyGetSteamDataFunc;

    /**
     * @brief 通知网页人脸信息获取状态
     * @return
     * @note
     */
    typedef std::function<BlError_E(BlError_E)> notifyWsFaceStateFunc;

    /**
     * @brief 发送学生全景截图命令函数指针
     * @param [std::string] : 截图路径
     * @param [int] : 通道号
     * @return [bool] 成功
     * @note
     */
    typedef std::function<bool(std::string)> sendStuPanoSSFunc;

    /**
     * @brief 发送学生特写截图命令函数指针
     * @param [std::string] : 截图路径
     * @param [int] : 通道号
     * @return [bool] 成功
     * @note
     */
    typedef std::function<bool(std::string)> sendStuSpecSSFunc;

    /**
     * @brief 处理上报教师预警事件给教育云平台函数指针
     * @param [FaceInfo_S] : 人脸信息参数
     * @return [BlError_E] 错误码
     * @note
     */
    typedef std::function<BlError_E(FaceInfo_S)> platformTeaEventFunc;

    /**
     * @brief 处理上报是否开始哪种行为给教育云平台函数指针
     * @param [int] nStartOrStop: START_BEHAVIOR-开始 STOP_BEHAVIOR 结束
     * @param [int] nBehavior: 行为类型
     * @param [int] nPersonType: 行为发生对象
     * @return *
     * @note
     */
    typedef std::function<void(int nStartOrStop, int nBehavior, int nPersonType)> platformBehaviorEventFunc;

    /**
     * @brief 处理上报学生考勤预警信息给教育云平台函数指针
     * @param [NumberInfo_S] 人数统计参数
     * @return [BlError_E] 错误码
     * @note
     */
    typedef std::function<BlError_E(NumberInfo_S)> platformAtStuAlertFunc;

    /**
     * @brief 处理上报学生趴桌预警信息给教育云平台函数指针
     * @param [int] 趴桌人数
     * @return [BlError_E] 错误码
     * @note
     */
    typedef std::function<BlError_E(int)> platformSdStuAlertFunc;

    /**
     * @brief 上报教育云平台预警信息函数指针
     * @param [int] nAlertType: 预警类型
     * @param [const char*] pDetails: 提示信息
     * @return [*]
     * @note
     */
    typedef std::function<BlError_E(int nAlertType, const char* pDetails)> platformPushAlertTypeFunc;

     /**
     * @brief 是否是教育云平台上课时间函数指针
     * @return [*]
     * @note
     */
    typedef std::function<bool(void)> isPlatformClassTimeFunc;

    /**
     * @brief 获取教育云平台班级信息函数指针
     * @param [stDataInfo] 班级信息
     * @return [BlError_E] 错误码
     * @note
     */
    typedef std::function<BlError_E(PlatformManage_NS::DataInfo_S stDataInfo)> getPlatformClassInfoFunc;

    /**
     * @brief 上报教育云平台缺勤人数和录制时长函数指针
     * @param [int] AI识别到的人数
     * @param [int] 抬头率
     * @param [int] 低头率
     * @return [BlError_E] 错误码
     * @note
     */
    typedef std::function<BlError_E(int, double, double)> platformPushStatusFunc;

    /**
     * @brief 获取教育云平台已经上课了多少时间函数指针
     * @return [*]
     * @note
     */
    typedef std::function<long(void)> getClassTimeFunc;

    /**
     * @brief 上报学生表情给教育云平台函数指针
     * @param [int] 表情类型
     * @return [BlError_E] 错误码
     * @note
     */
    typedef std::function<BlError_E(int)> pfPushEmoticonTypeFunc;

    /**
     * @brief 是否是教育云平台AI分析时间函数指针
     * @return [*]
     * @note
     */
    typedef std::function<bool(void)> isPlatformAiTimeFunc;

    /**
     * @brief 获取教育云平台预警开关状态函数指针
     * @param [int] 预警类型
     * @return [int] 开关状态
     * @note
     */
    typedef std::function<int(int)> getPlatformSwitchFunc;

    /**
     * @brief 获取跟踪类型函数指针
     * @return [*]
     * @note
     */
    typedef std::function<int(void)> getTrackTypeInfoFunc;
    

    /* 参数结构体 */
    typedef struct _Param_
    {
        AlgorithmParam_S               stTeTelephoneCalls;/* 教师接打电话识别 */
        AlgorithmParam_S               stStuDiscipline;   /* 学生课堂纪律 */
        AlgorithmParam_S               stStuPlayPhone;    /* 学生玩手机 */
        AlgorithmParam_S               stCountStudents;   /* 学生人数统计 */
        AlgorithmParam_S               stStudentBehavior; /* 学生行为分析 */
        AiScenario_NS::BehaviorParam_S stBehaviorExParam; /* 学生行为分析额外参数 */
        AlgorithmParam_S               stEmoDetecr;       /* 表情识别 */
        AlgorithmParam_S               stTeFaceDetecr;    /* 教师人脸识别 */
        AlgorithmParam_S               stStFaceDetecr;    /* 学生人脸识别 */
        AlgorithmParam_S               stTrackTeacher;    /* 老师轨迹检测 */
        AlgorithmParam_S               stBoardDetecr;     /* 板书检测 */
        
        getRecordTimeFunc        getRecordTime;           /* 获取录制时长 */
        getRecordStateFunc       getRecordState;          /* 获取录制状态 */
        getAiServerIpFunc        getAiServerIp;           /* 获取Ai服务器IP */
        setAiServerIpFunc        setAiServerIp;           /* 设置Ai服务器IP */
        getClassIdFunc           getClassId;              /* 获取绑定的课室ID */
        setClassIdFunc           setClassId;              /* 设置绑定的课室ID */
        PTZControlFunc           PTZControl;              /* 学生特写人脸考勤云台控制 */
        aiServerStatusChangeFunc aiServerStatusChange;    /* AI服务器状态改变通知 */
        setClassInfoFunc         setClassInfo;            /* 设置班级信息 */
        getPlatformIpFunc        getPlatformIp;           /* 获取平台IP */
        notifyGetSteamDataFunc   notifyGetSteamData;      /* 获取steam数据 */
        notifyWsFaceStateFunc    notifyWsFaceState;       /* 通知网页人脸信息获取状态 */
        sendStuPanoSSFunc        sendStuPanoSS;           /* 发送学生全景截图命令函数指针 */
        sendStuSpecSSFunc        sendStuSpecSS;           /* 发送学生特写截图命令函数指针 */
        platformTeaEventFunc     platformTeaEvent;        /* 处理上报教师预警事件给教育云平台函数指针 */
        platformBehaviorEventFunc platformBehaviorEvent;  /* 处理上报是否开始哪种行为给教育云平台函数指针 */
        platformAtStuAlertFunc   platformAtStuAlert;      /* 处理上报学生考勤预警信息给教育云平台函数指针 */
        platformSdStuAlertFunc   platformSdStuAlert;      /* 处理上报学生趴桌预警信息给教育云平台函数指针 */
        platformPushAlertTypeFunc platformPushAlertType;  /* 上报教育云平台预警信息函数指针 */
        isPlatformClassTimeFunc  isPlatformClassTime;     /* 是否是教育云平台上课时间函数指针 */
        getPlatformClassInfoFunc getPlatformClassInfo;    /* 获取教育云平台班级信息函数指针 */
        platformPushStatusFunc   platformPushStatus;      /* 上报教育云平台缺勤人数和录制时长函数指针 */
        getClassTimeFunc         getClassTime;            /* 获取教育云平台已经上课了多少时间函数指针 */
        pfPushEmoticonTypeFunc   pfPushEmoticonType;      /* 上报学生表情给教育云平台函数指针 */
        isPlatformAiTimeFunc     isPlatformAiTime;        /* 是否是教育云平台AI分析时间函数指针 */
        getPlatformSwitchFunc    getPlatformSwitch;       /* 获取教育云平台预警开关状态函数指针 */
        getTrackTypeInfoFunc     getTrackTypeInfo;        /* 获取跟踪类型 */
        
        std::string strDownloadPath;                      /* 人脸图片下载脚本路径 */
        std::string strUploadPath;                        /* 人脸图片上传脚本路径 */
        std::string strDevType;                           /* 本机设备类型 */
        int         nAiClientPort;                        /* AI客户端端口 -- 本机为客户端 */
        int         nAiServerPort;                        /* Ai服务器断开 -- 本机为服务器 */

        void clear()
        {
            getRecordTime        = nullptr;
            getRecordState       = nullptr;
            getAiServerIp        = nullptr;
            setAiServerIp        = nullptr;
            getClassId           = nullptr;
            setClassId           = nullptr;
            PTZControl           = nullptr;
            aiServerStatusChange = nullptr;
            setClassInfo         = nullptr;
            getPlatformIp        = nullptr;
            notifyGetSteamData   = nullptr;
            notifyWsFaceState    = nullptr;
            sendStuPanoSS        = nullptr;
            sendStuSpecSS        = nullptr;
            platformTeaEvent     = nullptr;
            platformBehaviorEvent= nullptr;
            platformAtStuAlert   = nullptr;
            platformPushAlertType= nullptr;
            isPlatformClassTime  = nullptr;
            getPlatformClassInfo = nullptr;
            platformPushStatus   = nullptr;
            getClassTime         = nullptr;
            pfPushEmoticonType   = nullptr;
            isPlatformAiTime     = nullptr;
            getPlatformSwitch    = nullptr;
            getTrackTypeInfo     = nullptr;
            
            stTeTelephoneCalls.clear();
            // stStuDiscipline.clear();
            stCountStudents.clear();
            stStudentBehavior.clear();
            stEmoDetecr.clear();
            stTeFaceDetecr.clear();
            stStFaceDetecr.clear();
            stTrackTeacher.clear();
            stBoardDetecr.clear();
            strDownloadPath.clear();
            strUploadPath.clear();
            strDevType.clear();
            nAiClientPort = 0;
            nAiServerPort = 0;
        }

        _Param_()
        {
            clear();
        }
    } Param_S;

    /* AI通讯数据信息结构体 */
    typedef struct _CommDataInfo_
    {
        int       nCode;          /* 通信码 */
        int       nCurRecordTime; /* 当前录制时长 */
        int       nClassId;       /* 课室ID */
        long long lTimestamp;     /* 当前时间戳 */

        CodedFormat_E enCodec;    /* 编码格式 */
        int           nExSize;    /* 额外数据大小 */
        int           nDataSize;  /* 数据内容的大小 */
        char          data[0];    /* 指向数据内容  数据结构=额外数据+图片数据 */

        _CommDataInfo_()
        {
            nCode          = 0;
            nCurRecordTime = 0;
            nClassId       = 0;
            lTimestamp     = 0;
            enCodec        = JPEG;
            nDataSize      = 0;
            nExSize        = 0;
        }

        int size()
        {
            return sizeof(_CommDataInfo_) + nExSize + nDataSize;
        }

    } CommDataInfo_S;

    /* 设备信息 */
    typedef struct _DevInfo_
    {
        std::string strDevModel; /* 设备型号 */
        std::string strDevIp;    /* 设备IP */
        std::string strDevMac;   /* 设备MAC */

        void clear()
        {
            strDevModel.clear();
            strDevIp.clear();
            strDevMac.clear();
        }

        void print()
        {
            std::cout << "设备型号:" << strDevModel << std::endl;
            std::cout << "IP地址:" << strDevIp << std::endl;
            std::cout << "MAC地址:" << strDevMac << std::endl;
        }
    } DevInfo_S;

    /* ai和录播的心跳数据结构 */
    typedef struct _VodHeartInfo_
    {
        int nDevCount; /*已连接设备数量*/

        _VodHeartInfo_()
        {
            nDevCount = 0;
        }

        void clear()
        {
            nDevCount = 0;
        }

        void print()
        {
            std::cout << "nDevCount:" << nDevCount << std::endl;
        }
    } VodHeartInfo_S;


}    // namespace AiManage_NS