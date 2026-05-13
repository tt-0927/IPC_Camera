/**
 * @FilePath     : ai_detect.cpp
 * @Author       : zhouzirui
 * @Date         : 2025-05-16 09:54:03
 * @LastEditors  : zhouzirui
 * @LastEditTime : 2025-05-20 10:24:43
 * @Description  : AI检测结果分析模块
 */

#include "ai_detect.h"

CAiDetect::CAiDetect()
{
    m_bResultsFlag.store(false, std::memory_order_release);
}

CAiDetect::~CAiDetect()
{
    m_bResultsFlag.store(false, std::memory_order_release);
}

int CAiDetect::init()
{
    m_bResultsFlag.store(true, std::memory_order_release);
    m_resultsThread = std::thread(&CAiDetect::results_analysis_thr, this);
    dlog_info("AI检测结果分析模块初始化成功");
    return OK;
}

int CAiDetect::deinit()
{
    m_bResultsFlag.store(false, std::memory_order_release);
    m_resultsThread.join();
    dlog_info("AI检测结果分析模块去初始化成功");
    return OK;
}

void CAiDetect::set_DetectConfig(AiDetectConfig_S &stConfig)
{
    m_stAiDetectConfig = stConfig;
}

void CAiDetect::send_results(ot_aidetect_result_array &stResult)
{
    ot_aidetect_result_array stResultCopy;
    if (deepCopyResult(&stResultCopy, &stResult) != 0)
    {
        dlog_error("拷贝失败");
        return;
    }
    m_AiResultQueue.push(stResultCopy);
}


int CAiDetect::deepCopyResult(ot_aidetect_result_array *pDst, const ot_aidetect_result_array *pSrc)
{
    if (pDst == nullptr || pSrc == nullptr)
    {
        return ERR;
    }

    memset(pDst, 0, sizeof(ot_aidetect_result_array));
    pDst->class_num = pSrc->class_num;

    for (td_u32 i = 0; i < pSrc->class_num && i < OT_AIDETECT_CLASS_BUTT; ++i)
    {
        const ot_aidetect_object_of_one_class &srcClass = pSrc->object_class[i];
        ot_aidetect_object_of_one_class &dstClass = pDst->object_class[i];

        dstClass.class_type = srcClass.class_type;
        dstClass.object_num = srcClass.object_num;
        dstClass.object_capacity = srcClass.object_capacity;

        if (srcClass.objects != nullptr && srcClass.object_num > 0)
        {
            dstClass.objects = (ot_aidetect_object *)malloc(sizeof(ot_aidetect_object) * dstClass.object_capacity);
            if (dstClass.objects == nullptr)
            {
                // 失败，释放已分配内存
                for (td_u32 j = 0; j < i; ++j)
                {
                    if (pDst->object_class[j].objects)
                    {
                        free(pDst->object_class[j].objects);
                        pDst->object_class[j].objects = nullptr;
                    }
                }
                return ERR;
            }
            memcpy(dstClass.objects, srcClass.objects, sizeof(ot_aidetect_object) * dstClass.object_num);
        }
        else
        {
            dstClass.objects = nullptr;
        }
    }

    return OK;
}

void CAiDetect::freeResult(ot_aidetect_result_array &stResult)
{
    for (int i = 0; i < stResult.class_num; ++i)
    {
        if (stResult.object_class[i].objects != TD_NULL)
        {
            free(stResult.object_class[i].objects);
            stResult.object_class[i].objects = TD_NULL;
        }
    }
}

bool CAiDetect::is_in_intrusion_region(const ot_rect *target_rect)
{
    /*目标矩形的中心点*/
    uint32_t center_x = target_rect->x + target_rect->width / 2;
    uint32_t center_y = target_rect->y + target_rect->height / 2;

    /*检查中心点是否在入侵区域内*/
    if (center_x >= m_stAiDetectConfig.stIntrusionConfig.stRegion.x &&
        center_x < (m_stAiDetectConfig.stIntrusionConfig.stRegion.x + m_stAiDetectConfig.stIntrusionConfig.stRegion.width) &&
        center_y >= m_stAiDetectConfig.stIntrusionConfig.stRegion.y &&
        center_y < (m_stAiDetectConfig.stIntrusionConfig.stRegion.y + m_stAiDetectConfig.stIntrusionConfig.stRegion.height))
    {
        return true;
    }

    return false;
}

void CAiDetect::process_intrusion_alarm(uint32_t track_id, ot_aidetect_class class_type, const ot_rect *rect)
{
    mpi_ai_detect_log("*** 区域入侵报警 *** 检测类型: %s, 跟踪ID: %u, 位置: [%u,%u,%u,%u]",
                      m_aClassTypes[class_type],
                      track_id,
                      rect->x, rect->y, rect->width, rect->height);

    // TODO: 在此调用事件上报接口
    // Event::TargetInfo_S stTargetInfo;
    // CEventManage::instance()->handleEventLink(Event::Type::INTRUSION, stTargetInfo);
}

void CAiDetect::results_analysis(ot_aidetect_result_array &stResult)
{
    uint32_t i = 0, j = 0;
    double current_time = get_time_ms();

    // mpi_ai_detect_log("class_num:%d",stResult.class_num);
    for (i = 0; i < stResult.class_num; ++i)
    {
        /*仅对人脸和人形进行入侵检测*/
        if (stResult.object_class[i].class_type != OT_AIDETECT_CLASS_FACE &&
            stResult.object_class[i].class_type != OT_AIDETECT_CLASS_HUMAN)
        {
            // 对于其他类型，仅打印检测信息
            for (j = 0; j < stResult.object_class[i].object_num; j++)
            {
                if (stResult.object_class[i].objects[j].track_status == OT_AIDETECT_TRACK_STATUS_DIE)
                {
                    mpi_ai_detect_log("检测类型: %s, 跟踪ID :%u 断开跟踪,坐标:[%u,%u,%u,%u]",
                                      m_aClassTypes[stResult.object_class[i].class_type],
                                      stResult.object_class[i].objects[j].track_id,
                                      stResult.object_class[i].objects[j].detect_rect.x,
                                      stResult.object_class[i].objects[j].detect_rect.y,
                                      stResult.object_class[i].objects[j].detect_rect.width,
                                      stResult.object_class[i].objects[j].detect_rect.height);
                }
                else
                {
                    mpi_ai_detect_log("{检测类型: %s, 坐标[%u,%u,%u,%u], 跟踪ID: %u,跟踪状态: %s[%d], 置信度(0,1): %f}",
                                      m_aClassTypes[stResult.object_class[i].class_type],
                                      stResult.object_class[i].objects[j].detect_rect.x,
                                      stResult.object_class[i].objects[j].detect_rect.y,
                                      stResult.object_class[i].objects[j].detect_rect.width,
                                      stResult.object_class[i].objects[j].detect_rect.height,
                                      stResult.object_class[i].objects[j].track_id,
                                      m_aTrackStatus[stResult.object_class[i].objects[j].track_status],
                                      stResult.object_class[i].objects[j].track_status,
                                      stResult.object_class[i].objects[j].detect_confidence);
                }
            }
            continue;
        }

        // mpi_ai_detect_log("object_num:%d",stResult.object_class[i].object_num);
        for (j = 0; j < stResult.object_class[i].object_num; j++)
        {
            uint32_t track_id = stResult.object_class[i].objects[j].track_id;
            // IntrusionStatus_S stIntrusionStatus;
            // m_vstTargetStatus.push_back(stIntrusionStatus);
            /*确保track_id在有效范围内*/
            // if (track_id >= MAX_TRACK_ID)
            // {
            //     mpi_ai_detect_log("警告：跟踪ID %u 超出范围", track_id);
            //     continue;
            // }

            /*当前目标断开跟踪*/
            if (stResult.object_class[i].objects[j].track_status == OT_AIDETECT_TRACK_STATUS_DIE)
            {
                /*目标断开跟踪，重置该ID的状态*/
                stIntrusionStatus[track_id].bIsInRegion = false;
                stIntrusionStatus[track_id].dEnterTime = 0;
                stIntrusionStatus[track_id].bAlarmed = false;

                // mpi_ai_detect_log("检测类型: %s, 跟踪ID :%u 断开跟踪,坐标:[%u,%u,%u,%u]",
                //         m_aClassTypes[stResult.object_class[i].class_type], track_id,
                //         stResult.object_class[i].objects[j].detect_rect.x, stResult.object_class[i].objects[j].detect_rect.y,
                //         stResult.object_class[i].objects[j].detect_rect.width,
                //         stResult.object_class[i].objects[j].detect_rect.height);
                continue;
            }

            /*常规日志输出*/
            // mpi_ai_detect_log("{检测类型: %s, 坐标[%u,%u,%u,%u], 跟踪ID: %u,跟踪状态: %s[%d], 置信度(0,1): %f}",
            //         m_aClassTypes[stResult.object_class[i].class_type],
            //         stResult.object_class[i].objects[j].detect_rect.x,
            //         stResult.object_class[i].objects[j].detect_rect.y,
            //         stResult.object_class[i].objects[j].detect_rect.width,
            //         stResult.object_class[i].objects[j].detect_rect.height, track_id,
            //         m_aTrackStatus[stResult.object_class[i].objects[j].track_status],
            //         stResult.object_class[i].objects[j].track_status,
            //         stResult.object_class[i].objects[j].detect_confidence);

            /*区域入侵检测逻辑*/
            bool current_in_region = is_in_intrusion_region(&stResult.object_class[i].objects[j].detect_rect);

            /*目标刚进入区域*/
            if (current_in_region && !stIntrusionStatus[track_id].bIsInRegion)
            {
                stIntrusionStatus[track_id].bIsInRegion = true;
                stIntrusionStatus[track_id].dEnterTime = current_time;
                mpi_ai_detect_log("目标 ID: %u 进入入侵区域", track_id);
            }
            /*目标离开区域*/
            else if (!current_in_region && stIntrusionStatus[track_id].bIsInRegion)
            {
                stIntrusionStatus[track_id].bIsInRegion = false;
                stIntrusionStatus[track_id].dEnterTime = 0;
                stIntrusionStatus[track_id].bAlarmed = false;
                mpi_ai_detect_log("目标 ID: %u 离开入侵区域", track_id);
            }
            /*目标持续在区域内*/
            else if (current_in_region && stIntrusionStatus[track_id].bIsInRegion)
            {
                /*计算停留时间*/
                double stay_time_ms = current_time - stIntrusionStatus[track_id].dEnterTime;
                uint32_t stay_time_sec = (uint32_t)(stay_time_ms / 1000);

                /*如果停留时间超过阈值且尚未报警，则触发报警*/
                if (stay_time_sec >= m_stAiDetectConfig.stIntrusionConfig.unSec && !stIntrusionStatus[track_id].bAlarmed)
                {
                    stIntrusionStatus[track_id].bAlarmed = true;
                    process_intrusion_alarm(track_id, stResult.object_class[i].class_type,
                                            &stResult.object_class[i].objects[j].detect_rect);
                }
            }
        }
    }
}

// info /*----------------------- 私有线程函数 -----------------------*/

void CAiDetect::results_analysis_thr()
{
    pthread_setname_np(pthread_self(), "results_analysis_thr");
    try
    {
        while (true == m_bResultsFlag.load(std::memory_order_acquire))
        {
            if (m_AiResultQueue.empty())
            {
                usleep(1);
                continue;
            }
            ot_aidetect_result_array stResult = m_AiResultQueue.front();
            m_AiResultQueue.pop();
            /*送AI检测结果分析*/
            results_analysis(stResult);
            /*释放AI检测结果结构体*/
            freeResult(stResult);
        }
    }
    catch (const std::exception &e)
    {
        dlog_error("%s", e.what());
    }
}