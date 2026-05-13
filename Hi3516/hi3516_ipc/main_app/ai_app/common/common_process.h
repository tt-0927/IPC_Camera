/**
 * @FilePath     : common_process.h
 * @Author       : zhouzirui
 * @Date         : 2025-06-06 16:02:10
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-04-20 15:29:34
 * @Description  : AIAPP 公共处理函数
 */

#pragma once

#include <iostream>
#include <unordered_map>
#include <chrono>
#include <thread>
#include <ctime>
#include "Json.h"

#include "action_code.h"
#include "event_configure.h"
#include "convert_interface.h"
#include "osd_manage.h"
#if CAP_EXHIBITION_OSD_PANEL
#include "osd_panel/osd_panel_result.hpp"
#endif
#include "YoloUltralytics_rpn.hpp"
#include "YoloUltralyticsPoint_rpn.hpp"
#include "stream_ai_detect.h"
#include "result_manager.hpp"

/**
 * @brief   : 打印目标检测算法结果
 * @param    {vector<Inference_NS::BoxData_S>} &vBoxDatas：算法结果框体数据
 */
void printResult(const std::vector<Inference_NS::BoxData_S> &vBoxDatas);

/**
 * @brief   : 打印关键点检测算法结果
 * @param    {vector<Inference_NS::PointData_S>} &vPointDatas：算法结果框体数据
 */
void printResult(const std::vector<Inference_NS::PointData_S> &vPointDatas);

/**
 * @brief   : 打印移动侦测结果
 * @param    {ot_sample_svp_rect_info} &stRectInfo：移动侦测结果
 */
void printResult(const ot_sample_svp_rect_info &stRectInfo);

/**
 * @brief 打印画线规则
 * @param ruleInfo 
 */
void printRuleInfo(const Event::RuleInfo_S& ruleInfo);

/**
 * @brief 打印 AlgorithmConfig_S
 * @param config 
 */
void printAlgoCfg(const Event::AlgorithmConfig_S &config);

/**
 * @brief 区域判断
 * @param pt 
 * @param polygon 
 */
// bool isPointInPolygon(const cv::Point2f& pt, const Event::Area& polygon);

/**
 * @brief 坐标比例放大
 * @param scaleFactor 
 */
void pointScaleUp(int& x1, int& y1, int& x2, int& y2, int max_w, int max_h, double scaleFactor);


void SendResToControl(std::vector<std::string> vstrPicPath);
void SendResToControl(int nChnId, std::string imagePath, int nType);
void SendResToControl(std::string& strData, int nActionCode, int nRetCode);
void SendResToControl(std::vector<int64_t> vIndices, std::vector<float> vfSimilarity);
void SendResToControl(int nChnId, std::string imagePath, std::string facePath, int nFaceLibId, float fSimilarity);


/**
 * @brief   : 发送结果至OSD模块，进行框选显示
 * @param    {int} nWidth：框对应的分辨率宽
 * @param    {int} nHeight：框对应的分辨率高
 * @param    {vector<Inference_NS::BoxData_S>} &vBoxDatas：目标检测框结果
 */
void send_detectionResult_to_osd(const int nWidth, const int nHeight, const std::vector<Inference_NS::BoxData_S> &vBoxDatas);

/**
 * @brief   : 发送结果至OSD模块，进行框选显示
 * @param    {int} nWidth：框对应的分辨率宽
 * @param    {int} nHeight：框对应的分辨率高
 * @param    {vector<Inference_NS::PointData_S>} &vPointDatas：关键点检测框结果
 */
void send_detectionResult_to_osd(const int nWidth, const int nHeight, const std::vector<Inference_NS::PointData_S> &vPointDatas);

/**
 * @brief   : 发送面积最大的矩形结果至OSD模块，进行框选显示
 * @param    {int} nWidth：框对应的分辨率宽
 * @param    {int} nHeight：框对应的分辨率高
 * @param    {ot_sample_svp_rect_info} &stRectInfo：移动侦测结果
 */
void send_detectionResult_to_osd(const int nWidth, const int nHeight, const ot_sample_svp_rect_info &stRectInfo);

/**
 * @brief   : 发送结果至OSD模块，进行框选显示
 * @param    {int} nWidth：框对应的分辨率宽
 * @param    {int} nHeight：框对应的分辨率高
 * @param    {ot_aidetect_result_array} &stResult：脸人车侦测结果
 */
void send_detectionResult_to_osd(const int nWidth, const int nHeight, const ot_aidetect_result_array &stResult);

/**
 * @brief   : 发送结果至OSD模块，进行框选显示
 * @param    {int} nWidth：框对应的分辨率宽
 * @param    {int} nHeight：框对应的分辨率高
 * @param    {vector<Common::RectInfo_S>} &vstRectInfo：OSD用矩形检测结果框
 */
void send_detectionResult_to_osd(const int nWidth, const int nHeight, const std::vector<Common::RectInfo_S> &vstRectInfo);

#if CAP_EXHIBITION_OSD_PANEL
/**
 * @brief   : 当前事件是否启用展会版左上角汇总面板
 * @param    {Event::Type_E} enEventType：事件类型
 * @return   {bool} true：需要汇总 false：不需要汇总
 * @note    : 统一由该接口收口展会面板支持的事件范围
 */
bool is_exhibition_panel_supported(Event::Type_E enEventType);

/**
 * @brief   : 初始化展会面板帧公共信息
 * @param    {OsdPanel::PanelFrame_S *} pstPanelFrame：待初始化面板帧
 * @param    {Event::Type_E} enEventType：事件类型
 * @param    {int} nWidth：检测结果源宽
 * @param    {int} nHeight：检测结果源高
 * @return   {bool} true：初始化成功 false：当前事件不支持或指针为空
 */
bool prepare_exhibition_panel_frame(OsdPanel::PanelFrame_S *pstPanelFrame,
                                    Event::Type_E enEventType,
                                    int nWidth,
                                    int nHeight);

/**
 * @brief   : 将整数百分比格式化为展会面板文本
 * @param    {int} nPercent：百分比数值
 * @return   {std::string} 百分比文本
 */
std::string get_exhibition_panel_percent_text(int nPercent);

/**
 * @brief   : 将浮点置信度转换为展会面板文本
 * @param    {float} fConfidence：0~1 的置信度
 * @return   {std::string} 百分比文本
 */
std::string get_exhibition_panel_confidence_text(float fConfidence);

/**
 * @brief   : 获取展会面板状态文本
 * @param    {bool} bAlarm：当前条目是否报警
 * @return   {std::string} 状态文本
 */
std::string get_exhibition_panel_status_text(bool bAlarm);

/**
 * @brief   : 生成展会面板条目优先级
 * @param    {bool} bAlarm：当前条目是否报警
 * @param    {float} fConfidence：当前条目置信度
 * @return   {int} 条目优先级
 * @note    : 已报警优先，其次按置信度排序
 */
int build_exhibition_panel_priority(bool bAlarm, float fConfidence);

/**
 * @brief   : 将 AI 目标框转换为展会面板矩形
 * @param    {const ot_aidetect_object &} stObject：AI 目标
 * @return   {Common::RectInfo_S} 展会面板矩形
 */
Common::RectInfo_S to_exhibition_panel_rect(const ot_aidetect_object &stObject);

/**
 * @brief   : 将推理框转换为展会面板矩形
 * @param    {const Inference_NS::Box_S &} stBox：推理框
 * @return   {Common::RectInfo_S} 展会面板矩形
 */
Common::RectInfo_S to_exhibition_panel_rect(const Inference_NS::Box_S &stBox);

/**
 * @brief   : 将多边形区域转换为展会面板矩形
 * @param    {const Alarm::Region_S &} stRegion：区域定义
 * @return   {Common::RectInfo_S} 包围该区域的外接矩形
 */
Common::RectInfo_S to_exhibition_panel_rect(const Alarm::Region_S &stRegion);

/**
 * @brief   : 向展会面板追加或覆盖同排序键条目
 * @param    {OsdPanel::PanelFrame_S *} pstPanelFrame：面板结果
 * @param    {const OsdPanel::PanelItem_S &} stCandidate：候选条目
 * @return   {void}
 * @note    : 同一排序键只保留优先级更高的条目
 */
void upsert_exhibition_panel_item(OsdPanel::PanelFrame_S *pstPanelFrame,
                                  const OsdPanel::PanelItem_S &stCandidate);

/**
 * @brief   : 发送展会面板结果至OSD模块
 * @param    {const OsdPanel::PanelFrame_S &} stPanelFrame：展会面板结果
 * @return   {void}
 */
void send_panelResult_to_osd(const OsdPanel::PanelFrame_S &stPanelFrame);
#endif

/**
 * @brief   : 转换网格区域至矩形数据结构
 * @param    {vector<std::vector<unsigned int>>} &vRegion：网格二维向量
 * @param    {int} nWidth：网格所在的分辨率宽度
 * @param    {int} nHeight：网格所在的分辨率高度
 * @param    {Rect_S} &stRect：转换后的矩形数据结构
 */
void convert_gridRegion_to_rect(const std::vector<std::vector<unsigned int>> &vRegion, const int nWidth, const int nHeight, Common::Rect_S &stRect);

/**
 * @brief   : 转换区域宽高为指定倍率
 * @param    {Rect_S} &stRectInfo：矩形区域
 * @param    {float} fRatio：缩放倍率
 * @param    {int} nImageWidth：图像总宽度（用于边界检查）
 * @param    {int} nImageHeight：图像总高度（用于边界检查）
 */
void convert_region_ratio(Common::RectInfo_S &stRectInfo, const float fRatio, const int nImageWidth, const int nImageHeight);

/*** 
 * @description : 用户输入坐标映射到 640×384 算法坐标
 * @author      : cyc
 * @param        {Rect_S&} userIn
 * @param        {Rect_S&} algoOut
 * @return       {*}
 */
void mapUserToAlgo(const Common::Rect_S& userIn,Common::Rect_S& algoOut);

/**
 * @brief   : 检查目标是否在区域内
 * @param   : stRegion 区域定义
 * @param   : stObject 目标对象
 * @return  : bool 是否在区域内
 */
bool is_in_region(const Alarm::Region_S &stRegion, const ot_aidetect_object &stObject);

/**
 * @brief   : 检查目标是否在区域内
 * @param   : stRegion 区域定义
 * @param   : stBox 目标框
 * @return  : bool 是否在区域内
 */
bool is_in_region(const Alarm::Region_S &stRegion, const Inference_NS::Box_S &stBox);

/**
 * @brief   : 检查目标是否在区域内(调试版本)
 * @param   : stRegion 区域定义
 * @param   : stBox 目标框
 * @return  : bool 是否在区域内
 */
bool is_in_region_debug(const Alarm::Region_S &stRegion, const Inference_NS::Box_S &stBox);

/**
* @brief 通过Bounding Box方法快速排除两条线段没有交点
* @param [Common::PosF_S] lineA1: 线段A的第1个端点
* @param [Common::PosF_S] lineA2: 线段A的第2个端点
* @param [Common::PosF_S] lineB1: 线段B的第1个端点
* @param [Common::PosF_S] lineB2: 线段B的第2个端点
* @return [*]
* @note
*/
bool isBoundingBoxIntersecting(
    const Common::PosF_S &lineA1,
    const Common::PosF_S &lineA2,
    const Common::PosF_S &lineB1,
    const Common::PosF_S &lineB2);

/**
 * @brief   : 判断是否跨越线段
 * @param    {PosF_S} &p1 线段A的第1个端点
 * @param    {PosF_S} &q1 线段A的第2个端点
 * @param    {PosF_S} &p2 线段B的第1个端点
 * @param    {PosF_S} &q2 线段B的第2个端点
 * @return   {*} true:跨越 false:未跨越
 */
bool doLinesIntersect(const Common::PosF_S &p1,
                      const Common::PosF_S &q1,
                      const Common::PosF_S &p2,
                      const Common::PosF_S &q2);

/**
* @brief 计算叉积
* @param [Common::PosF_S] alertLineStart: 线段的第1个端点
* @param [Common::PosF_S] alertLineEnd: 线段的第2个端点
* @param [Common::PosF_S] testPoint: 待测的点
* @return [*]
* @note
*/
int crossProduct(const Common::PosF_S& alertLineStart,
const Common::PosF_S& alertLineEnd,
const Common::PosF_S& testPoint);

/**
* @brief 拌线检测：判断两条线段是否有交点
* @param [Common::PosF_S] startPoint: 行人的起始坐标点
* @param [Common::PosF_S] lastPoint: 行人的当前坐标点
* @param [Common::PosF_S] alertLineFirst: 警戒线第一个点坐标
* @param [Common::PosF_S] alertLineSecond: 警戒线第二个点坐标
* @return [CrossDirection_E] 类型
* @note
*/
Alarm::CrossDirection_E tripLineDetection(
    const Common::PosF_S &startPoint,
    const Common::PosF_S &lastPoint,
    const Common::PosF_S &alertLineFirst,
    const Common::PosF_S &alertLineSecond);

/**
 * @brief   : 添加海思AI detect结果至vector结果容器
 * @param    {ot_aidetect_object} &stObject 海思AI detect结果
 * @param    {vector<Common::RectInfo_S>} &vstRectInfo 待添加的vector结果容器
 * @return   {int} 0：成功，非0：失败
 */
int add_result_to_vector(const ot_aidetect_object &stObject, std::vector<Common::RectInfo_S> &vstRectInfo);

/**
 * @brief   : 添加算法目标框结果至vector结果容器
 * @param    {BoxData_S} &stBoxData 算法目标框结果
 * @param    {vector<Common::RectInfo_S>} &vstRectInfo 待添加的vector结果容器
 * @return   {int} 0：成功，非0：失败
 */
int add_result_to_vector(const Inference_NS::BoxData_S &stBoxData, std::vector<Common::RectInfo_S> &vstRectInfo);

/**
 * @brief   : 添加算法目标框结果至vector结果容器
 * @param    {PointData_S} &stPointData 算法目标框结果
 * @param    {vector<Common::RectInfo_S>} &vstRectInfo 待添加的vector结果容器
 * @return   {int} 0：成功，非0：失败
 */
int add_result_to_vector(const Inference_NS::PointData_S &stPointData, std::vector<Common::RectInfo_S> &vstRectInfo);

/**
 * @brief   : 添加检测区域至vector结果容器
 * @param    {Region_S} &stRegion 检测区域
 * @param    {vector<Common::RectInfo_S>} &vstRectInfo 待添加的vector结果容器
 * @return   {int} 0：成功，非0：失败
 */
int add_result_to_vector(const Alarm::Region_S &stRegion, std::vector<Common::RectInfo_S> &vstRectInfo);
