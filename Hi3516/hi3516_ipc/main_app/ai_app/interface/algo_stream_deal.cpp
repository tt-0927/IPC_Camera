/**
 * @FilePath     : algo_stream_deal.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2025-06-06 16:02:10
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-04-28 20:22:54
 * @Description  : 负责取流传给算法检测
 */

#include "algo_stream_deal.h"

#include "algo_statistics_event_publisher.hpp"

#if CAP_AI_PEOPLE_DENSITY_LEGACY && CAP_AI_PEOPLE_DENSITY_V2
#error "CAP_AI_PEOPLE_DENSITY_LEGACY and CAP_AI_PEOPLE_DENSITY_V2 cannot be enabled at the same time"
#endif

#if (CAP_AI_PEOPLE_DENSITY_LEGACY || CAP_AI_PEOPLE_DENSITY_V2) && !CAP_AI_PEOPLE_STATISTICS
#error "CAP_AI_PEOPLE_STATISTICS must be enabled when people density capability is enabled"
#endif

int CAlgoStreamDeal::init()
{
    /*初始化算法句柄*/
    runOnce();
    /* 初始化事件统计上报器 */
    initEventStatisticsReporter();
    m_StreamHandler = std::make_shared<CStreamHandler>();
    dlog_debug("AI_APP: 初始化算法音视频流数据处理");
    return 0;
}

void CAlgoStreamDeal::deinit()
{
    /* 取消所有绑定 */
    unbindVideoSig(m_StreamHandler.get());

    /* 统一存储需要释放的句柄 */
    std::vector<std::shared_ptr<CAlgorithm>> algos;
    algos.emplace_back(std::move(m_pMotionAlgo));
    algos.emplace_back(std::move(m_pHideAlgo));
    algos.emplace_back(std::move(m_pAudioAlgo));
    algos.emplace_back(std::move(m_pSceneChangeAlgo));
    algos.emplace_back(std::move(m_pHVFAlgo));
    algos.emplace_back(std::move(m_pPeopleHeadAlgo));
    algos.emplace_back(std::move(m_pItemAlgo));
    algos.emplace_back(std::move(m_pPetAlgo));
    algos.emplace_back(std::move(m_pFaceAlgo));
#if CAP_AI_GARBAGE_DETECT
    algos.emplace_back(std::move(m_pGarbageAlgo));
#endif
    m_pEventStatisticsReporter.reset();

    /* 释放算法实例 */
    for (auto &pAlgo : algos)
    {
        pAlgo.reset();
        dlog_debug("AI_APP: 释放算法实例 引用计数为[%d]", pAlgo.use_count());
    }

    /* 释放 StreamHandler */
    m_StreamHandler.reset();
}

void CAlgoStreamDeal::deal_message(const ot_video_frame_info *pFrameInfo)
{
    if (pFrameInfo != nullptr)
    {
        m_StreamHandler.get()->recvDataProcess(pFrameInfo);
    }
    else
    {
        dlog_error("收到无效消息或空数据");
    }
}

void CAlgoStreamDeal::deal_message(const ot_audio_frame *pFrame)
{
    if (pFrame != nullptr)
    {
        m_StreamHandler.get()->recvDataProcess(pFrame);
    }
    else
    {
        dlog_error("收到无效消息或空数据");
    }
}

void CAlgoStreamDeal::deal_videoStreamData(const void *pData, int nLength, int nWidth, int nHeight)
{
    if (pData != nullptr && nLength > 0)
    {
        m_StreamHandler.get()->recvDataProcess(pData, nLength, nWidth, nHeight);
    }
    else
    {
        dlog_error("收到无效消息或空数据");
    }
}

void CAlgoStreamDeal::deal_audioStreamData(const void *pData, int nLength)
{
    if (pData != nullptr && nLength > 0)
    {
        m_StreamHandler.get()->recvDataProcess(pData, nLength);
    }
    else
    {
        dlog_error("收到无效消息或空数据");
    }
}

void CAlgoStreamDeal::bindRecvFunc(Event::AlgorithmConfig &stAlgoConfig)
{
    if (!m_StreamHandler)
    {
        dlog_error("绑定回调失败， m_StreamHandler未初始化");
        return;
    }

    /* 视频 配置项和算法对象的映射 */
    std::vector<std::pair<bool, std::shared_ptr<CAlgorithm>>> videoAlgoBindings =
    {
        {stAlgoConfig.nEnMotionDetect, m_pMotionAlgo},
        {stAlgoConfig.nEnOcclusionDetect, m_pHideAlgo},
        {stAlgoConfig.nEnSceneChange, m_pSceneChangeAlgo},
        {stAlgoConfig.nEnFaceDetect || stAlgoConfig.nEnLoiteringDetect || stAlgoConfig.nEnParkingDetect || stAlgoConfig.nEnLineCrossing||stAlgoConfig.nEnIntrusion || stAlgoConfig.nEnEnterRegion || stAlgoConfig.nEnLeaveRegion
#if CAP_AI_PEOPLE_STATISTICS
         || stAlgoConfig.nEnPeopleFlowStatistics
#endif
#if CAP_AI_PEOPLE_DENSITY_V2
         || stAlgoConfig.nEnPeopleDensityDetection
#endif
         , m_pHVFAlgo},
        {stAlgoConfig.nEnCrowdGathering
#if CAP_AI_PEOPLE_DENSITY_LEGACY
         || stAlgoConfig.nEnPeopleDensityDetection
#endif
         , m_pPeopleHeadAlgo},
        {stAlgoConfig.nEnUnattendedObject || stAlgoConfig.nEnObjectRemoval, m_pItemAlgo},
        {stAlgoConfig.nEnPetRecognition, m_pPetAlgo},
        {stAlgoConfig.nEnFaceCapture || stAlgoConfig.nEnFaceLib ||stAlgoConfig.nEnFaceCompare, m_pFaceAlgo},
#if CAP_AI_GARBAGE_DETECT
        {stAlgoConfig.nEnGarbageExposure || stAlgoConfig.nEnGarbageOverflow, m_pGarbageAlgo},
#endif
    };

    for (const auto& videoBinding : videoAlgoBindings)
    {
        if (videoBinding.first && videoBinding.second)
        {
            dlog_debug("AI_APP: 绑定视频回调成功");
            bindVideoSlot<CAlgorithm>(m_StreamHandler.get(), videoBinding.second.get(), &CAlgorithm::recvMediaData);
        }
    }

    /* 音频 配置项和算法对象的映射 */
    std::vector<std::pair<bool, std::shared_ptr<CAlgorithm>>> audioAlgoBindings =
    {
        {stAlgoConfig.nEnAudioAnomaly, m_pAudioAlgo},
    };

    for (const auto& audioBinding : audioAlgoBindings)
    {
        if (audioBinding.first && audioBinding.second)
        {
            dlog_debug("AI_APP: 绑定音频回调成功");
            bindAudioSlot<CAlgorithm>(m_StreamHandler.get(), audioBinding.second.get(), &CAlgorithm::recvMediaData);
        }
    }
}

void CAlgoStreamDeal::manageAlgorithmInstances(const Event::AlgorithmConfig& stAlgoConfig)
{
    int bIsNew = 0;

    /* 移动侦测算法 */
    bIsNew += manageSingleAlgorithm(m_pMotionAlgo, stAlgoConfig.nEnMotionDetect,
                                    []() -> std::shared_ptr<CAlgorithm>
                                    {
                                        return std::static_pointer_cast<CAlgorithm>(std::make_shared<CMotionDetect>());
                                    });
    /* 遮挡侦测算法 */
    bIsNew += manageSingleAlgorithm(m_pHideAlgo, stAlgoConfig.nEnOcclusionDetect,
                                    []()
                                    {
                                        return std::static_pointer_cast<CAlgorithm>(std::make_shared<CHideDetect>());
                                    });
    /* 音频异常侦测算法 */
    bIsNew += manageSingleAlgorithm(m_pAudioAlgo, stAlgoConfig.nEnAudioAnomaly,
                                    []()
                                    {
                                        return std::static_pointer_cast<CAlgorithm>(std::make_shared<CAudioDetect>());
                                    });
    /* 场景变更侦测算法 */
    bIsNew += manageSingleAlgorithm(m_pSceneChangeAlgo, stAlgoConfig.nEnSceneChange,
                                    []()
                                    {
                                        return std::static_pointer_cast<CAlgorithm>(std::make_shared<CSceneChangeDetect>());
                                    });
    /* 脸人车侦测算法 */
    bIsNew += manageSingleAlgorithm(m_pHVFAlgo, stAlgoConfig.nEnFaceDetect || stAlgoConfig.nEnLoiteringDetect || stAlgoConfig.nEnParkingDetect ||stAlgoConfig.nEnLineCrossing || stAlgoConfig.nEnIntrusion || stAlgoConfig.nEnEnterRegion || stAlgoConfig.nEnLeaveRegion
#if CAP_AI_PEOPLE_STATISTICS
                                    || stAlgoConfig.nEnPeopleFlowStatistics
#endif
#if CAP_AI_PEOPLE_DENSITY_V2
                                    || stAlgoConfig.nEnPeopleDensityDetection
#endif
                                    ,
                                    []() -> std::shared_ptr<CAlgorithm>
                                    {
                                        return std::static_pointer_cast<CAlgorithm>(std::make_shared<CHVFDetect>());
                                    });
    /* 人头检测算法 */
    bIsNew += manageSingleAlgorithm(m_pPeopleHeadAlgo, stAlgoConfig.nEnCrowdGathering
#if CAP_AI_PEOPLE_DENSITY_LEGACY
                                    || stAlgoConfig.nEnPeopleDensityDetection
#endif
                                    ,
                                    []() -> std::shared_ptr<CAlgorithm>
                                    {
                                        return std::static_pointer_cast<CAlgorithm>(std::make_shared<CPeopleHeadDetect>());
                                    });
    /* 物品侦测算法 */
    bIsNew += manageSingleAlgorithm(m_pItemAlgo, stAlgoConfig.nEnUnattendedObject || stAlgoConfig.nEnObjectRemoval,
                                    []() -> std::shared_ptr<CAlgorithm>
                                    {
                                        return std::static_pointer_cast<CAlgorithm>(std::make_shared<CItemDetect>());
                                    });
    /* 宠物识别算法 */
    bIsNew += manageSingleAlgorithm(m_pPetAlgo, stAlgoConfig.nEnPetRecognition,
                                    []() -> std::shared_ptr<CAlgorithm>
                                    {
                                        return std::static_pointer_cast<CAlgorithm>(std::make_shared<CPetRecognition>());
                                    });
    /* 人脸检测算法 */
    bIsNew += manageSingleAlgorithm(m_pFaceAlgo, stAlgoConfig.nEnFaceCapture ||stAlgoConfig.nEnFaceLib ||stAlgoConfig.nEnFaceCompare,
                                    []() -> std::shared_ptr<CAlgorithm>
                                    {
                                        return std::static_pointer_cast<CAlgorithm>(std::make_shared<CFaceDetect>());
                                    });
#if CAP_AI_GARBAGE_DETECT
    /* 垃圾检测算法 */
    bIsNew += manageSingleAlgorithm(m_pGarbageAlgo, stAlgoConfig.nEnGarbageExposure || stAlgoConfig.nEnGarbageOverflow,
                                    []() -> std::shared_ptr<CAlgorithm>
                                    {
                                        return std::static_pointer_cast<CAlgorithm>(std::make_shared<CGarbageDetect>());
                                    });
#endif
    if (bIsNew)
    {
        /* 统一获取区域配置 */
        dlog_debug("AI_APP: 统一获取区域配置 bIsNew[%d]", bIsNew);
    }
}

bool CAlgoStreamDeal::manageSingleAlgorithm(std::shared_ptr<CAlgorithm> &algo, bool bEnabled, std::function<std::shared_ptr<CAlgorithm>()> creator)
{
    if (bEnabled)
    {
        if (!algo)
        {
            algo = creator();
            dlog_debug("AI_APP: 初始化算法实例");
            return true;
        }
    }
    else
    {
        if (algo)
        {
            algo.reset();
            dlog_debug("AI_APP: 释放算法实例 引用计数为[%d]", algo.use_count());
            return false;
        }
    }
    return false;
}

void CAlgoStreamDeal::set_Algo_EnConfig(Event::AlgorithmConfig &stAlgoConfig)
{
    std::lock_guard<std::mutex> applyLock(m_algorithmApplyMutex);

    /* 始终保存用户最新的期望配置，但独占期间不加载任何常驻算法。 */
    m_stDesiredAlgoConfig = stAlgoConfig;
    m_bDesiredAlgoConfigValid = true;
    if (m_bFaceLibExclusive)
    {
        dlog_info("AI_APP: 人脸名单库正在独占AI资源，算法配置已保存，稍后恢复");
        return;
    }

    applyAlgorithmConfig(stAlgoConfig);
}

void CAlgoStreamDeal::applyAlgorithmConfig(Event::AlgorithmConfig &stAlgoConfig)
{
    std::lock_guard<std::mutex> faceAlgoLock(m_faceAlgoMutex);
    printAlgoCfg(stAlgoConfig);

    /* 取消所有绑定 */
    unbindVideoSig(m_StreamHandler.get());
    unbindAudioSig(m_StreamHandler.get());

    initEventStatisticsReporter();

    /* 根据配置管理算法实例 */
    manageAlgorithmInstances(stAlgoConfig);

    /* 重新绑定回调函数 */
    bindRecvFunc(stAlgoConfig);

    /* 通知 Algorithm 更新参数 */
    std::vector<std::shared_ptr<CAlgorithm>> algos = {
        m_pMotionAlgo, m_pHideAlgo, m_pAudioAlgo, m_pSceneChangeAlgo, m_pHVFAlgo, m_pPeopleHeadAlgo, m_pItemAlgo, m_pPetAlgo, m_pFaceAlgo
#if CAP_AI_GARBAGE_DETECT
        , m_pGarbageAlgo
#endif
    };

    for (auto &pAlgo : algos)
    {
        if (pAlgo)
        {
            pAlgo->setEventStatisticsReporter(m_pEventStatisticsReporter);
            pAlgo->setAlgoEnCfg(stAlgoConfig);
        }
    }
}
#if CAP_AI_FACE_COMPARE

int CAlgoStreamDeal::beginFaceLibExclusive()
{
    std::lock_guard<std::mutex> applyLock(m_algorithmApplyMutex);
    m_bFaceLibExclusive = true;

    try
    {
        dlog_info("AI_APP: 人脸名单库开始独占AI资源，暂停当前运行算法");
        Event::AlgorithmConfig stDisabledConfig{};
        applyAlgorithmConfig(stDisabledConfig);
        dlog_info("AI_APP: 当前运行算法已暂停，人脸名单库可以加载临时模型");
        return FACE_LIB_ADD_OK;
    }
    catch (...)
    {
        m_bFaceLibExclusive = false;
        dlog_error("AI_APP: 暂停当前运行算法时发生异常");
        return ERR_AI_SUSPEND_FAILED;
    }
}

int CAlgoStreamDeal::endFaceLibExclusive()
{
    std::lock_guard<std::mutex> applyLock(m_algorithmApplyMutex);
    int nRet = FACE_LIB_ADD_OK;

    /*
     * 持锁恢复，避免恢复过程中另一个配置线程穿插加载模型。独占期间收到的
     * 配置已经更新到 m_stDesiredAlgoConfig，因此这里恢复的是最新状态。
     */
    try
    {
        if (m_bDesiredAlgoConfigValid)
        {
            dlog_info("AI_APP: 人脸名单库释放AI资源，恢复最新算法配置");
            applyAlgorithmConfig(m_stDesiredAlgoConfig);
        }
        else
        {
            dlog_info("AI_APP: 尚未收到算法配置，人脸名单库结束后保持算法关闭");
        }
    }
    catch (...)
    {
        nRet = ERR_AI_RESTORE_FAILED;
        dlog_error("AI_APP: 恢复算法配置时发生异常");
    }

    m_bFaceLibExclusive = false;
    dlog_info("AI_APP: 人脸名单库AI资源独占结束, restoreRet[%d]", nRet);
    return nRet;
}
#endif
int CAlgoStreamDeal::dispatchRuntimeCommand(const RuntimeCommand_S &stCommand)
{
    std::vector<std::shared_ptr<CAlgorithm>> algos = {
        m_pMotionAlgo, m_pHideAlgo, m_pAudioAlgo, m_pSceneChangeAlgo, m_pHVFAlgo, m_pPeopleHeadAlgo, m_pItemAlgo, m_pPetAlgo, m_pFaceAlgo
#if CAP_AI_GARBAGE_DETECT
        , m_pGarbageAlgo
#endif
    };

    int nRet = ERR;
    for (auto &pAlgo : algos)
    {
        if (pAlgo && pAlgo->handleRuntimeCommand(stCommand) == OK)
        {
            nRet = OK;
        }
    }
    return nRet;
}

void CAlgoStreamDeal::initEventStatisticsReporter()
{
    if (!m_pEventStatisticsReporter)
    {
        m_pEventStatisticsReporter = std::make_shared<AiAppCommon::CAlgoStatisticsEventPublisher>();
    }
}

// void CAlgoStreamDeal::set_Algo_ParamConfig(Alarm::TargetDetection_S &stAlgoCfg)
// {
//     if (m_pFaceAlgo)
//     {
//         m_pFaceAlgo->setAlgoParamCfg(stAlgoCfg);
//     }
// }

float CAlgoStreamDeal::getCurrentDb() const
{
    if (!m_pAudioAlgo)
    {
        return 0.0f;
    }
    return m_pAudioAlgo.get()->getCurrentDb();
}

void CAlgoStreamDeal::runOnce()
{
    /* 使用静态局部变量来跟踪函数是否已执行过 */
    static bool s_bHasRun = false;

    /* 如果已执行过，直接返回 */
    if (s_bHasRun)
    {
        return;
    }

    s_bHasRun = true;
    if (!m_pFaceAlgo)
    {

        m_pFaceAlgo = std::static_pointer_cast<CAlgorithm>(std::make_shared<CFaceDetect>());
    }
    else{

    }
}
#if CAP_AI_FACE_COMPARE
/**
 * @brief 添加人脸名单库
 * @param stFaceLibData
 */
int CAlgoStreamDeal::add_Facelib_Groups(FaceDataDB_NS::FaceLibsInfo_S &stFaceLibData)
{
        // int nRet;
        // /* 通知 Algorithm 人脸检测 */

        // sleep(1);
        // if (m_pFaceAlgo)
        // {
        //     nRet = m_pFaceAlgo.get()->addFaceLibGroup(stFaceLibData);
        // }else {
        // }

        std::lock_guard<std::mutex> faceLibAddLock(m_faceLibAddMutex);
        int nRet = ERR;
         /*
     * 低内存设备添加名单库时独占AI资源。这里只改变实际运行状态，不修改
     * 用户保存的算法配置；入库结束后恢复独占期间收到的最新配置。
     */
     const int nSuspendRet = beginFaceLibExclusive();
     if (nSuspendRet != FACE_LIB_ADD_OK)
     {
         return nSuspendRet;
     }
 
     /*
      * 暂停常驻算法之后创建一次性人脸实例。CFaceDetect::addFaceLibGroup内部
      * 使用YOLO/ArcFace低内存互斥模式，任务完成后先销毁临时实例再恢复算法。
     */
    std::shared_ptr<CAlgorithm> pFaceAlgo;
    try
    {
        dlog_info("AI_APP: 为名单库入库创建临时人脸算法实例");
        pFaceAlgo = std::static_pointer_cast<CAlgorithm>(std::make_shared<CFaceDetect>());
        if (!pFaceAlgo)
        {
            dlog_error("AI_APP: 创建人脸算法实例失败，无法添加名单库成员");
            nRet = ERR_FACE_ALGO_CREATE_FAILED;
        }
        else
        {
            nRet = pFaceAlgo->addFaceLibGroup(stFaceLibData);
        }
    }
    // if (!pFaceAlgo)
    catch (...)
    { 
        dlog_error("AI_APP: 名单库临时人脸算法执行时发生异常");
        nRet = ERR_FACE_ALGO_CREATE_FAILED;
    }

    // if (!pFaceAlgo)
    /* 必须先析构临时模型，再恢复其他算法，避免模型同时驻留。 */
    pFaceAlgo.reset();
    dlog_info("AI_APP: 名单库临时人脸算法实例已释放，准备恢复原算法.");
    const int nRestoreRet = endFaceLibExclusive();
    if (nRet == FACE_LIB_ADD_OK && nRestoreRet != FACE_LIB_ADD_OK)
    {
        // dlog_error("AI_APP: 创建人脸算法实例失败，无法添加名单库成员");
        // return ERR;
        nRet = nRestoreRet;
    }
    else if (nRestoreRet != FACE_LIB_ADD_OK)
    {
        dlog_error("AI_APP: 人脸添加失败且原算法恢复失败, addRet[%d] restoreRet[%d]",
                   nRet, nRestoreRet);
    }
    // nRet = pFaceAlgo->addFaceLibGroup(stFaceLibData);
    return nRet;
}
/**
 * @brief 人员检索人脸比对
 * @param strPicPath
 */
 void CAlgoStreamDeal::compare_Face_Retrieval(std::string strPicPath)
 {

     {
         /* 通知 Algorithm 人脸比对 */
         m_pFaceAlgo.get()->faceRetrieval(strPicPath);
     }

     return;
 }
#endif
