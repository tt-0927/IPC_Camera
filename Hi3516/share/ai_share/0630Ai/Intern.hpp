
#pragma once

#include <atomic>
#include <memory>
#include <thread>

#include "BlError.h"
#include "dlog.h"
#include "Extern.hpp"
#include "opencv2/opencv.hpp"
#include "OutputDataEXT.hpp"

#ifdef ARCH_YTLF
    #include "base_type.hpp"
#endif

namespace Ai0630_NS
{
    /* 通讯数据头 */
    struct CommInfo_S
    {
        int32_t     nCode;       /* 通信码 */
        int32_t     nOptType;    /* 控制类型 */
        std::string strUserName; /* 用户名 */
        std::string strData;     /* 内容 */
        int         nReturn;     /* 返回值 */

        void clear()
        {
            nCode    = 0;
            nOptType = 0;
            strUserName.clear();
            strData.clear();
            nReturn = 0;
        }

        CommInfo_S()
        {
            clear();
        }
    };

    /* 数据头 */
    struct HeaderInfo_S
    {
        int32_t  nCode;            /* 通信码 */
        uint64_t nAiFlags;         /* AI功能位 */
        bool     bNeedPostProcess; /* 是否需要后处理 */

        void clear()
        {
            nCode            = 0;
            nAiFlags         = 0;
            bNeedPostProcess = false;
        }

        HeaderInfo_S()
        {
            clear();
        }
    };

    /* 参数 */
    struct AiParamInfo_S
    {
        int                 nId;   /* 数据ID */
        Inference_NS::Box_S stBox; /* 在原图的上框 */

        void clear()
        {
            nId = 0;
        }

        AiParamInfo_S()
        {
            clear();
        }
    };

    /* 用户参数，需要原封不动的传递回去，用于区分数据结果 */
    struct UserParamInfo_S
    {
        std::shared_ptr<char[]> pData;
        int                     nSize;

        void clear()
        {
            nSize = 0;
            pData.reset();
        }

        UserParamInfo_S()
        {
            clear();
        }
    };

    /* 媒体信息 */
    struct MediaData_S
    {
        int nWidth;
        int nHeight;

#if defined(ARCH_YTLF)
        std::shared_ptr<dcl::Mat> pMatData;
#elif defined(ARCH_CIX)
        std::shared_ptr<cv::Mat> pMatData;
#else
        // 默认代码（可选）
#endif

        void clear()
        {
            nWidth  = 0;
            nHeight = 0;
#if defined(ARCH_YTLF)
            pMatData.reset();
#elif defined(ARCH_CIX)
            pMatData.reset();
#else

#endif
        }

        MediaData_S()
        {
            clear();
        }
    };

    struct Result_S
    {
        int                         nCode;       /* 命令码 */
        void*                       pCommHandle; /* 通讯句柄 */
        std::shared_ptr<CommData_S> pstCommData;

        void clear()
        {
            nCode       = 0;
            pCommHandle = nullptr;
            pstCommData.reset();
        }

        Result_S()
        {
            clear();
        }
    };

    /**
     * @brief 分析结果的回调函数
     */
    typedef std::function<BlError_E(Result_S)> resultCallbackFunc;

    /* 数据 */
    struct Data_S
    {
        HeaderInfo_S    stHeader;
        AiParamInfo_S   stAiParam;
        UserParamInfo_S stUserParam;
        MediaData_S     stMedia;

        void*              pCommHandle;
        resultCallbackFunc resultCallback;

        void clear()
        {
            stHeader.clear();
            stAiParam.clear();
            stUserParam.clear();
            stMedia.clear();

            pCommHandle    = nullptr;
            resultCallback = nullptr;
        }

        Data_S()
        {
            clear();
        }
    };

    struct AIResult_S
    {
        int                                    nId;
        std::vector<Inference_NS::ClsData_S>   vstClsData;
        std::vector<Inference_NS::BoxData_S>   vstBoxData;
        std::vector<Inference_NS::PointData_S> vstPointData;

        void clear()
        {
            nId = 0;
            vstClsData.clear();
            vstBoxData.clear();
            vstPointData.clear();
        }

        AIResult_S()
        {
            clear();
        }
    };

    struct HeadResult_S
    {
        std::vector<AIResult_S> vstAIHeadDetectResult;
        std::vector<AIResult_S> vstAIFastPoseResult;

        void clear()
        {
            vstAIHeadDetectResult.clear();
            vstAIFastPoseResult.clear();
        }

        HeadResult_S()
        {
            clear();
        }
    };

    struct FaceResult_S
    {
        std::vector<AIResult_S> vstAIFaceDetectResult;
        std::vector<AIResult_S> vstAIHeadDetectResult;
        std::vector<AIResult_S> vstAIFastPoseResult;
        std::vector<AIResult_S> vstAIFaceFeatureResult;
        std::vector<AIResult_S> vstAIHumanFeatureResult;
        std::vector<AIResult_S> vstAIFaceEmotionResult;

        void clear()
        {
            vstAIFaceDetectResult.clear();
            vstAIHeadDetectResult.clear();
            vstAIFastPoseResult.clear();
            vstAIFaceFeatureResult.clear();
            vstAIHumanFeatureResult.clear();
            vstAIFaceEmotionResult.clear();
        }

        FaceResult_S()
        {
            clear();
        }
    };

    /* 通讯数据头 */
    struct LoginInfo_S
    {
        std::string strUserName; /* 用户名 */
        std::string strPassword; /* 密码 */

        void clear()
        {
            strUserName.clear();
            strPassword.clear();
        }

        LoginInfo_S()
        {
            clear();
        }
    };

    /*同步时间*/
    struct AdjuestTime_S
    {
        std::string strDateTime;
    };

    /* 升级状态 */
    typedef enum _UpgradeRuslut_
    {
        UPGRADE_NULL              = -1, /*未升级*/
        UPGRADE_RUNING            = 0,  /*升级中*/
        UPGRADE_RUNFAIL           = 1,  /*升级失败*/
        UPGRADE_RUNSUCCESS        = 2,  /*升级成功*/
        UPGRADE_OTHERUPDATE_FAILE = 3,  /*其他服务器升级失败*/
    } UpgradeRuslut_E;

    struct UpgradeInfo_S
    {
        std::string     strFileName; /* 升级文件名 */
        std::string     strFilePath; /* 升级文件路径 */
        UpgradeRuslut_E enStatus;    /* 升级状态 */
    };

    /* 系统信息 */
    struct SystemInfo_S
    {
        std::string strVersion;  /* 版本号 */
        std::string strMachinSn; /* 机器码 */
    };

    enum class SysResetMode_E
    {
        SYS_RESET_ONLYRESET = 1, /*只复位，不重启*/
        SYS_RESET_REBOOT    = 2, /*复位并重启*/
    };

    /*通讯命令码*/
    typedef enum _RegisterCode_
    {
        REGISTER_CMD_NET_HEARTBIT = 30032, /*心跳*/

        REGISTER_DEV_ACTIVATION = 0x6601,  /* 激活设备 */
    } RegisterCode_E;

    /*激活类型枚举*/
    typedef enum
    {
        AT_ONE_WEEK = 0, /*一周*/
        AT_ONE_MONTH,    /*一月*/
        AT_TWO_MONTH,    /*两月*/
        AT_THREE_MONTH,  /*三月*/
        AT_HALF_YEAR,    /*半年*/
        AT_FOREVER,      /*永久*/
        AT_NULL = -1,    /*未注册/激活 必须放在最下面*/
    } ActivationTime_E;

    struct RegisterInfo_S
    {
        std::string strDevID;          /* 设备id,从硬件读出来 一共16位 */
        std::string strMachinSn;       /* 机器码，根据设置id自己组成对应格式机器码 */
        std::string strRegisterEg;     /* 根据机器码生成的注册码，有试用版本注册码和永久注册码 */
        std::string strStartTime;      /* 注册时间 */
        std::string strPrevTime;       /* 最后一次检查的时间 */
        long long   nUsableTimer;      /* 可用时长，单位：分钟 */

        ActivationTime_E enActionTime; /* 注册的类型 */
    };

#include <string>

    struct SysNetwork_S
    {
        int nDhcp;

        std::string strIp;
        std::string strNetmask;
        std::string strGateway;
        std::string strMacAddr;
        std::string strDns;
        std::string strBroadcast;
    };

}    // namespace Ai0630_NS