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
#if CAP_UNIFIED_DETECTION
/* 名单库入库服务（统一检测层画像的入库实现，头文件路径由 detection.cmake 提供） */
#include "library/face_library_service.hpp"
#endif
#if CAP_UNIFIED_DETECTION
/* 人脸出图 JPEG 硬编码服务：全关时释放通道（约4MB MMZ），下次出图自动重建 */
#include "jpeg_encode_service.hpp"
#endif
#if CAP_AI_GARBAGE_DETECT
#include "capture_ctrl.h"
#include "time_utils.h"
#endif

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
    algos.emplace_back(std::move(m_pDetectionEngine));
    algos.emplace_back(std::move(m_pFaceEngine));
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
#if defined(SCENE_INTELLIGENCE) || CAP_AI_SMOKE_FIRE_DETECT
    algos.emplace_back(std::move(m_pSmokeFireAlgo));
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
    std::vector<std::pair<bool, std::shared_ptr<CAlgorithm>>> videoAlgoBindings = {
#if CAP_UNIFIED_DETECTION
        /* 统一检测引擎（MD/OD 模型）：开关与 manageAlgorithmInstances 中保持一致（已注册模型的开关之或） */
        {                       stAlgoConfig.nEnMotionDetect || stAlgoConfig.nEnOcclusionDetect,m_pDetectionEngine                                                                                                },
#else
        { stAlgoConfig.nEnMotionDetect, m_pMotionAlgo },
        { stAlgoConfig.nEnOcclusionDetect, m_pHideAlgo },
#endif
        {                                                           stAlgoConfig.nEnSceneChange, m_pSceneChangeAlgo },
        {                                                                                 stAlgoConfig.nEnFaceDetect || stAlgoConfig.nEnLoiteringDetect || stAlgoConfig.nEnParkingDetect || stAlgoConfig.nEnLineCrossing ||
                                                                                 stAlgoConfig.nEnIntrusion || stAlgoConfig.nEnEnterRegion || stAlgoConfig.nEnLeaveRegion
#if CAP_AI_PEOPLE_STATISTICS
                                                                                 || stAlgoConfig.nEnPeopleFlowStatistics
#endif
#if CAP_AI_PEOPLE_DENSITY_V2
                                                                                 || stAlgoConfig.nEnPeopleDensityDetection
#endif
          ,
         m_pHVFAlgo                                                                                                },
        {                                                                                 stAlgoConfig.nEnCrowdGathering
#if CAP_AI_PEOPLE_DENSITY_LEGACY
                                                                                 || stAlgoConfig.nEnPeopleDensityDetection
#endif
          ,
         m_pPeopleHeadAlgo                                                                                         },
        {                     stAlgoConfig.nEnUnattendedObject || stAlgoConfig.nEnObjectRemoval,        m_pItemAlgo },
        {                                                        stAlgoConfig.nEnPetRecognition,         m_pPetAlgo },
#if CAP_UNIFIED_DETECTION
        { stAlgoConfig.nEnFaceCapture || stAlgoConfig.nEnFaceLib || stAlgoConfig.nEnFaceCompare,      m_pFaceEngine },
#else
        { stAlgoConfig.nEnFaceCapture || stAlgoConfig.nEnFaceLib || stAlgoConfig.nEnFaceCompare, m_pFaceAlgo },
#endif
#if CAP_AI_GARBAGE_DETECT
        {                    stAlgoConfig.nEnGarbageExposure || stAlgoConfig.nEnGarbageOverflow,     m_pGarbageAlgo },
#endif
#if defined(SCENE_INTELLIGENCE) || CAP_AI_SMOKE_FIRE_DETECT
        {                                                             stAlgoConfig.nEnSmokeFire,   m_pSmokeFireAlgo },
#endif
    };

    for (const auto &videoBinding : videoAlgoBindings)
    {
        if (videoBinding.first && videoBinding.second)
        {
            dlog_debug("AI_APP: 绑定视频回调成功");
            bindVideoSlot<CAlgorithm>(m_StreamHandler.get(), videoBinding.second.get(), &CAlgorithm::recvMediaData);
        }
    }

    /* 音频 配置项和算法对象的映射 */
    std::vector<std::pair<bool, std::shared_ptr<CAlgorithm>>> audioAlgoBindings = {
        { stAlgoConfig.nEnAudioAnomaly, m_pAudioAlgo },
    };

    for (const auto &audioBinding : audioAlgoBindings)
    {
        if (audioBinding.first && audioBinding.second)
        {
            dlog_debug("AI_APP: 绑定音频回调成功");
            bindAudioSlot<CAlgorithm>(m_StreamHandler.get(), audioBinding.second.get(), &CAlgorithm::recvMediaData);
        }
    }
}

void CAlgoStreamDeal::manageAlgorithmInstances(const Event::AlgorithmConfig &stAlgoConfig)
{
    int bIsNew = 0;

#if CAP_UNIFIED_DETECTION
    /*
     * 统一检测引擎：承载 MD/OD 模型，替代 Legacy 单类实现。
     * 引擎开关 = 已注册模型的开关之或（未注册模型的开关不参与，避免空引擎收帧）；
     * register_model 必须在 start 前完成，manageSingleAlgorithm 无二次钩子，creator 内一并完成。
     */
    const bool bUnifiedEngineEnabled = stAlgoConfig.nEnMotionDetect || stAlgoConfig.nEnOcclusionDetect;
    bIsNew += manageSingleAlgorithm(m_pDetectionEngine,
                                    bUnifiedEngineEnabled,
                                    []() -> std::shared_ptr<CAlgorithm>
                                    {
                                        auto pEngine = std::make_shared<CUnifiedDetectionEngine>();
                                        pEngine->register_model(std::unique_ptr<CMotionDetectModel>(new CMotionDetectModel()));
                                        pEngine->register_model(std::unique_ptr<CHideDetectModel>(new CHideDetectModel()));
                                        pEngine->start();
                                        return std::static_pointer_cast<CAlgorithm>(pEngine);
                                    });
#endif
#if !CAP_UNIFIED_DETECTION
    /* 移动侦测算法（Legacy 路径，统一检测宏启用时编译剔除防双跑） */
    bIsNew += manageSingleAlgorithm(m_pMotionAlgo,
                                    stAlgoConfig.nEnMotionDetect,
                                    []() -> std::shared_ptr<CAlgorithm>
                                    {
                                        return std::static_pointer_cast<CAlgorithm>(std::make_shared<CMotionDetect>());
                                    });
    /* 遮挡侦测算法（Legacy 路径，统一检测宏启用时编译剔除防双跑） */
    bIsNew += manageSingleAlgorithm(m_pHideAlgo,
                                    stAlgoConfig.nEnOcclusionDetect,
                                    []()
                                    {
                                        return std::static_pointer_cast<CAlgorithm>(std::make_shared<CHideDetect>());
                                    });
#endif
    /* 音频异常侦测算法 */
    bIsNew += manageSingleAlgorithm(m_pAudioAlgo,
                                    stAlgoConfig.nEnAudioAnomaly,
                                    []()
                                    {
                                        return std::static_pointer_cast<CAlgorithm>(std::make_shared<CAudioDetect>());
                                    });
    /* 场景变更侦测算法 */
    bIsNew += manageSingleAlgorithm(m_pSceneChangeAlgo,
                                    stAlgoConfig.nEnSceneChange,
                                    []()
                                    {
                                        return std::static_pointer_cast<CAlgorithm>(std::make_shared<CSceneChangeDetect>());
                                    });
    /* 脸人车侦测算法 */
    bIsNew += manageSingleAlgorithm(m_pHVFAlgo,
                                    stAlgoConfig.nEnFaceDetect || stAlgoConfig.nEnLoiteringDetect || stAlgoConfig.nEnParkingDetect ||
                                        stAlgoConfig.nEnLineCrossing || stAlgoConfig.nEnIntrusion || stAlgoConfig.nEnEnterRegion ||
                                        stAlgoConfig.nEnLeaveRegion
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
    bIsNew += manageSingleAlgorithm(m_pPeopleHeadAlgo,
                                    stAlgoConfig.nEnCrowdGathering
#if CAP_AI_PEOPLE_DENSITY_LEGACY
                                        || stAlgoConfig.nEnPeopleDensityDetection
#endif
                                    ,
                                    []() -> std::shared_ptr<CAlgorithm>
                                    {
                                        return std::static_pointer_cast<CAlgorithm>(std::make_shared<CPeopleHeadDetect>());
                                    });
    /* 物品侦测算法 */
    bIsNew += manageSingleAlgorithm(m_pItemAlgo,
                                    stAlgoConfig.nEnUnattendedObject || stAlgoConfig.nEnObjectRemoval,
                                    []() -> std::shared_ptr<CAlgorithm>
                                    {
                                        return std::static_pointer_cast<CAlgorithm>(std::make_shared<CItemDetect>());
                                    });
    /* 宠物识别算法 */
    bIsNew += manageSingleAlgorithm(m_pPetAlgo,
                                    stAlgoConfig.nEnPetRecognition,
                                    []() -> std::shared_ptr<CAlgorithm>
                                    {
                                        return std::static_pointer_cast<CAlgorithm>(std::make_shared<CPetRecognition>());
                                    });
#if CAP_UNIFIED_DETECTION
    /*
     * 人脸统一检测引擎：独立实例承载（YOLO/ArcFace 推理耗时长，不与 MD/OD 共享 Worker）。
     * 名单库入库仍走 Legacy 临时实例路径（add_Facelib_Groups），不经此引擎。
     * 存活条件不含 nEnFaceLib：名单库开关（下发字段 EnFaceFea）建库后常开，计入会让
     * "关闭抓拍和比对"后引擎仍存活、双模型常驻不卸载（约15兆NPU内存不释放）。
     * 名单库自身不需要常驻引擎：入库用临时 worker，比对由 nEnFaceCompare 单独承载。
     */
    bIsNew += manageSingleAlgorithm(m_pFaceEngine,
                                    stAlgoConfig.nEnFaceCapture || stAlgoConfig.nEnFaceCompare,
                                    []() -> std::shared_ptr<CAlgorithm>
                                    {
                                        auto pEngine = std::make_shared<CUnifiedDetectionEngine>();
                                        pEngine->register_model(std::unique_ptr<CFaceDetectModel>(new CFaceDetectModel()));
                                        pEngine->start();
                                        return std::static_pointer_cast<CAlgorithm>(pEngine);
                                    });
#else
    /* 人脸检测算法（Legacy 路径，统一检测宏启用时编译剔除防双跑；名单库入库临时实例不受此开关影响） */
    bIsNew += manageSingleAlgorithm(m_pFaceAlgo,
                                    stAlgoConfig.nEnFaceCapture || stAlgoConfig.nEnFaceLib || stAlgoConfig.nEnFaceCompare,
                                    []() -> std::shared_ptr<CAlgorithm>
                                    {
                                        return std::static_pointer_cast<CAlgorithm>(std::make_shared<CFaceDetect>());
                                    });
#endif
#if CAP_AI_GARBAGE_DETECT
    /* 垃圾检测算法 */
    bIsNew += manageSingleAlgorithm(m_pGarbageAlgo,
                                    stAlgoConfig.nEnGarbageExposure || stAlgoConfig.nEnGarbageOverflow,
                                    []() -> std::shared_ptr<CAlgorithm>
                                    {
                                        return std::static_pointer_cast<CAlgorithm>(std::make_shared<CGarbageDetect>());
                                    });
#endif
#if defined(SCENE_INTELLIGENCE) || CAP_AI_SMOKE_FIRE_DETECT
    bIsNew += manageSingleAlgorithm(m_pSmokeFireAlgo,
                                    stAlgoConfig.nEnSmokeFire,
                                    []() -> std::shared_ptr<CAlgorithm>
                                    {
                                        return std::make_shared<CSmokeFireDetect>();
                                    });
#endif
    if (bIsNew)
    {
        /* 统一获取区域配置 */
        dlog_debug("AI_APP: 统一获取区域配置 bIsNew[%d]", bIsNew);
    }
}

bool CAlgoStreamDeal::manageSingleAlgorithm(std::shared_ptr<CAlgorithm> &algo,
                                            bool bEnabled,
                                            std::function<std::shared_ptr<CAlgorithm>()> creator)
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

    Event::AlgorithmConfig effectiveConfig = stAlgoConfig;
#if CAP_AI_SMOKE_FIRE_DETECT && CAP_AI_GARBAGE_DETECT
    /* 兼容升级前已保存的冲突状态：运行时以垃圾识别为优先。 */
    if (effectiveConfig.nEnSmokeFire && (effectiveConfig.nEnGarbageExposure || effectiveConfig.nEnGarbageOverflow))
    {
        dlog_error("AI_APP: 烟火识别与垃圾识别同时启用，暂停烟火识别");
        effectiveConfig.nEnSmokeFire = 0;
    }
#endif

    /* 取消所有绑定 */
    unbindVideoSig(m_StreamHandler.get());
    unbindAudioSig(m_StreamHandler.get());

    initEventStatisticsReporter();

    /* 根据配置管理算法实例 */
#if CAP_AI_SMOKE_FIRE_DETECT && CAP_AI_GARBAGE_DETECT
    /* 从烟火切换到垃圾时，先等待烟火线程退出并释放模型。 */
    if ((effectiveConfig.nEnGarbageExposure || effectiveConfig.nEnGarbageOverflow) && m_pSmokeFireAlgo)
    {
        m_pSmokeFireAlgo.reset();
    }
#endif
    manageAlgorithmInstances(effectiveConfig);

#if CAP_UNIFIED_DETECTION
    /*
     * 人脸出图 JPEG 编码通道随人脸算法全关释放：通道码流池约 4MB 常驻 MMZ，
     * 只在出图瞬间用到；任一抓拍/比对开启则保留（出图还要用，重建虽毫秒级
     * 但无必要）。名单库不入条件：入库不出图，只开名单库时通道也释放。
     */
    if (!effectiveConfig.nEnFaceCapture && !effectiveConfig.nEnFaceCompare)
    {
        AiAppCommon::CJpegEncodeService::instance()->releaseChannel();
    }
#endif

    /* 重新绑定回调函数 */
    bindRecvFunc(effectiveConfig);

    /* 通知 Algorithm 更新参数 */
    std::vector<std::shared_ptr<CAlgorithm>> algos = { m_pDetectionEngine,
                                                       m_pFaceEngine,
                                                       m_pMotionAlgo,
                                                       m_pHideAlgo,
                                                       m_pAudioAlgo,
                                                       m_pSceneChangeAlgo,
                                                       m_pHVFAlgo,
                                                       m_pPeopleHeadAlgo,
                                                       m_pItemAlgo,
                                                       m_pPetAlgo,
                                                       m_pFaceEngine,
                                                       m_pFaceAlgo
#if CAP_AI_GARBAGE_DETECT
                                                       ,
                                                       m_pGarbageAlgo
#endif
#if defined(SCENE_INTELLIGENCE) || CAP_AI_SMOKE_FIRE_DETECT
                                                       ,
                                                       m_pSmokeFireAlgo
#endif
    };

    for (auto &pAlgo : algos)
    {
        if (pAlgo)
        {
            pAlgo->setEventStatisticsReporter(m_pEventStatisticsReporter);
            pAlgo->setAlgoEnCfg(effectiveConfig);
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
    std::vector<std::shared_ptr<CAlgorithm>> algos = { m_pDetectionEngine,
                                                       m_pMotionAlgo,
                                                       m_pHideAlgo,
                                                       m_pAudioAlgo,
                                                       m_pSceneChangeAlgo,
                                                       m_pHVFAlgo,
                                                       m_pPeopleHeadAlgo,
                                                       m_pItemAlgo,
                                                       m_pPetAlgo,
                                                       m_pFaceEngine,
                                                       m_pFaceAlgo
#if CAP_AI_GARBAGE_DETECT
                                                       ,
                                                       m_pGarbageAlgo
#endif
#if defined(SCENE_INTELLIGENCE) || CAP_AI_SMOKE_FIRE_DETECT
                                                       ,
                                                       m_pSmokeFireAlgo
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
#if !CAP_UNIFIED_DETECTION
    /*
     * Legacy 常驻人脸实例预热：统一检测宏启用时人脸由引擎承载
     * （manageAlgorithmInstances 按开关创建），此处不再无条件创建，防双跑双模型驻留。
     */
    if (!m_pFaceAlgo)
    {

        m_pFaceAlgo = std::static_pointer_cast<CAlgorithm>(std::make_shared<CFaceDetect>());
    }
    else
    {
    }
#endif
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

#if CAP_UNIFIED_DETECTION
    /*
     * 统一检测层画像：改接名单库服务（library/face_library_service）。服务内部
     * 自建临时轻量 Worker（YOLO/ArcFace 低内存互斥模式）完成单图建模，
     * enrollFromImage 返回前即销毁临时模型，先于下方恢复常驻算法。
     */
    try
    {
        dlog_info("AI_APP: 名单库入库转入统一检测层名单库服务");
        FaceChain_NS::CFaceLibraryService stFaceLibService;
        nRet = stFaceLibService.enrollFromImage(stFaceLibData);
    }
    catch (...)
    {
        dlog_error("AI_APP: 名单库入库服务执行时发生异常，本次入库按失败处理");
        nRet = ERR_FACE_ALGO_CREATE_FAILED;
    }
#else
    /*
     * Legacy 画像（检测层未编译，如 TL4G/TLW）：保留临时 CFaceDetect 实例路径。
     * CFaceDetect::addFaceLibGroup 内部使用YOLO/ArcFace低内存互斥模式，任务
     * 完成后先销毁临时实例再恢复算法。
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
    catch (...)
    {
        dlog_error("AI_APP: 名单库临时人脸算法执行时发生异常");
        nRet = ERR_FACE_ALGO_CREATE_FAILED;
    }

    /* 必须先析构临时模型，再恢复其他算法，避免模型同时驻留。 */
    pFaceAlgo.reset();
#endif
    dlog_info("AI_APP: 名单库临时模型已释放，准备恢复原算法.");
    const int nRestoreRet = endFaceLibExclusive();
    if (nRet == FACE_LIB_ADD_OK && nRestoreRet != FACE_LIB_ADD_OK)
    {
        // dlog_error("AI_APP: 创建人脸算法实例失败，无法添加名单库成员");
        // return ERR;
        nRet = nRestoreRet;
    }
    else if (nRestoreRet != FACE_LIB_ADD_OK)
    {
        dlog_error("AI_APP: 人脸添加失败且原算法恢复失败, addRet[%d] restoreRet[%d]", nRet, nRestoreRet);
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

#if CAP_AI_GARBAGE_DETECT
void CAlgoStreamDeal::snapshot_garbage_detect(void *pData)
{
    Json::Object *pRootJson = Json::init();
    int nResult = -1;
    std::string strImagePath;

    dlog_info("垃圾站抓图识别-AI层: 收到抓图识别请求，开始处理");

    do
    {
        if (!pRootJson)
        {
            dlog_error("垃圾站抓图识别-AI层: 创建 JSON 失败");
            break;
        }

        if (!m_pGarbageAlgo)
        {
            dlog_error("垃圾站抓图识别-AI层: 垃圾算法实例未创建（垃圾暴露与垃圾满溢可能均未启用）");
            break;
        }

        std::shared_ptr<CGarbageDetect> pGarbageAlgo = std::dynamic_pointer_cast<CGarbageDetect>(m_pGarbageAlgo);
        if (!pGarbageAlgo)
        {
            dlog_error("垃圾站抓图识别-AI层: 算法实例类型转换失败");
            break;
        }

        /* 单帧检测：请求由算法流式线程串行执行，避免并发调用推理句柄。 */
        CGarbageDetect::SnapshotResult_S stSnapshot;
        if (!pGarbageAlgo->detectOnce(stSnapshot))
        {
            dlog_error("垃圾站抓图识别-AI层: 单帧检测失败（超时或算法未就绪）");
            break;
        }
        dlog_info("垃圾站抓图识别-AI层: 单帧检测成功，命中框[%d] 满溢[%d] 暴露[%d] JPEG[%d]字节",
                  static_cast<int>(stSnapshot.vstRectInfo.size()),
                  stSnapshot.bGarbageOverflow ? 1 : 0,
                  stSnapshot.bGarbageExposure ? 1 : 0,
                  static_cast<int>(stSnapshot.vecJpeg.size()));

        /* 快照触发时刻即事件触发时间戳：图片命名、上报时间与响应字段统一使用该值。 */
        const long long llEventTimestampMs = TimeUtils_NS::get_currentTimestampMs();
        const std::string strTimestamp = std::to_string(llEventTimestampMs);
        const std::string strDateDash = TimeUtils_NS::timestamp_to_date(llEventTimestampMs);
        const std::string strTimeColon = TimeUtils_NS::timestamp_to_time(llEventTimestampMs);
        const std::string strDateTimeDash = strDateDash + " " + strTimeColon;
        std::string strDateCompact;
        for (char ch : strDateDash)
        {
            if (ch != '-')
            {
                strDateCompact += ch;
            }
        }
        std::string strTimeCompactMs;
        for (char ch : strTimeColon)
        {
            if (ch != ':')
            {
                strTimeCompactMs += ch;
            }
        }
        /* 补足 3 位毫秒，与事件链路的时间片段格式一致。 */
        strTimeCompactMs += std::to_string(1000 + static_cast<int>(llEventTimestampMs % 1000)).substr(1);

        /* 本功能使用独立事件类型，与垃圾暴露/满溢的自动事件区分开。 */
        const Event::Type_E enEventType = Event::Type_E::GARBAGE_STATION_SNAPSHOT;

        /* 按命中的事件类型落盘，命名与入库与正常垃圾事件抓图一致。 */
        if (!stSnapshot.vecJpeg.empty())
        {
            const int nSaveRet = CCaptureCtrl::instance()->save_event_image(stSnapshot.vecJpeg.data(),
                                                                            static_cast<int>(stSnapshot.vecJpeg.size()),
                                                                            enEventType,
                                                                            strDateCompact,
                                                                            strTimeCompactMs,
                                                                            strImagePath);
            if (nSaveRet < 0)
            {
                dlog_error("垃圾站抓图识别-AI层: 图片落盘失败，事件类型[%d]", static_cast<int>(enEventType));
            }
            else
            {
                dlog_info("垃圾站抓图识别-AI层: 图片落盘成功，事件类型[%d] 大小[%d]字节 路径[%s]",
                          static_cast<int>(enEventType),
                          nSaveRet,
                          strImagePath.c_str());
            }
        }
        else
        {
            dlog_warn("垃圾站抓图识别-AI层: JPEG 数据为空，跳过落盘");
        }

        Json::add(pRootJson, "GarbageOverflow", stSnapshot.bGarbageOverflow ? 1 : 0);
        Json::add(pRootJson, "GarbageExposure", stSnapshot.bGarbageExposure ? 1 : 0);
        Json::add(pRootJson, "ImagePath", strImagePath);

        /* 以下字段与正常垃圾事件上报平台的格式保持一致（见 event_linkage_action_direct.cpp:130-152）。 */
        Json::add(pRootJson, "EventType", static_cast<int>(enEventType));
        Json::add(pRootJson, "EventStatus", 1);
        Json::add(pRootJson, "Channel", 0);
        Json::add(pRootJson, "Timestamp", strTimestamp);
        Json::add(pRootJson, "Date", strDateCompact);
        Json::add(pRootJson, "Time", strTimestamp);
        Json::add(pRootJson, "StartTime", strDateTimeDash);
        Json::add(pRootJson, "EndTime", strDateTimeDash);

        nResult = 0;
    }
    while (false);

    if (pRootJson)
    {
        Json::add(pRootJson, "Result", nResult);
        if (pData != nullptr)
        {
            *(static_cast<std::string *>(pData)) = Json::to_string(pRootJson);
        }
        Json::deinit(pRootJson);
    }
    else if (pData != nullptr)
    {
        *(static_cast<std::string *>(pData)) = "{\"Result\":-1}";
    }

    dlog_info("垃圾站抓图识别-AI层: 处理结束，结果码[%d] 图片路径[%s]", nResult, strImagePath.empty() ? "无" : strImagePath.c_str());
}
#endif

#if defined(SCENE_INTELLIGENCE) || CAP_AI_SMOKE_FIRE_DETECT
void CAlgoStreamDeal::snapshot_smoke_fire_detect(void *pData)
{
    Json::Object *pRootJson = Json::init();
    int nResult = -1;
    std::string strImagePath;

    dlog_info("烟火抓图识别-AI层: 收到抓图识别请求，开始处理");

    do
    {
        if (!pRootJson)
        {
            dlog_error("烟火抓图识别-AI层: 创建 JSON 失败");
            break;
        }

        if (!m_pSmokeFireAlgo)
        {
            dlog_error("烟火抓图识别-AI层: 烟火算法实例未创建（烟火识别可能未启用）");
            break;
        }

        std::shared_ptr<CSmokeFireDetect> pSmokeFireAlgo = std::dynamic_pointer_cast<CSmokeFireDetect>(m_pSmokeFireAlgo);
        if (!pSmokeFireAlgo)
        {
            dlog_error("烟火抓图识别-AI层: 算法实例类型转换失败");
            break;
        }

        /* 单帧检测：请求由算法流式线程串行执行，避免并发调用推理句柄。 */
        CSmokeFireDetect::SnapshotResult_S stSnapshot;
        if (!pSmokeFireAlgo->detectOnce(stSnapshot))
        {
            dlog_error("烟火抓图识别-AI层: 单帧检测失败（超时或算法未就绪）");
            break;
        }
        dlog_info("烟火抓图识别-AI层: 单帧检测成功，命中框[%d] 烟雾[%d] 火焰[%d] JPEG[%d]字节",
                  static_cast<int>(stSnapshot.vstRectInfo.size()),
                  stSnapshot.bSmokeDetected ? 1 : 0,
                  stSnapshot.bFireDetected ? 1 : 0,
                  static_cast<int>(stSnapshot.vecJpeg.size()));

        /* 快照触发时刻即事件触发时间戳：图片命名、上报时间与响应字段统一使用该值。 */
        const long long llEventTimestampMs = TimeUtils_NS::get_currentTimestampMs();
        const std::string strTimestamp = std::to_string(llEventTimestampMs);
        const std::string strDateDash = TimeUtils_NS::timestamp_to_date(llEventTimestampMs);
        const std::string strTimeColon = TimeUtils_NS::timestamp_to_time(llEventTimestampMs);
        const std::string strDateTimeDash = strDateDash + " " + strTimeColon;
        std::string strDateCompact;
        for (char ch : strDateDash)
        {
            if (ch != '-')
            {
                strDateCompact += ch;
            }
        }
        std::string strTimeCompactMs;
        for (char ch : strTimeColon)
        {
            if (ch != ':')
            {
                strTimeCompactMs += ch;
            }
        }
        /* 补足 3 位毫秒，与事件链路的时间片段格式一致。 */
        strTimeCompactMs += std::to_string(1000 + static_cast<int>(llEventTimestampMs % 1000)).substr(1);

        /* 复用烟火识别的既有事件类型，平台按 EventType=33 归类。 */
        const Event::Type_E enEventType = Event::Type_E::SMOKE_FIRE;

        /* 按事件类型落盘，命名与入库与正常烟火事件抓图一致。 */
        if (!stSnapshot.vecJpeg.empty())
        {
            const int nSaveRet = CCaptureCtrl::instance()->save_event_image(stSnapshot.vecJpeg.data(),
                                                                            static_cast<int>(stSnapshot.vecJpeg.size()),
                                                                            enEventType,
                                                                            strDateCompact,
                                                                            strTimeCompactMs,
                                                                            strImagePath);
            if (nSaveRet < 0)
            {
                dlog_error("烟火抓图识别-AI层: 图片落盘失败，事件类型[%d]", static_cast<int>(enEventType));
            }
            else
            {
                dlog_info("烟火抓图识别-AI层: 图片落盘成功，事件类型[%d] 大小[%d]字节 路径[%s]",
                          static_cast<int>(enEventType),
                          nSaveRet,
                          strImagePath.c_str());
            }
        }
        else
        {
            dlog_warn("烟火抓图识别-AI层: JPEG 数据为空，跳过落盘");
        }

        Json::add(pRootJson, "SmokeDetected", stSnapshot.bSmokeDetected ? 1 : 0);
        Json::add(pRootJson, "FireDetected", stSnapshot.bFireDetected ? 1 : 0);
        Json::add(pRootJson, "ImagePath", strImagePath);

        /* 以下字段与正常事件上报平台的格式保持一致（见 event_linkage_action_direct.cpp:130-152）。 */
        Json::add(pRootJson, "EventType", static_cast<int>(enEventType));
        Json::add(pRootJson, "EventStatus", 1);
        Json::add(pRootJson, "Channel", 0);
        Json::add(pRootJson, "Timestamp", strTimestamp);
        Json::add(pRootJson, "Date", strDateCompact);
        Json::add(pRootJson, "Time", strTimestamp);
        Json::add(pRootJson, "StartTime", strDateTimeDash);
        Json::add(pRootJson, "EndTime", strDateTimeDash);

        nResult = 0;
    }
    while (false);

    if (pRootJson)
    {
        Json::add(pRootJson, "Result", nResult);
        if (pData != nullptr)
        {
            *(static_cast<std::string *>(pData)) = Json::to_string(pRootJson);
        }
        Json::deinit(pRootJson);
    }
    else if (pData != nullptr)
    {
        *(static_cast<std::string *>(pData)) = "{\"Result\":-1}";
    }

    dlog_info("烟火抓图识别-AI层: 处理结束，结果码[%d] 图片路径[%s]", nResult, strImagePath.empty() ? "无" : strImagePath.c_str());
}
#endif