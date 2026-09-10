/**
 * @FilePath     : result_dispatcher.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-08-24 13:30:00
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-08-24 13:30:00
 * @Description  : 同步结果分发器实现
 */

#include "result_dispatcher.hpp"

#include <algorithm>

#include "IpcRet.h"
#include "dlog.h"

namespace AiPipeline_NS
{
int CResultDispatcher::register_processor(std::unique_ptr<IResultProcessor> pProcessor)
{
    if (pProcessor == nullptr)
    {
        dlog_warn("AI Pipeline 注册处理器失败：处理器为空");
        return ERR_PARAM_NULL;
    }

    m_vecProcessors.emplace_back(std::move(pProcessor));
    /* 按 nOrder 升序排序，保证初始化后执行顺序固定 */
    std::sort(m_vecProcessors.begin(),
              m_vecProcessors.end(),
              [](const std::unique_ptr<IResultProcessor> &pLeft, const std::unique_ptr<IResultProcessor> &pRight)
              {
                  return pLeft->descriptor().nOrder < pRight->descriptor().nOrder;
              });
    m_bSorted = true;
    return OK;
}

int CResultDispatcher::dispatch(const DetectionBatch_S &stBatch, ProcessorOutput_S &stOutput)
{
    if (stBatch.stMetadata.stNativeResultSize.nWidth == 0U || stBatch.stMetadata.stNativeResultSize.nHeight == 0U)
    {
        dlog_warn("AI Pipeline 分发失败：原生结果坐标分辨率非法 [%ux%u]",
                  stBatch.stMetadata.stNativeResultSize.nWidth,
                  stBatch.stMetadata.stNativeResultSize.nHeight);
        return ERR_PARAM;
    }

    for (const auto &pProcessor : m_vecProcessors)
    {
        const ProcessorDescriptor_S &stDescriptor = pProcessor->descriptor();

        /* 能力筛选：批次能力必须覆盖处理器必需能力 */
        if (!capability::covers(stBatch.stMetadata.unCapabilities, stDescriptor.unRequiredCapabilities))
        {
            continue;
        }

        /* 目标类型筛选：批次存在可接受类型才分发，空掩码表示不限制 */
        if (stDescriptor.unAcceptedObjectMask != 0U)
        {
            bool bAcceptable = false;
            for (const auto &stObject : stBatch.vecObjects)
            {
                const uint32_t unObjectBit = 1U << static_cast<uint32_t>(stObject.enType);
                if ((stDescriptor.unAcceptedObjectMask & unObjectBit) != 0U)
                {
                    bAcceptable = true;
                    break;
                }
            }
            if (!bAcceptable)
            {
                continue;
            }
        }

        /* 事务输出：先写入临时输出，成功才 move-merge，失败丢弃半成品 */
        ProcessorOutput_S stPendingOutput;
        const int nRet = pProcessor->process(stBatch, stPendingOutput);
        if (nRet != OK)
        {
            dlog_warn("AI Pipeline 处理器[%s]处理失败，输出丢弃，错误码[%d]",
                      stDescriptor.pName != nullptr ? stDescriptor.pName : "unknown",
                      nRet);
            continue;
        }

        stOutput.vecStatisticsDrafts.insert(stOutput.vecStatisticsDrafts.end(),
                                            std::make_move_iterator(stPendingOutput.vecStatisticsDrafts.begin()),
                                            std::make_move_iterator(stPendingOutput.vecStatisticsDrafts.end()));
        stOutput.vecEventConditions.insert(stOutput.vecEventConditions.end(),
                                           std::make_move_iterator(stPendingOutput.vecEventConditions.begin()),
                                           std::make_move_iterator(stPendingOutput.vecEventConditions.end()));
        stOutput.vecOverlayItems.insert(stOutput.vecOverlayItems.end(),
                                        std::make_move_iterator(stPendingOutput.vecOverlayItems.begin()),
                                        std::make_move_iterator(stPendingOutput.vecOverlayItems.end()));
        stOutput.vecImageRequests.insert(stOutput.vecImageRequests.end(),
                                         std::make_move_iterator(stPendingOutput.vecImageRequests.begin()),
                                         std::make_move_iterator(stPendingOutput.vecImageRequests.end()));
    }

    return OK;
}

size_t CResultDispatcher::processor_count() const
{
    return m_vecProcessors.size();
}

void CResultDispatcher::reset()
{
    m_vecProcessors.clear();
    m_bSorted = false;
}
} // namespace AiPipeline_NS
