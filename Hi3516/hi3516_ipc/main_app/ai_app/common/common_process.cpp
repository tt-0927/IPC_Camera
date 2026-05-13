/**
 * @FilePath     : common_process.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2025-07-08 17:01:53
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-04-23 09:40:41
 * @Description  : AIAPP 公共处理函数
 */

#include "common_process.h"
#include "stream_server.h"
#if CAP_EXHIBITION_OSD_PANEL
#include <cmath>
#include <limits>
#endif

namespace
{
#if CAP_EXHIBITION_OSD_PANEL
/* 已报警条目的优先级基数，确保报警状态优先于纯置信度排序 */
constexpr int EXHIBITION_ALARM_PRIORITY_BASE = 100000;
#endif
} // namespace

void printResult(const std::vector<Inference_NS::BoxData_S> &vBoxDatas)
{
    if(vBoxDatas.size() <= 0)
    {
        return;
    }
    std::cout << "检测到对象的数量: " << vBoxDatas.size() << "\n";
    for (size_t i = 0; i < vBoxDatas.size(); i++)
    {
        std::cout << "对象: " << i + 1 << ":\n";
        std::cout << "类型ID: " << vBoxDatas[i].nLabel << "\n";
        std::cout << "置信度: " << vBoxDatas[i].fConfidence << "\n";
        std::cout << "边界框: (x1 = " << vBoxDatas[i].stBoxs.nX1 << ", y1 = " << vBoxDatas[i].stBoxs.nY1
                  << ", x2 = " << vBoxDatas[i].stBoxs.nX2 << ", y2 = " << vBoxDatas[i].stBoxs.nY2 << ")\n";
    }
    std::cout << "\n";
}

void printResult(const std::vector<Inference_NS::PointData_S> &vPointDatas)
{
    if(vPointDatas.size() <= 0)
    {
        return;
    }

    /* 打印输出数据 */
    std::cout << "检测到对象的数量: " << vPointDatas.size() << "\n";
    for (size_t i = 0; i < vPointDatas.size(); i++)
    {
        // printf("[%d ,%d] ,[%d ,%d] ,%f, %d\n",
        //     vPointDatas[i].stBoxs.nX1,
        //     vPointDatas[i].stBoxs.nY1,
        //     vPointDatas[i].stBoxs.nX2,
        //     vPointDatas[i].stBoxs.nY2,
        //     vPointDatas[i].fConfidence,
        //     vPointDatas[i].nLabel
        // );

        std::cout << "对象: " << i + 1 << ":\n";
        std::cout << "类型ID: " << vPointDatas[i].nLabel << "\n";
        std::cout << "置信度: " << vPointDatas[i].fConfidence << "\n";
        std::cout << "边界框: (x1 = " << vPointDatas[i].stBoxs.nX1 << ", y1 = " << vPointDatas[i].stBoxs.nY1
                  << ", x2 = " << vPointDatas[i].stBoxs.nX2 << ", y2 = " << vPointDatas[i].stBoxs.nY2 << ")\n";

        for (size_t jj = 0; jj < vPointDatas[i].vPoints.size(); jj++)
        {
            printf("第 %d 个点坐标：[%d ,%d]\n", jj, vPointDatas[i].vPoints[jj].nX, vPointDatas[i].vPoints[jj].nY);
        }
    }
    std::cout << "\n";
}

void printResult(const ot_sample_svp_rect_info &stRectInfo)
{
    printf("检测到 %d 个移动物体\n", stRectInfo.num);
    printf("灵敏度 %f\n", stRectInfo.sensitivity);
    /* 处理检测到的矩形区域 */
    for (size_t i = 0; i < stRectInfo.num; i++)
    {
        printf("Rect %d: (%d,%d) (%d,%d) (%d,%d) (%d,%d)\n", i,
               stRectInfo.rect[i].point[0].x, stRectInfo.rect[i].point[0].y,
               stRectInfo.rect[i].point[1].x, stRectInfo.rect[i].point[1].y,
               stRectInfo.rect[i].point[2].x, stRectInfo.rect[i].point[2].y,
               stRectInfo.rect[i].point[3].x, stRectInfo.rect[i].point[3].y);
    }
    printf("\n");
    fflush(stdout);
}

/**
 * @brief 打印画线规则
 * @param ruleInfo 
 */
void printRuleInfo(const Event::RuleInfo_S& stRuleInfo)
{
    std::cout << "Channel ID: " << stRuleInfo.nChnId << "\n";
    std::cout << "Type: " << static_cast<int>(stRuleInfo.enType) << "\n";

    std::cout << "Lines:\n";
    for (const auto& line : stRuleInfo.lines)
    {
        std::cout << "  Line:\n";
        for (const auto& point : line)
        {
            std::cout << "    Point(X: " << point.nX << ", Y: " << point.nY << ")\n";
        }
    }

    std::cout << "Areas:\n";
    for (const auto& area : stRuleInfo.areas)
    {
        std::cout << "  Area:\n";
        for (const auto& point : area)
        {
            std::cout << "    Point(X: " << point.nX << ", Y: " << point.nY << ")\n";
        }
    }
}

/* 打印 AlgorithmConfig_S */
void printAlgoCfg(const Event::AlgorithmConfig_S &config)
{
    dlog_debug("ai_app: 更新算法配置: "
               "普通[%d %d %d %d %d %d %d %d] "
               "周界[%d %d %d %d] "
               "Smart[%d %d %d %d %d %d %d %d %d %d %d]",

               /* 普通事件 */
               config.nEnMotionDetect, config.nEnOcclusionDetect,
               config.nEnAnomalyAlarm, config.nEnAudioAlarm,
               config.nEnAlarmInput, config.nEnAlarmOutput,
               config.nEnFlashAlarm, config.nEnPIRAlarm,

               /* 周界事件 */
               config.nEnLineCrossing, config.nEnIntrusion,
               config.nEnEnterRegion, config.nEnLeaveRegion,

               /* Smart事件 */
               config.nEnAudioAnomaly, config.nEnSceneChange,
               config.nEnFaceDetect, config.nEnLoiteringDetect,
               config.nEnCrowdGathering, config.nEnParkingDetect,
               config.nEnUnattendedObject, config.nEnObjectRemoval,
               config.nEnPetRecognition, config.nEnFaceCapture, config.nEnFaceCompare);
#if CAP_AI_PEOPLE_STATISTICS
    dlog_debug("ai_app: 人数统计配置: 人流统计[%d] 人员密度[%d]",
               config.nEnPeopleFlowStatistics,
               config.nEnPeopleDensityDetection);
#endif
}

/**
 * @brief 区域判断
 * @param pt 
 * @param polygon 
 */
// bool isPointInPolygon(const cv::Point2f& pt, const Event::Area& polygon)
// {
//     if (polygon.empty())
//     {
//         return false;
//     }
    
//     std::vector<cv::Point2f> cvPolygon;
//     for (const auto& p : polygon)
//     {
//         cvPolygon.emplace_back(p.nX, p.nY);
//     }
    
//     double d = cv::pointPolygonTest(cvPolygon, pt, false);
//     return (d >= 0);
// }


/**
 * @brief 坐标比例放大
 * @param scaleFactor 
 */
void pointScaleUp(int& x1, int& y1, int& x2, int& y2, int max_w, int max_h, double scaleFactor)
{
    int width = x2 - x1;
    int height = y2 - y1;

    int newWidth = static_cast<int>(width * scaleFactor);
    int newHeight = static_cast<int>(height * scaleFactor);

    /* 新的左上角坐标, 确保新矩形不会超出图像边界 */
    x1 = std::max(0, x1 - (newWidth - width) / 2);
    y1 = std::max(0, y1 - (newHeight - height) / 2);

    x2 = std::min(x1 + newWidth, max_w);
    y2 = std::min(y1 + newHeight, max_h);
}



/* 发送请求结果通知control */
void SendResToControl(std::string& strData, int nActionCode, int nRetCode)
{
    CStreamServer::instance()->fill_returnHead(strData, nActionCode, nRetCode);
    CStreamServer::instance()->send(strData, nActionCode);

    if (nRetCode != 0)
    {
        dlog_debug("ai_app: strData: %d %s\n", nRetCode, strData.c_str());
    }
}


void send_detectionResult_to_osd(const int nWidth, const int nHeight, const std::vector<Inference_NS::BoxData_S> &vBoxDatas)
{
    std::vector<Common::RectInfo_S> vstRectInfo;
    for (size_t i = 0; i < vBoxDatas.size(); i++)
    {
        Common::RectInfo_S stInfo;
        stInfo.nX1 = vBoxDatas[i].stBoxs.nX1;
        stInfo.nY1 = vBoxDatas[i].stBoxs.nY1;
        stInfo.nX2 = vBoxDatas[i].stBoxs.nX2;
        stInfo.nY2 = vBoxDatas[i].stBoxs.nY2;
        vstRectInfo.emplace_back(stInfo);
    }

    /* 使用结果管理器 */
    CResultManager::instance()->addDetectionResult(CResultManager::ALGORITHM_BOX, nWidth, nHeight, vstRectInfo);
}

void send_detectionResult_to_osd(const int nWidth, const int nHeight, const std::vector<Inference_NS::PointData_S> &vPointDatas)
{
    std::vector<Common::RectInfo_S> vstRectInfo;
    for (size_t i = 0; i < vPointDatas.size(); i++)
    {
        Common::RectInfo_S stInfo;
        stInfo.nX1 = vPointDatas[i].stBoxs.nX1;
        stInfo.nY1 = vPointDatas[i].stBoxs.nY1;
        stInfo.nX2 = vPointDatas[i].stBoxs.nX2;
        stInfo.nY2 = vPointDatas[i].stBoxs.nY2;
        vstRectInfo.emplace_back(stInfo);
    }

    /* 使用结果管理器 */
    CResultManager::instance()->addDetectionResult(CResultManager::ALGORITHM_POINT, nWidth, nHeight, vstRectInfo);
}

void send_detectionResult_to_osd(const int nWidth, const int nHeight, const ot_sample_svp_rect_info &stRectInfo)
{
    std::vector<Common::RectInfo_S> vstRectInfo;

    if (stRectInfo.num == 0)
    {
        COsdManage::instance()->send_detection_result(nWidth, nHeight, vstRectInfo);
        return;
    }

    /* 找到面积最大的矩形 */
    size_t maxAreaIndex = 0;
    int maxArea = 0;

    for (size_t i = 0; i < stRectInfo.num; i++)
    {
        /* 计算矩形面积：宽度 * 高度 */
        /* point[0]=左上角, point[2]=右下角 */
        int width = stRectInfo.rect[i].point[2].x - stRectInfo.rect[i].point[0].x;
        int height = stRectInfo.rect[i].point[2].y - stRectInfo.rect[i].point[0].y;
        int area = width * height;

        if (area > maxArea)
        {
            maxArea = area;
            maxAreaIndex = i;
        }
    }

    /* 只添加面积最大的矩形 */
    Common::RectInfo_S stInfo;
    stInfo.nX1 = stRectInfo.rect[maxAreaIndex].point[0].x; // 左上角x
    stInfo.nY1 = stRectInfo.rect[maxAreaIndex].point[0].y; // 左上角y
    stInfo.nX2 = stRectInfo.rect[maxAreaIndex].point[2].x; // 右下角x
    stInfo.nY2 = stRectInfo.rect[maxAreaIndex].point[2].y; // 右下角y
    vstRectInfo.emplace_back(stInfo);

    /* 使用结果管理器 */
    CResultManager::instance()->addDetectionResult(CResultManager::ALGORITHM_MOTION, nWidth, nHeight, vstRectInfo);
}

void send_detectionResult_to_osd(const int nWidth, const int nHeight, const ot_aidetect_result_array &stResult)
{
    std::vector<Common::RectInfo_S> vstRectInfo;
    uint32_t i = 0, j = 0;

    for (i = 0; i < stResult.class_num; ++i)
    {
        for (j = 0; j < stResult.object_class[i].object_num; j++)
        {
            Common::RectInfo_S stInfo;
            stInfo.nX1 = stResult.object_class[i].objects[j].detect_rect.x;
            stInfo.nY1 = stResult.object_class[i].objects[j].detect_rect.y;
            stInfo.nX2 = stResult.object_class[i].objects[j].detect_rect.width + stInfo.nX1;
            stInfo.nY2 = stResult.object_class[i].objects[j].detect_rect.height + stInfo.nY1;
            vstRectInfo.emplace_back(stInfo);
        }
    }

    /* 使用结果管理器 */
    CResultManager::instance()->addDetectionResult(CResultManager::ALGORITHM_AI, nWidth, nHeight, vstRectInfo);
}


// void send_detectionResult_to_osd(const int nWidth, const int nHeight, const std::vector<Common::RectInfo_S> &vstRectInfo)
// {
//     /* 使用结果管理器 */
//     CResultManager::instance()->addDetectionResult(CResultManager::ALGORITHM_OSD, nWidth, nHeight, vstRectInfo);
// }

// /**
//  * @brief   : 发送展会面板结果至OSD模块
//  * @param    {const OsdPanel::PanelFrame_S &} stPanelFrame：展会面板结果
//  * @return   {void}
//  * @note    : 非展会版本下编译宏关闭时，该接口静默忽略
//  */
// void send_panelResult_to_osd(const OsdPanel::PanelFrame_S &stPanelFrame)
// {
// #if CAP_EXHIBITION_OSD_PANEL
//     COsdManage::instance()->send_panel_result(stPanelFrame);
// #else
//     (void)stPanelFrame;
// #endif
// }

// void convert_gridRegion_to_rect(const std::vector<std::vector<unsigned int>> &vRegion, const int nWidth, const int nHeight, Common::Rect_S &stRect)
// {

void send_detectionResult_to_osd(const int nWidth, const int nHeight, const std::vector<Common::RectInfo_S> &vstRectInfo)
{
    /* 使用结果管理器 */
    CResultManager::instance()->addDetectionResult(CResultManager::ALGORITHM_OSD, nWidth, nHeight, vstRectInfo);
}

#if CAP_EXHIBITION_OSD_PANEL
bool is_exhibition_panel_supported(Event::Type_E enEventType)
{
    switch (enEventType)
    {
    case Event::Type_E::INTRUSION:
    case Event::Type_E::LOITERING_DETECT:
    case Event::Type_E::CROWD_GATHERING:
    case Event::Type_E::UNATTENDED_OBJECT:
    case Event::Type_E::OBJECT_REMOVAL:
        return true;
    default:
        return false;
    }
}

bool prepare_exhibition_panel_frame(OsdPanel::PanelFrame_S *pstPanelFrame,
                                    Event::Type_E enEventType,
                                    int nWidth,
                                    int nHeight)
{
    if (!pstPanelFrame || !is_exhibition_panel_supported(enEventType))
    {
        return false;
    }

    pstPanelFrame->clear();
    pstPanelFrame->enEventType = enEventType;
    pstPanelFrame->nWidth = nWidth;
    pstPanelFrame->nHeight = nHeight;
    return true;
}

std::string get_exhibition_panel_percent_text(int nPercent)
{
    nPercent = std::max(0, std::min(100, nPercent));
    return std::to_string(nPercent) + "%";
}

std::string get_exhibition_panel_confidence_text(float fConfidence)
{
    const int nPercent = static_cast<int>(std::round(fConfidence * 100.0f));
    return get_exhibition_panel_percent_text(nPercent);
}

std::string get_exhibition_panel_status_text(bool bAlarm)
{
    return bAlarm ? "已触发报警" : "未触发报警";
}

int build_exhibition_panel_priority(bool bAlarm, float fConfidence)
{
    /* 将浮点置信度放大为整数，便于与报警优先级统一比较 */
    int nConfidenceScore = static_cast<int>(std::round(fConfidence * 1000.0f));
    nConfidenceScore = std::max(0, std::min(1000, nConfidenceScore));
    return (bAlarm ? EXHIBITION_ALARM_PRIORITY_BASE : 0) + nConfidenceScore;
}

Common::RectInfo_S to_exhibition_panel_rect(const ot_aidetect_object &stObject)
{
    Common::RectInfo_S stRectInfo;
    stRectInfo.nX1 = stObject.detect_rect.x;
    stRectInfo.nY1 = stObject.detect_rect.y;
    stRectInfo.nX2 = stObject.detect_rect.x + stObject.detect_rect.width;
    stRectInfo.nY2 = stObject.detect_rect.y + stObject.detect_rect.height;
    return stRectInfo;
}

Common::RectInfo_S to_exhibition_panel_rect(const Inference_NS::Box_S &stBox)
{
    Common::RectInfo_S stRectInfo;
    stRectInfo.nX1 = stBox.nX1;
    stRectInfo.nY1 = stBox.nY1;
    stRectInfo.nX2 = stBox.nX2;
    stRectInfo.nY2 = stBox.nY2;
    return stRectInfo;
}

Common::RectInfo_S to_exhibition_panel_rect(const Alarm::Region_S &stRegion)
{
    Common::RectInfo_S stRectInfo;
    const size_t nPointCount = std::min(static_cast<size_t>(stRegion.nPointNum), stRegion.aPoint.size());
    if (nPointCount == 0)
    {
        return stRectInfo;
    }

    float fMinX = std::numeric_limits<float>::max();
    float fMinY = std::numeric_limits<float>::max();
    float fMaxX = std::numeric_limits<float>::lowest();
    float fMaxY = std::numeric_limits<float>::lowest();

    for (size_t i = 0; i < nPointCount; ++i)
    {
        fMinX = std::min(fMinX, stRegion.aPoint[i].fX);
        fMinY = std::min(fMinY, stRegion.aPoint[i].fY);
        fMaxX = std::max(fMaxX, stRegion.aPoint[i].fX);
        fMaxY = std::max(fMaxY, stRegion.aPoint[i].fY);
    }

    stRectInfo.nX1 = static_cast<int>(fMinX);
    stRectInfo.nY1 = static_cast<int>(fMinY);
    stRectInfo.nX2 = static_cast<int>(fMaxX);
    stRectInfo.nY2 = static_cast<int>(fMaxY);
    return stRectInfo;
}

void upsert_exhibition_panel_item(OsdPanel::PanelFrame_S *pstPanelFrame,
                                  const OsdPanel::PanelItem_S &stCandidate)
{
    if (!pstPanelFrame || stCandidate.empty())
    {
        return;
    }

    auto it = std::find_if(pstPanelFrame->vecItems.begin(),
                           pstPanelFrame->vecItems.end(),
                           [&stCandidate](const OsdPanel::PanelItem_S &stItem)
                           { return stItem.nSortKey == stCandidate.nSortKey; });
    if (it == pstPanelFrame->vecItems.end())
    {
        pstPanelFrame->vecItems.emplace_back(stCandidate);
        return;
    }

    if (stCandidate.nPriority >= it->nPriority)
    {
        *it = stCandidate;
    }
}

/**
 * @brief   : 发送展会面板结果至OSD模块
 * @param    {const OsdPanel::PanelFrame_S &} stPanelFrame：展会面板结果
 * @return   {void}
 * @note    : 非展会版本下编译宏关闭时，该接口静默忽略
 */
void send_panelResult_to_osd(const OsdPanel::PanelFrame_S &stPanelFrame)
{
    COsdManage::instance()->send_panel_result(stPanelFrame);
}
#endif

void convert_gridRegion_to_rect(const std::vector<std::vector<unsigned int>> &vRegion, const int nWidth, const int nHeight, Common::Rect_S &stRect)
{
    /* 初始化边界值 */
    int minRow = GRID_HEIGHT_DEFAULT;
    int maxRow = -1;
    int minCol = GRID_WIDTH_DEFAULT;
    int maxCol = -1;
    bool foundRegion = false;

    /* 遍历网格找到所有非零区域的边界 */
    for (int row = 0; row < GRID_HEIGHT_DEFAULT; ++row)
    {
        for (int col = 0; col < GRID_WIDTH_DEFAULT; ++col)
        {
            if (vRegion[row][col] != 0)
            {
                foundRegion = true;
                minRow = std::min(minRow, row);
                maxRow = std::max(maxRow, row);
                minCol = std::min(minCol, col);
                maxCol = std::max(maxCol, col);
            }
        }
    }

    /* 如果没有找到任何区域，设置为默认值 */
    if (!foundRegion)
    {
        stRect.nX = 0;
        stRect.nY = 0;
        stRect.nWidth = 0;
        stRect.nHeight = 0;
        return;
    }

    // dlog_debug("src: [%d,%d] [%d,%d]", stRect.nX, stRect.nY, stRect.nWidth, stRect.nHeight);
    /* 计算每个网格单元在实际画面中的尺寸 */
    double cellWidth = static_cast<double>(nWidth) / GRID_WIDTH_DEFAULT;
    double cellHeight = static_cast<double>(nHeight) / GRID_HEIGHT_DEFAULT;

    /* 转换网格坐标为实际画面坐标 */
    stRect.nX = static_cast<int>(minCol * cellWidth);
    stRect.nY = static_cast<int>(minRow * cellHeight);
    stRect.nWidth = static_cast<int>((maxCol - minCol + 1) * cellWidth);
    stRect.nHeight = static_cast<int>((maxRow - minRow + 1) * cellHeight);
    // dlog_debug("dst: [%d,%d] [%d,%d]", stRect.nX, stRect.nY, stRect.nWidth, stRect.nHeight);
}

void convert_region_ratio(Common::RectInfo_S &stRectInfo, const float fRatio, const int nImageWidth, const int nImageHeight)
{
    /* 计算原始矩形的中心点 */
    int nCenterX = (stRectInfo.nX1 + stRectInfo.nX2) / 2;
    int nCenterY = (stRectInfo.nY1 + stRectInfo.nY2) / 2;
    
    /* 计算原始宽高 */
    int nOrigWidth = stRectInfo.nX2 - stRectInfo.nX1;
    int nOrigHeight = stRectInfo.nY2 - stRectInfo.nY1;
    
    /* 计算缩放后的新宽高 */
    int nNewWidth = static_cast<int>(nOrigWidth * fRatio);
    int nNewHeight = static_cast<int>(nOrigHeight * fRatio);
    
    /* 以中心点为基准，计算新的左上角和右下角坐标 */
    int nNewX1 = nCenterX - nNewWidth / 2;
    int nNewY1 = nCenterY - nNewHeight / 2;
    int nNewX2 = nNewX1 + nNewWidth;
    int nNewY2 = nNewY1 + nNewHeight;
    
    /* 边界检查：确保矩形不超出图像范围 */
    /* 如果缩放后的宽度超过图像宽度，限制宽度 */
    if (nNewWidth > nImageWidth)
    {
        nNewWidth = nImageWidth;
        nNewX1 = 0;
        nNewX2 = nImageWidth;
    }
    else
    {
        /* 左边界检查 */
        if (nNewX1 < 0)
        {
            nNewX1 = 0;
            nNewX2 = nNewX1 + nNewWidth;
        }
        /* 右边界检查 */
        if (nNewX2 > nImageWidth)
        {
            nNewX2 = nImageWidth;
            nNewX1 = nNewX2 - nNewWidth;
        }
    }

    /* 如果缩放后的高度超过图像高度，限制高度 */
    if (nNewHeight > nImageHeight)
    {
        nNewHeight = nImageHeight;
        nNewY1 = 0;
        nNewY2 = nImageHeight;
    }
    else
    {
        /* 上边界检查 */
        if (nNewY1 < 0)
        {
            nNewY1 = 0;
            nNewY2 = nNewY1 + nNewHeight;
        }
        /* 下边界检查 */
        if (nNewY2 > nImageHeight)
        {
            nNewY2 = nImageHeight;
            nNewY1 = nNewY2 - nNewHeight;
        }
    }

    /* 更新矩形坐标 */
    stRectInfo.nX1 = nNewX1;
    stRectInfo.nY1 = nNewY1;
    stRectInfo.nX2 = nNewX2;
    stRectInfo.nY2 = nNewY2;
}

/* 将用户输入坐标映射到 640×384 算法坐标 */
void mapUserToAlgo(const Common::Rect_S& userIn,Common::Rect_S& algoOut)
{
    float scaleX = static_cast<float>(PIXEL_WIDTH_640) / PIXEL_WIDTH_1920;
    float scaleY = static_cast<float>(PIXEL_HEIGHT_384) / PIXEL_HEIGHT_1080;

    algoOut.nX      = static_cast<int>(userIn.nX      * scaleX);
    algoOut.nY      = static_cast<int>(userIn.nY      * scaleY);
    algoOut.nWidth  = static_cast<int>(userIn.nWidth  * scaleX);
    algoOut.nHeight = static_cast<int>(userIn.nHeight * scaleY);
}

bool is_in_region(const Alarm::Region_S &stRegion, const ot_aidetect_object &stObject)
{
    Common::PosF_S stResultPoint;
    /* 取检测框的中心点 */
    stResultPoint.fX = (stObject.detect_rect.x + stObject.detect_rect.x + stObject.detect_rect.width) / 2.0f;
    stResultPoint.fY = (stObject.detect_rect.y + stObject.detect_rect.y + stObject.detect_rect.height) / 2.0f;

    int nCount = 0;
    for (size_t i = 0; i < stRegion.aPoint.size(); ++i)
    {
        const Common::PosF_S &Point1 = stRegion.aPoint[i];
        const Common::PosF_S &Point2 = stRegion.aPoint[(i + 1) % stRegion.aPoint.size()];
        /* 判断交点数 */
        if (((Point1.fY > stResultPoint.fY) != (Point2.fY > stResultPoint.fY)))
        {
            float fXIntersect = (Point2.fX - Point1.fX) * (stResultPoint.fY - Point1.fY) / (Point2.fY - Point1.fY + 1e-10f) + Point1.fX;
            if (stResultPoint.fX < fXIntersect)
            {
                nCount++;
            }
        }
    }

    return (nCount % 2 == 1) ? true : false;
}

bool is_in_region(const Alarm::Region_S &stRegion, const Inference_NS::Box_S &stBox)
{
    Common::PosF_S stResultPoint;
    /* 取检测框的中心点 */
    stResultPoint.fX = (stBox.nX1 + stBox.nX2) / 2.0f;
    stResultPoint.fY = (stBox.nY1 + stBox.nY2) / 2.0f;

    int nCount = 0;
    const float EPSILON = 1e-6f; /* 浮点数比较精度阈值 */

    for (size_t i = 0; i < stRegion.aPoint.size(); ++i)
    {
        const Common::PosF_S &Point1 = stRegion.aPoint[i];
        const Common::PosF_S &Point2 = stRegion.aPoint[(i + 1) % stRegion.aPoint.size()];

        /* 跳过水平边 */
        float fDeltaY = Point2.fY - Point1.fY;
        if (std::abs(fDeltaY) < EPSILON)
        {
            continue;
        }

        /* 判断点是否在边的Y范围内 */
        if (((Point1.fY > stResultPoint.fY) != (Point2.fY > stResultPoint.fY)))
        {
            float fXIntersect = (Point2.fX - Point1.fX) * (stResultPoint.fY - Point1.fY) / fDeltaY + Point1.fX;
            if (stResultPoint.fX < fXIntersect)
            {
                nCount++;
            }
        }
    }

    return (nCount % 2 == 1);
}

bool is_in_region_debug(const Alarm::Region_S &stRegion, const Inference_NS::Box_S &stBox)
{
    Common::PosF_S stResultPoint;
    /* 取检测框的中心点 */
    stResultPoint.fX = (stBox.nX1 + stBox.nX2) / 2.0f;
    stResultPoint.fY = (stBox.nY1 + stBox.nY2) / 2.0f;

    printf("=== 区域检测调试信息 ===\n");
    printf("检测框: [%d,%d][%d,%d]\n", stBox.nX1, stBox.nY1, stBox.nX2, stBox.nY2);
    printf("中心点: (%.2f, %.2f)\n", stResultPoint.fX, stResultPoint.fY);
    printf("区域点数: %zu\n", stRegion.aPoint.size());
    
    /* 打印所有区域顶点 */
    for (size_t i = 0; i < stRegion.aPoint.size(); ++i)
    {
        printf("顶点%zu: (%.2f, %.2f)\n", i, stRegion.aPoint[i].fX, stRegion.aPoint[i].fY);
    }

    int nCount = 0;
    const float EPSILON = 1e-6f;
    
    for (size_t i = 0; i < stRegion.aPoint.size(); ++i)
    {
        const Common::PosF_S &Point1 = stRegion.aPoint[i];
        const Common::PosF_S &Point2 = stRegion.aPoint[(i + 1) % stRegion.aPoint.size()];
        
        printf("\n--- 边%zu: (%.2f,%.2f) -> (%.2f,%.2f) ---\n", 
            i, Point1.fX, Point1.fY, Point2.fX, Point2.fY);
        
        float fDeltaY = Point2.fY - Point1.fY;
        printf("DeltaY: %.6f\n", fDeltaY);
        
        /* 跳过水平边 */
        if (std::abs(fDeltaY) < EPSILON)
        {
            printf("跳过水平边\n");
            continue;
        }
        
        /* 判断测试点是否在边的Y范围内 */
        bool bInYRange = ((Point1.fY > stResultPoint.fY) != (Point2.fY > stResultPoint.fY));
        printf("Y范围检查: Point1.fY(%.2f) > TestY(%.2f) = %d, Point2.fY(%.2f) > TestY(%.2f) = %d\n",
            Point1.fY, stResultPoint.fY, Point1.fY > stResultPoint.fY,
            Point2.fY, stResultPoint.fY, Point2.fY > stResultPoint.fY);
        printf("在Y范围内: %d\n", bInYRange);
        
        if (bInYRange)
        {
            float fXIntersect = (Point2.fX - Point1.fX) * (stResultPoint.fY - Point1.fY) / fDeltaY + Point1.fX;
            printf("交点X坐标: %.2f, 测试点X: %.2f\n", fXIntersect, stResultPoint.fX);
            
            if (stResultPoint.fX < fXIntersect)
            {
                nCount++;
                printf("交点在测试点右侧,计数+1,当前计数: %d\n", nCount);
            }
            else
            {
                printf("交点在测试点左侧,不计数\n");
            }
        }
    }

    printf("\n=== 最终结果 ===\n");
    printf("交点总数: %d\n", nCount);
    printf("结果: %s\n", (nCount % 2 == 1) ? "在区域内(true)" : "不在区域内(false)");
    
    return (nCount % 2 == 1);
}

int crossProduct(const Common::PosF_S &alertLineStart,
                 const Common::PosF_S &alertLineEnd,
                 const Common::PosF_S &testPoint)
{
    return (alertLineEnd.fX - alertLineStart.fX) * (testPoint.fY - alertLineStart.fY) -
           (alertLineEnd.fY - alertLineStart.fY) * (testPoint.fX - alertLineStart.fX);
}

bool isBoundingBoxIntersecting(const Common::PosF_S &lineA1,
                               const Common::PosF_S &lineA2,
                               const Common::PosF_S &lineB1,
                               const Common::PosF_S &lineB2)
{
    return std::max(lineA1.fX, lineA2.fX) >= std::min(lineB1.fX, lineB2.fX) &&
           std::max(lineB1.fX, lineB2.fX) >= std::min(lineA1.fX, lineA2.fX) &&
           std::max(lineA1.fY, lineA2.fY) >= std::min(lineB1.fY, lineB2.fY) &&
           std::max(lineB1.fY, lineB2.fY) >= std::min(lineA1.fY, lineA2.fY);
}

/* 判断是否跨越线段 */
bool doLinesIntersect(const Common::PosF_S &p1,
                      const Common::PosF_S &q1,
                      const Common::PosF_S &p2,
                      const Common::PosF_S &q2)
{
    int d1 = crossProduct(p2, q2, p1);
    int d2 = crossProduct(p2, q2, q1);
    int d3 = crossProduct(p1, q1, p2);
    int d4 = crossProduct(p1, q1, q2);

    /* 如果两条线段相交 */
    if (((d1 > 0 && d2 < 0) || (d1 < 0 && d2 > 0)) && ((d3 > 0 && d4 < 0) || (d3 < 0 && d4 > 0)))
        return true;

    /* 处理共线的特殊情况 */
    if (d1 == 0 && d2 == 0 && d3 == 0 && d4 == 0)
    {
        return isBoundingBoxIntersecting(p1, q1, p2, q2);
    }

    return false;
}

/* 判断两条线段是否有交点 */
Alarm::CrossDirection_E tripLineDetection(
 const Common::PosF_S &startPoint,
 const Common::PosF_S &lastPoint,
 const Common::PosF_S &alertLineFirst,
 const Common::PosF_S &alertLineSecond)
{
    Alarm::CrossDirection_E enType = Alarm::CROSS_DIRECTION_INVALID;

    /* 首先检查包围盒是否相交，避免无交点线段进入后续方向判断 */
    if (!isBoundingBoxIntersecting(startPoint, lastPoint, alertLineFirst, alertLineSecond))
    {
        return enType;
    }

    /* 检查线段是否真的相交，只有实际穿越警戒线才计算方向 */
    if (!doLinesIntersect(startPoint, lastPoint, alertLineFirst, alertLineSecond))
    {
        return enType;
    }

    /* 以警戒线起终点作为固定方向，判断运动轨迹从线的一侧移动到另一侧 */
    const int crossStartPoint = crossProduct(alertLineFirst, alertLineSecond, startPoint);
    const int crossLastPoint = crossProduct(alertLineFirst, alertLineSecond, lastPoint);
    if (crossStartPoint < 0 && crossLastPoint > 0)
    {
        enType = Alarm::A_TO_B;
    }
    else if (crossStartPoint > 0 && crossLastPoint < 0)
    {
        enType = Alarm::B_TO_A;
    }
    else if (crossStartPoint == 0 || crossLastPoint == 0)
    {
        /* 起点或终点在线上时无法稳定区分方向，返回双向供上层按需处理 */
        enType = Alarm::BOTH_WAYS;
    }

    switch (enType)
    {
    case Alarm::CrossDirection_E::BOTH_WAYS:
        dlog_debug("穿越方向: 双向");
        break;
    case Alarm::CrossDirection_E::A_TO_B:
        dlog_debug("穿越方向: A->B");
        break;
    case Alarm::CrossDirection_E::B_TO_A:
        dlog_debug("穿越方向: B->A");
        break;
    case Alarm::CrossDirection_E::CROSS_DIRECTION_INVALID:
    default:
        dlog_debug("异常穿越方向");
        break;
    }

    return enType;
}

int add_result_to_vector(const ot_aidetect_object &stObject, std::vector<Common::RectInfo_S> &vstRectInfo)
{
    Common::RectInfo_S stInfo;
    stInfo.nX1 = stObject.detect_rect.x;
    stInfo.nY1 = stObject.detect_rect.y;
    stInfo.nX2 = stObject.detect_rect.width + stInfo.nX1;
    stInfo.nY2 = stObject.detect_rect.height + stInfo.nY1;
    vstRectInfo.emplace_back(stInfo);
    return OK;
}

int add_result_to_vector(const Inference_NS::BoxData_S &stBoxDatas, std::vector<Common::RectInfo_S> &vstRectInfo)
{
    Common::RectInfo_S stInfo;
    stInfo.nX1 = stBoxDatas.stBoxs.nX1;
    stInfo.nY1 = stBoxDatas.stBoxs.nY1;
    stInfo.nX2 = stBoxDatas.stBoxs.nX2;
    stInfo.nY2 = stBoxDatas.stBoxs.nY2;
    vstRectInfo.emplace_back(stInfo);
    return OK;
}

int add_result_to_vector(const Inference_NS::PointData_S &stPointData, std::vector<Common::RectInfo_S> &vstRectInfo)
{
    Common::RectInfo_S stInfo;
    stInfo.nX1 = stPointData.stBoxs.nX1;
    stInfo.nY1 = stPointData.stBoxs.nY1;
    stInfo.nX2 = stPointData.stBoxs.nX2;
    stInfo.nY2 = stPointData.stBoxs.nY2;
    vstRectInfo.emplace_back(stInfo);
    return OK;
}

int add_result_to_vector(const Alarm::Region_S &stRegion, std::vector<Common::RectInfo_S> &vstRectInfo)
{
    /* 检查是否为四边形 */
    if (stRegion.nPointNum != 4 || !stRegion.IsValid())
    {
        return ERR;
    }

    /* 计算包围四边形的矩形 */
    Common::RectInfo_S rect;

    /* 初始化为第一个点的坐标 */
    rect.nX1 = static_cast<int>(stRegion.aPoint[0].fX);
    rect.nY1 = static_cast<int>(stRegion.aPoint[0].fY);
    rect.nX2 = rect.nX1;
    rect.nY2 = rect.nY1;

    /* 遍历所有点，找到最小和最大坐标 */
    for (size_t i = 1; i < 4; ++i)
    {
        int x = static_cast<int>(stRegion.aPoint[i].fX);
        int y = static_cast<int>(stRegion.aPoint[i].fY);

        if (x < rect.nX1)
            rect.nX1 = x;
        if (x > rect.nX2)
            rect.nX2 = x;
        if (y < rect.nY1)
            rect.nY1 = y;
        if (y > rect.nY2)
            rect.nY2 = y;
    }

    /* 添加到结果向量 */
    vstRectInfo.emplace_back(rect);

    return OK;
}
