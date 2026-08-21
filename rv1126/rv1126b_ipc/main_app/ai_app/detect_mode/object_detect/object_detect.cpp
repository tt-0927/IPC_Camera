/**
 * @file object_detect.cpp
 * @author tianl (tianl@kfb.cn)
 * @date 2025-11-12
 * 
 * @brief 物品检测
 */
#include "object_detect.hpp"
#include "algo_stream_deal.h"
#include "StatisticsTimer.hpp"
#include "SaveImage.hpp"

/* 数据队列 */
#define QUEUE_MAX (2)

CObjectDetect::CObjectDetect()
    : m_dateQueue(QUEUE_MAX)
{
    m_bRunning.store(true);
    m_thread = std::thread(&CObjectDetect::run, this);
}

CObjectDetect::~CObjectDetect()
{
    /* 通知线程停止 */
    m_bRunning.store(false);
    m_condition.notify_all();
    MediaData_S stMediaData;
    m_dateQueue.pushOrReplace(stMediaData);
    if (m_thread.joinable())
    {
        m_thread.join();
    }
    unInit();
}

void ObjectDetectPrintArea(const std::vector<::Event::Point_S>& area) 
{
    // 打印区域包含的点数量
    std::cout << "当前区域包含 " << area.size() << " 个点：" << std::endl;
    
    // 遍历区域内的每个点
    for (size_t pointIdx = 0; pointIdx < area.size(); ++pointIdx) {
        const ::Event::Point_S& point = area[pointIdx];  // 获取当前点（注意命名空间::Event::）
        // 打印点的索引和坐标（nX 为x坐标，nY 为y坐标）
        std::cout << "  点 " << pointIdx << "：(nX=" << point.nX << ", nY=" << point.nY << ")" << std::endl;
    }
    std::cout << "-------------------------" << std::endl;
}

/* 接受媒体数据 */
void CObjectDetect::recvMediaData(MediaData_S stMediaData)
{
    if (!m_stAlgoUnattendedObjectCfg.bEnable && !m_stAlgoObjectRemovalCfg.bEnable)
    {
        dlog_debug("ai_app: 物品检测-开关未启用");
        return;
    }
	 m_nChannelId = stMediaData.stMediaParam.nChannel;
    if (m_RecvManager.handleEvent(stMediaData.stMediaParam.nChannel))
    {
        if (m_dateQueue.size() >= QUEUE_MAX)
        {
            dlog_error("物品检测-数据队列满了 [%d]" ,m_dateQueue.size());
        }
        m_dateQueue.pushOrReplace(stMediaData);
    }
}


/**
 * @brief 更新算法配置参数
 * @param stAlgoConfig 
 */
void CObjectDetect::setAlgoEnCfg(const Event::AlgorithmConfig &stAlgoConfig)
{
	std::lock_guard<std::mutex> lock(m_mutex);

    m_stAlgoUnattendedObjectCfg.bEnable = stAlgoConfig.nEnUnattendedObject;
    m_stAlgoObjectRemovalCfg.bEnable = stAlgoConfig.nEnObjectRemoval;
	m_bOnlyRemoval = (!m_stAlgoUnattendedObjectCfg.bEnable && m_stAlgoObjectRemovalCfg.bEnable);
	/* 规则清除 */
	if(m_stAlgoUnattendedObjectCfg.bEnable || m_stAlgoObjectRemovalCfg.bEnable)
	{
		m_vstRuleInfo.clear();
		m_lastHighS1TimeMap.clear();
		m_eventTriggeredMap.clear();
		m_lastEndTimeMap.clear();
		m_eventTriggerStartMap.clear();
		m_removalHighStartMap.clear();
		m_removalHighDropTime.clear();
		m_lastLowTimeMap.clear();
		m_highSustainedStartMap.clear();
		m_freezeBgUntilMap.clear();
		m_highS1RateMap.clear();
		m_lastItemState = NONE;  /* 重置状态机，避免上次测试的状态影响本次 */
	}

	if(m_stAlgoUnattendedObjectCfg.bEnable)
	{
		Alarm::UnattendedObject_S stInfo;
        CEventConfigure::instance()->get_configure(stInfo);
        setAlgoParamCfg(stInfo);
	}
	if(m_stAlgoObjectRemovalCfg.bEnable)
	{
		Alarm::ObjectRemoval_S stInfo;
        CEventConfigure::instance()->get_configure(stInfo);
        setAlgoParamCfg(stInfo);
	}
  
}

void CObjectDetect::setAlgoParamCfg(const Alarm::UnattendedObject_S &stAlgoCfg,Event::Type_E enType)
{
    dlog_debug("ai_app: 设置物品遗留检测参数");
    m_stAlgoUnattendedObjectCfg = stAlgoCfg;
    m_lastItemState = NONE;
    m_lastBgUpdateTime -= std::chrono::seconds(10);
    
    if (!m_stAlgoUnattendedObjectCfg.aRule.empty())
    {
        m_nLeaveTimeThrd = m_stAlgoUnattendedObjectCfg.aRule[0].nTimeThreshold * 1000;

        m_fSensiThrd = 13 - m_stAlgoUnattendedObjectCfg.aRule[0].nSensitivity/10;
    }
	convertResolutionAndEnable(m_stAlgoUnattendedObjectCfg,enType); 
}

void CObjectDetect::setAlgoParamCfg(const Alarm::ObjectRemoval_S &stAlgoCfg,Event::Type_E enType)
{
    dlog_debug("ai_app: 设置物品拿取检测参数");
    m_stAlgoObjectRemovalCfg = stAlgoCfg;
    m_lastItemState = NONE;
    m_lastBgUpdateTime -= std::chrono::seconds(10);

    if (!m_stAlgoObjectRemovalCfg.aRule.empty())
    {
        m_nPickupTimeThrd = m_stAlgoObjectRemovalCfg.aRule[0].nTimeThreshold * 1000;

        m_fSensiThrd = 13 - m_stAlgoObjectRemovalCfg.aRule[0].nSensitivity/10;
    }
	convertResolutionAndEnable(m_stAlgoObjectRemovalCfg,enType); 
}


/* 绘制区域和线条到给定的图像 */
void CObjectDetect::drawRulesToImage(cv::Mat& inMat)
{
    for (const auto& rule : m_vstRuleInfo)
    {
        
        /* 绘制所有的区域(四边形边框) */
        for (const auto& area : rule.areas)
        {
            std::vector<cv::Point> polygon;
            for (const auto& point : area)
            {
                polygon.emplace_back(point.nX, point.nY);
            }
            cv::polylines(inMat, 
                          polygon, 
                          true,
                          cv::Scalar(0, 255, 0), /* 边框颜色(绿色) */
                          2,
                          cv::LINE_AA);
        }
    }
}


/* 初始化 */
bool CObjectDetect::init()
{
    /* 初始化无需额外操作 */
    dlog_info("物品检测算法初始化成功");
    return true;
}


bool CObjectDetect::unInit()
{
    /* 反初始化无需额外操作 */
    return true;
}


/* 线程函数 */
void CObjectDetect::run()
{
   MediaData_S      stMediaData;

    /* 记录检测区域的时间累计 */
    std::map<int, std::chrono::steady_clock::time_point> lastStayTimeMap;   /* 物品遗留时间 */
    std::map<int, std::chrono::steady_clock::time_point> lastRemoveTimeMap; /* 物品拿取时间 */

    /* 预分配可复用的Mat对象 */
    cv::Mat currentFrame;
    cv::Mat grayPrev, grayCurrent;
    cv::Mat diff, mask, maskedDiff;
    std::vector<cv::Point> polygon;

    while (m_bRunning.load())
    {
        std::vector<int> triggeredEvents;
        
        /* 阻塞获取 */
        m_dateQueue.pop(stMediaData, -1);
        if (m_vstRuleInfo.empty() || stMediaData.nSize == 0)
        {
            /* 区域规则为空 */
			if (access("/testPrint", F_OK) == 0)
			{
				dlog_debug("========区域规则为空==========");
			}
            continue;
        } /* 区域规则为空 */
		if (access("/testPrint", F_OK) == 0)
		{
			dlog_debug("========区域规则不为空==========");
		}
        CStatisticsTimer runTime("物品检测完整耗时");

        cv::Mat i420Mat(
            stMediaData.stMediaParam.nVideoHeight * 3/2,
            stMediaData.stMediaParam.nVideoWidth,
            CV_8UC1,
            stMediaData.pData.get()
        );

        // /* 转换为RGB */  
        cv::Mat rgbMat; cv::cvtColor(i420Mat, rgbMat, cv::COLOR_YUV2BGR_I420);
        m_lastRgbFrame = rgbMat.clone();
		// cv::rotate(rgbMat, rgbMat, cv::ROTATE_180);
        
        /* 送分析 */
        if (1)
        {
            frameRate("物品检测-分析数据", 5);

            currentFrame = rgbMat.clone();
            
            if (!currentFrame.empty())
            {
                /* 分析数据 */
                {
                    //CStatisticsTimer runTime("物品检测分析耗时");
                    std::unique_lock<std::mutex> lock(m_mutex);
                    if (m_backgroundFrame.empty())
                    {
                        m_backgroundFrame = currentFrame.clone();
                        continue;
                    }
                    
                    /* 转换为灰度图像 */
                    cv::cvtColor(m_backgroundFrame, grayPrev, cv::COLOR_BGR2GRAY);
                    cv::cvtColor(currentFrame, grayCurrent, cv::COLOR_BGR2GRAY);
                    
                    /* 计算帧差 */
                    cv::absdiff(grayPrev, grayCurrent, diff);

                    /* 二值化处理 */
                    cv::threshold(diff, diff, 30, 255, cv::THRESH_BINARY);

                    /* 高斯模糊减少噪声影响 */
                    cv::GaussianBlur(diff, diff, cv::Size(3, 3), 0.5);

                    /* 计算全图变化像素值的总和ST */
                    double ST = cv::sum(diff)[0];

                    /* 状态判断：是否有区域处于监测中 */
                    auto now = std::chrono::steady_clock::now();
                    auto timeSinceLastUpdate = std::chrono::duration_cast<std::chrono::seconds>(now - m_lastBgUpdateTime).count();
                    
                    /* ObjectRemoval 跟踪时短暂冻结背景防止吸收，15 秒自动过期避免死锁 */
                    bool freezeBg = false;
                    for (size_t j = 0; j < m_vstRuleInfo.size(); j++)
                    {
                        if (m_vstRuleInfo[j].enType == Event::Type::OBJECT_REMOVAL && m_stAlgoObjectRemovalCfg.bEnable)
                        {
                            auto it = m_freezeBgUntilMap.find(j);
                            if (it != m_freezeBgUntilMap.end() && now < it->second)
                            {
                                freezeBg = true;
                                break;
                            }
                        }
                    }
                    
                    /* 无事件监测时更新背景帧, 每隔10s更新 */
                    if (timeSinceLastUpdate > 10 && !freezeBg)
                    {
                        m_backgroundFrame = currentFrame.clone();
                        m_lastBgUpdateTime = now;
                        if (access("/testPrint", F_OK) == 0) {dlog_debug("ai_app: 物品检测: 背景帧已更新");}
                        continue;
                    }
                    
                    /* 全图变化率过高(>30%)：全局场景变换（摄像头晃动/光照变化），更新背景并完全重置状态 */
                    {
                        double stRate = ST / (diff.rows * diff.cols * 255 + 1e-5);
                        if (stRate > 0.30)
                        {
                            m_backgroundFrame = currentFrame.clone();
                            m_lastBgUpdateTime = now;
                            m_highSustainedStartMap.clear();
                            m_lastHighS1TimeMap.clear();
                            m_freezeBgUntilMap.clear();
                            m_highS1RateMap.clear();
                            m_eventTriggeredMap.clear();
                            m_lastEndTimeMap.clear();
                            m_eventTriggerStartMap.clear();
                            m_removalHighStartMap.clear();
                            m_removalHighDropTime.clear();
                            m_lastLowTimeMap.clear();
                            m_lastItemState = NONE;
                            if (access("/testPrint", F_OK) == 0) {dlog_debug("ai_app: 物品检测: 全局场景变化,更新背景");}
                            continue;
                        }
                    }

                    /* 遍历检测区域 */
                    for (int i = 0; i < m_vstRuleInfo.size(); i++)
                    {
                        const auto& rule = m_vstRuleInfo[i];
						for (const auto& area : rule.areas)
						{
							if (access("/testPrint", F_OK) == 0)
							{
								dlog_debug("==============打印当前区域=================");
								ObjectDetectPrintArea(area);
								drawRulesToImage(currentFrame);
								Modules_NS::saveImage(currentFrame, "/mnt/object_test");
							}
							
							/* 创建检测区域的掩码 */
							polygon.clear();
							for (const auto& point : area)
							{
								polygon.emplace_back(point.nX, point.nY);
							}

							if (mask.size() != diff.size() || mask.type() != CV_8U)
							{
								mask.create(diff.size(), CV_8U);
							}
							mask.setTo(0);
							cv::fillPoly(mask, polygon, cv::Scalar(255));

							/* 计算检测区域变化像素值的总和S1 */
							cv::bitwise_and(diff, diff, maskedDiff, mask);

							double S1 = cv::sum(maskedDiff)[0];

							/* 计算 ST_rate 和 S1_rate */
							double ST_rate = ST / (diff.rows * diff.cols * 255 + 1e-5);     /* 全图变化率 */
							double S1_rate = S1 / (cv::countNonZero(mask) * 255 + 1e-5);    /* 区域变化率 */
							
							/* 计算相对变化占比
							 * 当 ST_rate 很小时（画面稳定），fObjDiffThrd 设为 0，避免除法溢出导致误触发
							 */
							float fObjDiffThrd = (ST_rate > 0.001) ? static_cast<float>((S1_rate / (ST_rate + 1e-5))) : 0.0f;
							float nThreshold = 0.5f + (ST_rate * m_fSensiThrd);
							float areaRatio = cv::countNonZero(mask) / (float)(diff.rows * diff.cols);


							/* 区分事件类型 */
							if (rule.enType == Event::Type::UNATTENDED_OBJECT && m_stAlgoUnattendedObjectCfg.bEnable)
							{
								/*
							 * 物品遗留触发条件：
							 *   1. S1_rate > 4%         — 区域有明显变化
							 *   2. fObjDiffThrd >= 0.45 — 区域变化率 ≥ 全图的 45%
							 *   3. 局部/全局自适应：
							 *      - 小区域(<50%画面): S1_rate - ST_rate > 1%
							 *      - 大区域(≥50%画面): ST_rate < 25%
							 */
							bool bTrigger;
							if (areaRatio < 0.5f) {
								bTrigger = (S1_rate * 100 > 4) && (fObjDiffThrd >= 0.45f) && (S1_rate - ST_rate > 0.01);
							} else {
								bTrigger = (S1_rate * 100 > 4) && (fObjDiffThrd >= 0.45f) && (ST_rate < 0.25);
							}
								/* 冷却期检查：结束事件后 15 秒内冷却，但 S1 曾 LOW >5s 又 HIGH 视为新放置 */
								auto endIt = m_lastEndTimeMap.find(i);
								if (endIt != m_lastEndTimeMap.end())
								{
									auto elapsedSinceEnd = std::chrono::duration_cast<std::chrono::seconds>(now - endIt->second).count();
									if (elapsedSinceEnd < 5)
									{
										/* 冷却期内：检查是否 S1 曾 LOW >5s 又变 HIGH（新放置） */
										auto lowIt = m_lastLowTimeMap.find(i);
										if (lowIt != m_lastLowTimeMap.end() && bTrigger)
										{
											auto lowDuration = std::chrono::duration_cast<std::chrono::seconds>(now - lowIt->second).count();
											if (lowDuration >= 5)
											{
												dlog_debug("规则[%d]冷却期内检测到新放置(低电平>5s), 提前结束冷却", i);
												m_lastEndTimeMap.erase(i);
												lastStayTimeMap.erase(i);
												m_lastLowTimeMap.erase(i);
											}
											else
											{
												continue;
											}
										}
										else
										{
											continue;
										}
									}
									else
									{
									/* 冷却期结束，清除冷却记录 */
									dlog_debug("规则[%d]冷却期结束", i);
									lastStayTimeMap.erase(i);
									m_lastEndTimeMap.erase(i);
									}
								}
							//dlog_debug("物品遗留检测 - S1_rate:%.2f%%, ST_rate:%.4f, area:%.0f%%, fObjDiffThrd:%.2f, bTrigger:%d, state:%d",
							//           S1_rate * 100, ST_rate, areaRatio * 100, fObjDiffThrd, bTrigger, m_lastItemState);
								/* 物品遗留逻辑 */
								if (bTrigger)
								{
									/* 冻结背景：仅在事件未触发过或事件进行中时冻结
									 * 事件已结束(state=LEFT)时不冻结，让 bg 更新吸收物体后 S1 跌落重置状态 */
									if (!(m_lastItemState == LEFT && !m_eventTriggeredMap[i]))
									{
										m_lastBgUpdateTime = now;
									}

									if (lastStayTimeMap.find(i) == lastStayTimeMap.end())
									{
										dlog_debug("规则[%d]物品遗留首次检测到物体", i);
										lastStayTimeMap[i] = now;
										m_lastLowTimeMap.erase(i);
									}
									else if (!m_eventTriggeredMap[i])
									{
										/* 事件未触发过，检查时间阈值 */
										auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastStayTimeMap[i]).count();
										if (duration >= m_nLeaveTimeThrd)
										{
											/* 在规则循环内直接处理，防止 m_eventTriggeredMap 提前设 true 后又被 state 拦截 */
											if (m_lastItemState == NONE || m_lastItemState == PICKED)
											{
												/* 防误判：OBJECT_REMOVAL 正在跟踪连续 HIGH 时，不触发遗留（可能是移除） */
												if (m_stAlgoObjectRemovalCfg.bEnable && !m_bOnlyRemoval)
												{
													bool bRemovalActive = false;
													for (size_t j = 0; j < m_vstRuleInfo.size(); j++)
													{
														if (m_vstRuleInfo[j].enType == Event::Type::OBJECT_REMOVAL
														    && m_removalHighStartMap.find((int)j) != m_removalHighStartMap.end())
														{
															bRemovalActive = true;
															break;
														}
													}
													if (bRemovalActive)
													{
														dlog_debug("ai_app: 物体移除检测中, 跳过物品遗留触发");
														continue;
													}
												}
												m_lastItemState = LEFT;
												m_eventTriggeredMap[i] = true;
												m_eventTriggerStartMap[i] = now;
											dlog_debug("=======ai_app: 物品遗留事件触发==========");
#ifdef ENABLE_TVSDK_SRC
											{
												EventTriggerContext_S stCtx;
												stCtx.enEventType = Event::Type::UNATTENDED_OBJECT;
												stCtx.nChnId = m_nChannelId;
												stCtx.llTimestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
													std::chrono::system_clock::now().time_since_epoch()).count();
												if (!m_lastRgbFrame.empty()) {
												auto pPayload = std::make_shared<EventTvSdkPayload_S>();
												pPayload->enType = get_tvsdk_payload_type(stCtx.enEventType);
												if (encode_mat_to_tvsdk_image(m_lastRgbFrame, pPayload->stPanoramaImage, 85, false)) {
													stCtx.pTvSdkPayload = pPayload;
												}
												}
												stCtx.bEventEnded = false;
												CEventLinkage::instance()->handleEvent(stCtx);
											}
#else
											CEventLinkage::instance()->handleEvent(Event::Type::UNATTENDED_OBJECT, false);
#endif
												lastRemoveTimeMap.clear();
											}
											else
											{
												dlog_debug("ai_app: 物品未拿取, 忽略掉本次物品遗留事件");
											}
										}
									}
								}
								else
								{
									/* 触发条件不满足（物体消失） */
									if (m_lastItemState == LEFT && !m_eventTriggeredMap[i])
									{
										m_lastItemState = NONE;
									}
									if (lastStayTimeMap.find(i) != lastStayTimeMap.end())
									{
										auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastStayTimeMap[i]).count();
										if (duration >= 1800)
										{
											dlog_debug("物品遗留检测 - 物体消失超过1.8秒, 重置检测");
											m_lastLowTimeMap[i] = now;
											lastStayTimeMap.erase(i);
											/* 事件结束：物体消失，物品遗留事件结束 */
											if (m_eventTriggeredMap[i])
											{
												dlog_debug("=======ai_app: 物品遗留事件结束==========");
#ifdef ENABLE_TVSDK_SRC
												{
													EventTriggerContext_S stCtx;
													stCtx.enEventType = Event::Type::UNATTENDED_OBJECT;
													stCtx.nChnId = m_nChannelId;
													stCtx.llTimestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
														std::chrono::system_clock::now().time_since_epoch()).count();
													if (!m_lastRgbFrame.empty()) {
														auto pPayload = std::make_shared<EventTvSdkPayload_S>();
														pPayload->enType = get_tvsdk_payload_type(stCtx.enEventType);
														if (encode_mat_to_tvsdk_image(m_lastRgbFrame, pPayload->stPanoramaImage, 85, false)) {
															stCtx.pTvSdkPayload = pPayload;
														}
													}
													stCtx.bEventEnded = true;
													CEventLinkage::instance()->handleEvent(stCtx);
												}
#else
												CEventLinkage::instance()->handleEvent(Event::Type::UNATTENDED_OBJECT, true);
#endif
												m_eventTriggeredMap[i] = false;
												m_eventTriggerStartMap.erase(i);
												m_lastEndTimeMap[i] = now; /* 记录结束时间，进入 30 秒冷却期 */
											}
											else
											{
												/* 事件未触发过，重置状态 */
												m_lastItemState = NONE;
											}
										}
									}
								}
							}
							else if (rule.enType == Event::Type::OBJECT_REMOVAL && m_stAlgoObjectRemovalCfg.bEnable)
							{
								float pickupThreshold = 1.2f + (ST_rate * m_fSensiThrd * 0.3f);

								/*
								 * 物品拿取触发条件：
								 *   1. hadRecentChange — 最近 30 秒内区域曾有明显变化（物体存在过）
								 *   2. S1_rate < 15%   — 当前区域变化率低（物体已被移走，区域稳定）
								 *   3. fObjDiffThrd < pickupThreshold — 区域变化占比低于阈值
								 *
								 * 静态画面：S1_rate 从未 HIGH → hadRecentChange=false → 不触发
								 * 物体放回：S1_rate HIGH → 不满足条件2 → 不触发
								 * 物体拿走：区域曾 HIGH→现在 LOW → 触发
								 */
								/* 记录 S1_rate 为 HIGH 开始跟踪，要求场景稳定（排除摄像头晃动）。小区域宽、大区域更宽 */
								float stRateGuard = (areaRatio < 0.5f) ? 0.20f : 0.30f;
								if (S1_rate * 100 > 3.0 && ST_rate < stRateGuard)
								{
									if (m_highSustainedStartMap.find(i) == m_highSustainedStartMap.end())
									{
										m_highSustainedStartMap[i] = now;
									}
									m_lastHighS1TimeMap[i] = now;
									/* 记录连续 HIGH 起始时间（用于启动时物体已在场景的拿取检测） */
									if (m_removalHighStartMap.find(i) == m_removalHighStartMap.end())
									{
										m_removalHighStartMap[i] = now;
										m_highS1RateMap.erase(i); /* 新周期清除旧峰值 */
									}
								}
								else
								{
									if (m_removalHighStartMap.find(i) != m_removalHighStartMap.end())
									{
										m_removalHighDropTime[i] = now;
									}
									m_removalHighStartMap.erase(i);
								}
								auto it = m_lastHighS1TimeMap.find(i);
								bool hadRecentChange = false;
								if (it != m_lastHighS1TimeMap.end())
								{
									auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - it->second).count();
									bool sustained = false;
									auto sustainedIt = m_highSustainedStartMap.find(i);
									if (sustainedIt != m_highSustainedStartMap.end())
									{
										auto dur = std::chrono::duration_cast<std::chrono::seconds>(now - sustainedIt->second).count();
										if (S1_rate * 100 > 3.0)
										{
											sustained = (dur >= 2);
										}
										else
										{
											auto highDur = std::chrono::duration_cast<std::chrono::seconds>(it->second - sustainedIt->second).count();
											sustained = (highDur >= 2);
										}
									}
									hadRecentChange = (elapsed < 20) && sustained;
									if (hadRecentChange)
									{
										m_freezeBgUntilMap[i] = now + std::chrono::seconds(15);
										float curr = S1_rate * 100;
										auto highIt = m_highS1RateMap.find(i);
										if (highIt == m_highS1RateMap.end() || curr > highIt->second)
										{
											m_highS1RateMap[i] = curr;
										}
									}
								}
								/* 相对阈值：当前 S1_rate 低于物体存在时水平的 70% 即认为已被拿走 */
								bool isLow = false;
								{
									auto highIt = m_highS1RateMap.find(i);
									if (highIt != m_highS1RateMap.end())
									{
										isLow = (S1_rate * 100 < highIt->second * 0.5f);
									}
								}
								/* 状态机要求：非纯拿取模式下 state 必须是 LEFT（物品曾遗留过） */
								bool stateAllowsRemoval = m_bOnlyRemoval
								    ? (m_lastItemState == NONE || m_lastItemState == PICKED)
								    : (m_lastItemState == LEFT);
								/* fObjDiffThrd 仅在 S1 较高时做 check（S1<3% 说明已稳定拿走，比值无意义） */
								bool bObjDiffOk = (S1_rate * 100 < 3.0) || (fObjDiffThrd < pickupThreshold);
								bool bTrigger = (hadRecentChange && isLow && bObjDiffOk && stateAllowsRemoval);
								//dlog_debug("物品拿取检测 - S1_rate:%.2f%%, hadRecentChange:%d, isLow:%d, bTrigger:%d",
								//           S1_rate * 100, hadRecentChange, isLow, bTrigger);
								/* 物品拿取逻辑 */
								if (bTrigger)
								{
									if (10 - timeSinceLastUpdate < 3)
									{
										m_lastBgUpdateTime += std::chrono::seconds(1);
									}

									if (lastRemoveTimeMap.find(i) == lastRemoveTimeMap.end())
									{
										/* 首次检测到物体消失 */
										lastRemoveTimeMap[i] = now;
									}
									else if (!m_eventTriggeredMap[i])
									{
										/* 事件未触发过，检查时间阈值 */
										auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastRemoveTimeMap[i]).count();
										if (duration >= m_nPickupTimeThrd)
										{
											triggeredEvents.push_back((int)Event::Type::OBJECT_REMOVAL);
											m_eventTriggeredMap[i] = true;
											m_eventTriggerStartMap[i] = now;
											m_lastBgUpdateTime -= std::chrono::seconds(10);
										}
									}
									/* 事件已触发过，保持 lastRemoveTimeMap 不清除，等待区域变化后重置 */
								}
								else
								{
									/* 触发条件不满足（区域又变化） */
									if (lastRemoveTimeMap.find(i) != lastRemoveTimeMap.end())
									{
										auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastRemoveTimeMap[i]).count();
										if (duration >= 1800)
										{
											lastRemoveTimeMap.erase(i);
											/* 事件结束：区域又发生变化，物品拿取事件结束 */
											if (m_eventTriggeredMap[i])
											{
											dlog_debug("=======ai_app: 物品拿取事件结束======");
#ifdef ENABLE_TVSDK_SRC
											{
												EventTriggerContext_S stCtx;
												stCtx.enEventType = Event::Type::OBJECT_REMOVAL;
												stCtx.nChnId = m_nChannelId;
												stCtx.llTimestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
													std::chrono::system_clock::now().time_since_epoch()).count();
												if (!m_lastRgbFrame.empty()) {
												auto pPayload = std::make_shared<EventTvSdkPayload_S>();
												pPayload->enType = get_tvsdk_payload_type(stCtx.enEventType);
												if (encode_mat_to_tvsdk_image(m_lastRgbFrame, pPayload->stPanoramaImage, 85, false)) {
													stCtx.pTvSdkPayload = pPayload;
												}
												}
												stCtx.bEventEnded = true;
												CEventLinkage::instance()->handleEvent(stCtx);
											}
#else
											CEventLinkage::instance()->handleEvent(Event::Type::OBJECT_REMOVAL, true);
#endif
												m_eventTriggeredMap[i] = false;
												m_eventTriggerStartMap.erase(i);
												m_removalHighDropTime[i] = now;
												m_removalHighStartMap.erase(i);
												m_lastEndTimeMap[i] = now;
											}
										}
									}
										}
									}
								/* 强制超时结束：事件触发后超过阈值仍未自然结束则强制结束 */
								if (m_eventTriggeredMap[i])
								{
									auto triggerIt = m_eventTriggerStartMap.find(i);
									if (triggerIt != m_eventTriggerStartMap.end())
									{
										auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - triggerIt->second).count();
										bool shouldEnd = false;
										if (rule.enType == Event::Type::UNATTENDED_OBJECT)
										{
											shouldEnd = (elapsed > 2500);
										}
										else if (rule.enType == Event::Type::OBJECT_REMOVAL)
										{
											shouldEnd = (elapsed > 2500);
										}
										if (shouldEnd)
										{
										dlog_debug("ai_app: 事件触发超时(%lldms), 强制结束", (long long)elapsed);
#ifdef ENABLE_TVSDK_SRC
										{
											EventTriggerContext_S stCtx;
											stCtx.enEventType = rule.enType;
											stCtx.nChnId = m_nChannelId;
											stCtx.llTimestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
												std::chrono::system_clock::now().time_since_epoch()).count();
											if (!m_lastRgbFrame.empty()) {
												auto pPayload = std::make_shared<EventTvSdkPayload_S>();
												pPayload->enType = get_tvsdk_payload_type(stCtx.enEventType);
												if (encode_mat_to_tvsdk_image(m_lastRgbFrame, pPayload->stPanoramaImage, 85, false)) {
													stCtx.pTvSdkPayload = pPayload;
												}
											}
											stCtx.bEventEnded = true;
											CEventLinkage::instance()->handleEvent(stCtx);
										}
#else
										CEventLinkage::instance()->handleEvent(rule.enType, true);
#endif
											m_eventTriggeredMap[i] = false;
											if (rule.enType == Event::Type::UNATTENDED_OBJECT)
											{
												lastStayTimeMap.erase(i);
											}
											else if (rule.enType == Event::Type::OBJECT_REMOVAL)
											{
												lastRemoveTimeMap.erase(i);
											}
											m_lastEndTimeMap[i] = now;
											m_eventTriggerStartMap.erase(i);
											if (rule.enType == Event::Type::OBJECT_REMOVAL)
											{
												m_removalHighDropTime[i] = now;
											}
											m_removalHighStartMap.erase(i);
										}
									}
								}
								}
                            
                    }
                    
                    for (auto eventType : triggeredEvents)
                    {
                        if (access("/drawRulesToImage", F_OK) == 0)
                        {
                            drawRulesToImage(i420Mat);
                        }

                        if (eventType == (int)Event::Type::OBJECT_REMOVAL)
                        {
                            // 单独拿取模式下，允许从NONE/PICKED状态触发
						bool canTrigger = (m_bOnlyRemoval && (m_lastItemState == NONE || m_lastItemState == PICKED)) 
									|| (!m_bOnlyRemoval && m_lastItemState == LEFT);
                            if (canTrigger)
                            {
                                m_lastItemState = PICKED;
								/* 上报事件 */
                                dlog_debug("=======ai_app: 物品拿取事件触发======");
#ifdef ENABLE_TVSDK_SRC
                                {
                                    EventTriggerContext_S stCtx;
                                    stCtx.enEventType = Event::Type::OBJECT_REMOVAL;
                                    stCtx.nChnId = m_nChannelId;
                                    stCtx.llTimestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
                                        std::chrono::system_clock::now().time_since_epoch()).count();
                                    if (!m_lastRgbFrame.empty()) {
                                        auto pPayload = std::make_shared<EventTvSdkPayload_S>();
                                        pPayload->enType = get_tvsdk_payload_type(stCtx.enEventType);
                                        if (encode_mat_to_tvsdk_image(m_lastRgbFrame, pPayload->stPanoramaImage, 85, false)) {
                                            stCtx.pTvSdkPayload = pPayload;
                                        }
                                    }
                                    stCtx.bEventEnded = false;
                                    CEventLinkage::instance()->handleEvent(stCtx);
                                }
#else
                                CEventLinkage::instance()->handleEvent(Event::Type::OBJECT_REMOVAL, false);
#endif
								lastStayTimeMap.clear();
								m_freezeBgUntilMap.clear();
								m_highS1RateMap.clear();
								if (m_bOnlyRemoval)
								{
									/* 冷却期 30 秒：防止同一物体反复触发，等待背景帧充分更新后再允许下一次检测 */
									std::thread([this]() {
										std::this_thread::sleep_for(std::chrono::seconds(30));
										m_lastItemState = NONE;
									}).detach();
								}
                               
                            }
                            else
                            {
                                dlog_debug("ai_app: 物品未遗留, 忽略掉本次物品拿取事件");
                            }
                        }
                    }
                }
            }
            else
            {
                dlog_error("ai_app: 图片数据为空");
            }
        }
        else
        {
            dlog_error("ai_app: 物品检测-获取虚拟地址失败");
        }
    }
}


template<typename T>
void CObjectDetect::convertResolutionAndEnable(T &stConfig,Event::Type enType)
{
	if (!stConfig.aRule.empty())
    {
        /* 是否有任何一个区域初始化成功 */
        bool bIsInit = false;
        
        /* 转换区域坐标分辨率至算法分辨率 */
        for (auto &rule : stConfig.aRule) /* 使用引用而不是值拷贝 */
        {
            Event::RuleInfo stRule;
            std::vector<::Event::Point_S> area;
            /* 判断是否设置了正确的多边形 */
            if (!rule.stRegion.IsValid())
            {
                stRule.bEnable = false;
                dlog_debug("当前多边形区域无效");
                continue;
            }

            bool bVaildPoint = false;
            for (int i = 0; i < 4; ++i) 
            {
                const auto& point = rule.stRegion.aPoint[i];

                if (point.fX != 0 || point.fY != 0 ) 
                {
                    bVaildPoint = true;
                    dlog_debug("当前多边形区域有效");
                    break;
                }
            }
            if(!bVaildPoint)
            {
                stRule.bEnable = false;
                dlog_debug("当前多边形区域无效");
                continue;
            }
			stRule.bEnable = true;
            rule.stRegion.ConvertResolution(PIXEL_WIDTH_1920, PIXEL_HEIGHT_1080, m_nWidth, m_nHeight);
          
			stRule.enType = enType;
			stRule.nSensitivity = rule.nSensitivity;
			stRule.nTimeThreshold = rule.nTimeThreshold;
			dlog_debug("ai_app:  当前规则获取到 灵敏度[%d] 时间阈值[%d] ",rule.nSensitivity,rule.nTimeThreshold);
			for (auto &pos : rule.stRegion.aPoint)
			{
				::Event::Point_S stPoint;
				stPoint.nX = pos.fX;
				stPoint.nY = pos.fY;
				area.push_back(stPoint);
			}
			stRule.areas.push_back(area);
            
            if(!stRule.bEnable)
            {
                dlog_debug("当前规则无效");
                continue;
            }
			m_vstRuleInfo.push_back(stRule);
        }
    }

}
