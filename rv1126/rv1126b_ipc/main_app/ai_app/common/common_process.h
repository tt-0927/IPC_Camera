/**
 * @FilePath     : common_process.h
 * @Author       : zhouzirui
 * @Date         : 2025-06-06 16:02:10
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2025-08-18 14:40:56
 * @Description  : AIAPP 公共处理函数
 */

#pragma once

#include <iostream>
#include <unordered_map>
#include <chrono>
#include <thread>
#include <ctime>
#include "Json.h"
#include "event_configure.h"
#include "action_code.h"
#include "event_configure.h"
#include "convert_interface.h"
#include "osd_manage.h"
#include "result_manager.hpp"
#include <opencv2/opencv.hpp>
#include "alarm_define.h"
#include "RockchipRga.h"
#include "im2d.h" 

#if CAP_EXHIBITION_OSD_PANEL
#include "osd_panel/osd_panel_result.hpp"
#endif


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
 * @brief   : 打印移动侦测结果
 * @param    {std::vector<Common::RectInfo_S>} &vstRectsInfo：移动侦测结果
 */
void printResult(const std::vector<Common::RectInfo_S> &vstRectsInfo);

/**
 * @brief   : 打印场景变更结果
 * @param    {std::vector<Common::RectInfo_S>} &vstRectsInfo：场景变更结果
 */
void printResult(const std::vector<std::vector<int>> &vstRectsInfo);

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

/**
 * @brief   : 发送结果至OSD模块，进行框选显示
 * @param    {int} nWidth：框对应的分辨率宽
 * @param    {int} nHeight：框对应的分辨率高
 * @param    {vector<Common::RectInfo_S>} &vstRectInfo：OSD用矩形检测结果框
 */
void send_detectionResult_to_osd(const int nWidth, const int nHeight, const std::vector<Common::RectInfo_S> &vstRectInfo);

/**
 * @brief   : 发送面积最大的矩形结果至OSD模块，进行框选显示
 * @param    {int} nWidth：框对应的分辨率宽
 * @param    {int} nHeight：框对应的分辨率高
 * @param    {vector<Common::RectInfo_S>} &vstRectsInfo：移动侦测结果
 */
void send_detectionResult_to_osd(const int nWidth, const int nHeight, const Common::RectInfo_S &vstRectsInfo);

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
 * @brief   : 将矩形框转换为展会面板矩形
 * @param    {const Common::RectInfo_S &} stRect：矩形框
 * @return   {Common::RectInfo_S} 展会面板矩形
 */
Common::RectInfo_S to_exhibition_panel_rect(const Common::RectInfo_S &stRect);

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
bool is_in_region(const Alarm::Region_S &stRegion, const Common::RectInfo_S &stObject);

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
 * @brief 将图片从源路径移动到目标路径，并更新配置中的路径
 * @param targetBasePath 目标路径的基础目录
 * @param ImagePath 图片路径加文件名
 * @return 0表示成功, -1表示失败
 */
int moveImageToAnalysisDir(const std::string& targetBasePath,std::string& ImagePath);

/**
 * @brief 判断文件路径是否在指定目录下
 * @param filePath 完整的文件路径
 * @param directory 目录路径
 * @return true-文件在指定目录下, false-文件不在指定目录下
 */
bool isFileInDirectory(const std::string& filePath, const std::string& directory);

/**
 * @brief 根据时间找到具体视频分片ts文件
 * @param recordDir 目标路径
 * @param createTime 日期时间
 * @return 对应文件
 */
std::string findClosestTSByM3U8(const std::string& recordDir, const std::string& createTime);

 /**
 * @brief 构建用于多模态大模型的视觉输入Prompt
 * @param user_query [in] 用户的原始文本问题
 * @return 返回包含了<image>标签的、符合模型输入规范的完整Prompt字符串
 */
 std::string build_vision_prompt(const std::string& user_query);
#ifdef DEVICE_TV_3882TI
/*** 
* @description : 将检测对象枚举转换为字符串
* @author      : cyc
* @param        {TextPreset_S&} stCfg
* @return       {*}
*/ 
std::string get_object_string(const Alarm::TextPreset_S& stCfg);

/*** 
* @description : 将判定条件枚举转换为字符串
* @author      : cyc
* @param        {TextPreset_S&} stCfg
* @return       {*}
*/ 
std::string get_condition_string(const Alarm::TextPreset_S& stCfg);

/*** 
 * @description : 构建用于文字预设任务的Prompt
 * @author      : cyc
 * @param        {TextPreset_S&} stTextPreseCfg
 * @return       {*}
 */ 
std::string build_text_preset_prompt(const Alarm::TextPreset_S& stTextPreseCfg);
#endif
/**
 * @brief 构建用于画面分析的Prompt
 * @param user_query [in] 用户的原始文本问题
 * @param isText 是否纯文本问题
 * @return 返回包含了<image>标签的、符合模型输入规范的完整Prompt字符串
 */
std::string build_image_analysis_prompt(bool isText,const std::string& user_query);

/**
 * @brief 将源RGB图像的ROI区域缩放后居中填充到目标RGB图像
 * @param src 源RGB图像 (CV_8UC3)
 * @param roi 源图像中的感兴趣区域 (ROI)
 * @param dst 目标RGB图像 (CV_8UC3)，函数会直接修改此图像
 * @return 是否成功
 */
bool fillRGBToCenter(const cv::Mat& src, const cv::Rect& roi, cv::Mat& dst);

/**
 * @brief 处理图像黑边（裁剪边缘黑色区域）并转换为 BGR 格式
 * @param src 输入图像（支持 3 通道 RGB/BGR 格式）
 * @param dst 输出图像（CV_8UC3 BGR 格式，已去除黑边）
 * @param black_threshold 黑色阈值（默认 10，像素值 <= 此值视为黑色）
 * @return 是否成功（true=成功，false=输入无效或全黑图像）
 */
bool removeBlackBorderAndConvertToBGR(const cv::Mat& src, cv::Mat& dst, int black_threshold = 10);

/**
* @brief RGA 通用图像处理,RGA硬件加速：颜色空间转换并缩放,增加裁剪
* @param src_vir 源数据指针 (NV12)
* @param sw 源图完整宽度
* @param sh 源图完整高度
* @param src_format  源图像格式 (例如: RK_FORMAT_YCbCr_420_SP, RK_FORMAT_RGB_888 等)
* @param dst_vir 目标数据指针
* @param dw 目标图宽度
* @param dh 目标图高度
* @param target_format 目标格式 (如 RK_FORMAT_BGR_888)
* @param cx 裁剪起始点 X (默认为 0)
* @param cy 裁剪起始点 Y (默认为 0)
* @param cw 裁剪宽度 (如果为 0，则设为 sw)
* @param ch 裁剪高度 (如果为 0，则设为 sh)
* @param rotation   旋转角度 (0, 90, 180, 270)
* @note :
        1.坐标 (cx, cy): 必须 2 对齐 (偶数)。
        2.源裁剪尺寸 (cw, ch): 必须 2 对齐 (偶数)。
        3.输出宽度 dw: 对于 RGB888 格式，必须 4 对齐。
        4.输出高度 dh: 必须 2 对齐 (偶数)。
* @return true-转换成功，false-转换失败
*/
bool rga_image_transform(void* src_vir, int sw, int sh, int src_format,
                         void* dst_vir, int dw, int dh, int dst_format,
                         int cx = 0, int cy = 0, int cw = 0, int ch = 0,
                         int rotation = 0);

