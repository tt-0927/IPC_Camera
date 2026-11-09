/**
 * @FilePath     : algo_stream_deal.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2025-06-06 16:02:10
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2025-08-15 16:58:55
 * @Description  : 负责取流传给算法检测
 */

#include "algo_stream_deal.h"
/* 课堂 AI 分析引擎与配置下发接口共用同一功能宏，未定义该宏的型号不装配该算法 */
#if defined(CAP_TV3881TJY_CLASSROOM_CONFIG) && CAP_TV3881TJY_CLASSROOM_CONFIG
#include "classroom_gateway.h"
#endif

int CAlgoStreamDeal::init()
{
    /*初始化算法句柄*/
    runOnce();
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
#if defined(CAP_TV3881TJY_CLASSROOM_CONFIG) && CAP_TV3881TJY_CLASSROOM_CONFIG
    algos.emplace_back(std::move(m_pClassroomAlgo));
#endif

    /* 释放算法实例 */
    for (auto &pAlgo : algos)
    {
        pAlgo.reset();
        dlog_debug("AI_APP: 释放算法实例 引用计数为[%d]", pAlgo.use_count());
    }

    /* 释放 StreamHandler */
    m_StreamHandler.reset();
}

void CAlgoStreamDeal::deal_message(const void *pData, int nLength, int nH, int nW, uint64_t u64PTS)
{
    if (pData != nullptr)
    {
        if (m_StreamHandler)
        {
            m_StreamHandler.get()->recvDataProcess(pData, nLength, nH, nW, u64PTS);
        }
        else
        {
            dlog_error("m_StreamHandler is null");
        }
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
        if (m_StreamHandler)
        {
            m_StreamHandler->recvDataProcess(pData, nLength);
        }
        else
        {
            dlog_error("m_StreamHandler is null");
        }
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

    /* 配置项和算法对象的映射 */
    std::vector<std::pair<bool, std::shared_ptr<CAlgorithm>>> algoBindings =
        {
            /* 课堂分析（仅定义课堂配置宏的型号装配） */
#if defined(CAP_TV3881TJY_CLASSROOM_CONFIG) && CAP_TV3881TJY_CLASSROOM_CONFIG
            {true, m_pClassroomAlgo},
#endif
        };

    for (const auto &binding : algoBindings)
    {
        if (binding.first && binding.second)
        {
            dlog_debug("AI_APP: 绑定视频回调成功");
            bindVideoSlot<CAlgorithm>(m_StreamHandler.get(), binding.second.get(), &CAlgorithm::recvMediaData);
        }
    }

}

void CAlgoStreamDeal::manageAlgorithmInstances(const Event::AlgorithmConfig &stAlgoConfig)
{
    int bIsNew = 0;

    /* 课堂分析算法（与配置下发接口同一宏门控） */
#if defined(CAP_TV3881TJY_CLASSROOM_CONFIG) && CAP_TV3881TJY_CLASSROOM_CONFIG
    bIsNew += manageSingleAlgorithm(m_pClassroomAlgo, true, [this]() {
        return std::static_pointer_cast<CAlgorithm>(std::make_shared<Classroom::CClassroomGateway>());
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
    printAlgoCfg(stAlgoConfig);

    /* 取消所有绑定 */
    unbindVideoSig(m_StreamHandler.get());
    unbindAudioSig(m_StreamHandler.get());

    /* 根据配置管理算法实例 */
    manageAlgorithmInstances(stAlgoConfig);

    /* 重新绑定回调函数 */
    bindRecvFunc(stAlgoConfig);

    /* 通知 Algorithm 更新参数 */
    std::vector<std::shared_ptr<CAlgorithm>> algos = {
#if defined(CAP_TV3881TJY_CLASSROOM_CONFIG) && CAP_TV3881TJY_CLASSROOM_CONFIG
        m_pClassroomAlgo,
#endif
    };

    for (auto &pAlgo : algos)
    {
        if (pAlgo)
        {
            pAlgo->setAlgoEnCfg(stAlgoConfig);
        }
    }
}

float CAlgoStreamDeal::getCurrentDb() const
{

    return 0.0f;
}

void CAlgoStreamDeal::runOnce()
{
    /* 用一张静态标记保证算法实例只装配一次：本函数被 init() 调用 */
    static bool s_bHasRun = false;

    /* 如果已执行过，直接返回 */
    if (s_bHasRun)
    {
        return;
    }

    s_bHasRun = true;

    Event::AlgorithmConfig stAlgoConfig;
    CEventConfigure::instance()->get_configure(stAlgoConfig);
    manageAlgorithmInstances(stAlgoConfig);
}
