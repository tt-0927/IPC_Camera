/**
 * @FilePath     : common_process.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2025-07-08 17:01:53
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2025-08-18 14:40:38
 * @Description  : AIAPP 公共处理函数
 */

#include "common_process.h"
#include "video_define.h"

#include <algorithm>
#include <cmath>
#include <dirent.h>
#include <filesystem>

namespace fs = std::filesystem;

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
               "Smart[%d %d %d %d %d %d %d %d %d %d]"
               "场景智能分析[%d %d %d]",

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
               config.nEnPetRecognition, config.nEnFaceCapture
            #ifdef DEVICE_TV_3882TI
               /* 场景智能分析事件 */
               ,config.nEnLLmInference,config.nEnLLmInference, 
               config.nEnAISceneAnalysis
            #endif
            );
}

void printResult(const std::vector<Common::RectInfo_S> &vstRectsInfo)
{
    printf("检测到 %d 个移动物体\n", vstRectsInfo.size());
    // printf("灵敏度 %f\n", stRectInfo.sensitivity);
    /* 处理检测到的矩形区域 */
    for (size_t i = 0; i < vstRectsInfo.size(); i++)
    {
        printf("Rect %d: 左上角坐标(%d,%d) === 右下角坐标(%d,%d)\n", i, vstRectsInfo[i].nX1, vstRectsInfo[i].nY1, vstRectsInfo[i].nX2, vstRectsInfo[i].nY2);
    }
    printf("\n");
    fflush(stdout);
}

void printResult(const std::vector<std::vector<int>> &vstRectsInfo)
{
    printf("检测到 %d 个移动物体\n", vstRectsInfo.size());
    /* 处理检测到的矩形区域 */
    for (size_t i = 0; i < vstRectsInfo.size(); i++)
    {

        printf("Rect %d: 左上角坐标(%d,%d) === 矩形长*宽(%d*%d)\n", i, vstRectsInfo[i][0], vstRectsInfo[i][1], vstRectsInfo[i][2], vstRectsInfo[i][3]);
    }
    printf("\n");
    fflush(stdout);
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

void send_detectionResult_to_osd(const int nWidth, const int nHeight, const Common::RectInfo_S &vstRectInfo)
{
    std::vector<Common::RectInfo_S> vstRectsInfo;
    vstRectsInfo.emplace_back(vstRectInfo);
    CResultManager::instance()->addDetectionResult(CResultManager::ALGORITHM_MOTION, nWidth, nHeight, vstRectsInfo);
    return ;
}

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
    case Event::Type_E::PHONE_USAGE:
    case Event::Type_E::SMOKING:
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
    int nConfidenceScore = static_cast<int>(std::round(fConfidence * 1000.0f));
    nConfidenceScore = std::max(0, std::min(1000, nConfidenceScore));
    return (bAlarm ? 100000 : 0) + nConfidenceScore;
}

Common::RectInfo_S to_exhibition_panel_rect(const Common::RectInfo_S &stRect)
{
    return stRect;
}

Common::RectInfo_S to_exhibition_panel_rect(const Alarm::Region_S &stRegion)
{
    Common::RectInfo_S stRectInfo;
    if (stRegion.aPoint.empty())
    {
        return stRectInfo;
    }

    float fMinX = stRegion.aPoint.front().fX;
    float fMaxX = stRegion.aPoint.front().fX;
    float fMinY = stRegion.aPoint.front().fY;
    float fMaxY = stRegion.aPoint.front().fY;
    for (const auto &stPoint : stRegion.aPoint)
    {
        fMinX = std::min(fMinX, stPoint.fX);
        fMaxX = std::max(fMaxX, stPoint.fX);
        fMinY = std::min(fMinY, stPoint.fY);
        fMaxY = std::max(fMaxY, stPoint.fY);
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
    /* 左边界检查 */
    if (nNewX1 < 0) {
        // int nOffset = -nNewX1;
        nNewX1 = 0;
        nNewX2 = nNewWidth;
        if (nNewX2 > nImageWidth) {
            nNewX2 = nImageWidth;
        }
    }
    
    /* 上边界检查 */
    if (nNewY1 < 0) {
        // int nOffset = -nNewY1;
        nNewY1 = 0;
        nNewY2 = nNewHeight;
        if (nNewY2 > nImageHeight) {
            nNewY2 = nImageHeight;
        }
    }
    
    /* 右边界检查 */
    if (nNewX2 > nImageWidth) {
        nNewX2 = nImageWidth;
        nNewX1 = nImageWidth - nNewWidth;
        if (nNewX1 < 0) {
            nNewX1 = 0;
        }
    }
    
    /* 下边界检查 */
    if (nNewY2 > nImageHeight) {
        nNewY2 = nImageHeight;
        nNewY1 = nImageHeight - nNewHeight;
        if (nNewY1 < 0) {
            nNewY1 = 0;
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

bool is_in_region(const Alarm::Region_S &stRegion, const Common::RectInfo_S &stObject)
{
    Common::PosF_S stResultPoint;
    /* 取检测框的中心点 */
    stResultPoint.fX = (stObject.nX1 + stObject.nX2) / 2.0f;
    stResultPoint.fY = (stObject.nY1 + stObject.nY2) / 2.0f;

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

int crossProduct(const Common::PosF_S& alertLineStart,
    const Common::PosF_S& alertLineEnd,
    const Common::PosF_S& testPoint)
{
    return (alertLineEnd.fX - alertLineStart.fX) * (testPoint.fY - alertLineStart.fY) -
    (alertLineEnd.fY - alertLineStart.fY) * (testPoint.fX - alertLineStart.fX);
}

bool isBoundingBoxIntersecting(
    const Common::PosF_S &lineA1,
    const Common::PosF_S &lineA2,
    const Common::PosF_S &lineB1,
    const Common::PosF_S &lineB2)
{
    return std::max(lineA1.fX, lineA2.fX) >= std::min(lineB1.fX, lineB2.fX) &&
           std::max(lineB1.fX, lineB2.fX) >= std::min(lineA1.fX, lineA2.fX) &&
           std::max(lineA1.fY, lineA2.fY) >= std::min(lineB1.fY, lineB2.fY) &&
           std::max(lineB1.fY, lineB2.fY) >= std::min(lineA1.fY, lineA2.fY);
}

/* 判断两条线段是否有交点 */
Alarm::CrossDirection_E tripLineDetection(
 const Common::PosF_S &startPoint,
 const Common::PosF_S &lastPoint,
 const Common::PosF_S &alertLineFirst,
 const Common::PosF_S &alertLineSecond)
{
    Alarm::CrossDirection_E enType;
    if (!isBoundingBoxIntersecting(startPoint, lastPoint,alertLineFirst, alertLineSecond))
    {
        return enType;
    }

    Common::PosF_S lineStartPoint;
    Common::PosF_S lineEndPoint;

    /* 判断线段的情况 */
    if (alertLineFirst.fX == alertLineSecond.fX)
    {
        if (alertLineFirst.fY < alertLineSecond.fY)
        {
            lineStartPoint = alertLineSecond;
            lineEndPoint = alertLineFirst;
        }
        else
        {
            lineStartPoint = alertLineFirst;
            lineEndPoint = alertLineSecond;
        }
    }
    else
    {
        if (alertLineFirst.fX < alertLineSecond.fX)
        {
            lineStartPoint = alertLineFirst;
            lineEndPoint = alertLineSecond;
        }
        else 
        {
            lineStartPoint = alertLineSecond;
            lineEndPoint = alertLineFirst;
        }
    }

    /* 计算叉积 */
    int crossStartPoint = crossProduct(lineStartPoint,lineEndPoint, startPoint);
    int crossLastPoint = crossProduct(lineStartPoint, lineEndPoint,lastPoint);
    /* 判断A->B */
    if (crossStartPoint < 0 && crossLastPoint > 0)
    {
        /* 拌线方向A->B */
        enType = Alarm::A_TO_B;
        dlog_debug("拌线方向A->B");
    }

    /* 判断B->A */
    if (crossStartPoint > 0 && crossLastPoint < 0)
    {
        /* 拌线方向B->A */
        enType = Alarm::B_TO_A;
        dlog_debug("拌线方向B->A");
    }

    /* 判断B<->A */
    if (crossStartPoint > 0 && crossLastPoint < 0)
    {
        /* 拌线方向B<->A*/
        enType = Alarm::BOTH_WAYS;
        dlog_debug("拌线方向B<->A");
    }

    return enType;
}



/**
 * @brief 将图片从源路径移动到目标路径，并更新配置中的路径
 * @param targetBasePath 目标路径的基础目录
 * @param ImagePath 图片路径加文件名
 * @return 0表示成功, -1表示失败
 */
int moveImageToAnalysisDir(const std::string& targetBasePath,std::string& ImagePath)
{ 
    try {
        // 检查原文件是否存在
        if (!fs::exists(ImagePath)) {
            std::cout << "原文件不存在: " << ImagePath << std::endl;
            return -1;
        }
        
        // 提取文件名
        fs::path sourceFilePath(ImagePath);
        std::string filename = sourceFilePath.filename().string();
        
        // 构建目标目录和路径
        std::string targetPath = targetBasePath + filename;

        // 检查源路径和目标路径是否相同
        fs::path absSourcePath = fs::absolute(ImagePath);
        fs::path absTargetPath = fs::absolute(targetPath);
       
        if (absSourcePath == absTargetPath) {
            std::cout << "源路径和目标路径相同，无需移动: " << ImagePath << std::endl;
            return 0; 
        }
        
        // 创建目标目录（如果不存在）
        if (!fs::exists(targetBasePath)) {
            fs::create_directories(targetBasePath);
        }
    
        // 检查目标文件是否已存在，如果存在则生成新文件名
        if (fs::exists(targetPath)) {
            std::cout << "目标文件已存在: " << targetPath << "，正在生成新文件名..." << std::endl;
            
            fs::path originalPath(targetPath);
            std::string stem = originalPath.stem().string();  /* 文件名（不含扩展名）*/
            std::string extension = originalPath.extension().string();  /* 扩展名*/
            
            int counter = 1;
            std::string newFilename;
            
            do {
                /* 生成新文件名格式：原文件名_数字.扩展名*/
                newFilename = stem + "_" + std::to_string(counter) + extension;
                targetPath = targetBasePath + newFilename;
                counter++;
            } while (fs::exists(targetPath));
            
            std::cout << "使用新文件名: " << newFilename << std::endl;
        }

       /* 尝试直接重命名移动文件*/
        try {
            fs::rename(ImagePath, targetPath);
            std::cout << "图片移动成功(重命名): " << ImagePath << " -> " << targetPath << std::endl;
        } 
        catch (const fs::filesystem_error& rename_ex) {
            /* 如果重命名失败（可能是跨文件系统），使用复制+删除方式*/
            if (std::string(rename_ex.what()).find("cross-device") != std::string::npos) {
                std::cout << "检测到跨文件系统移动，使用复制+删除方式..." << std::endl;
                
                /* 复制文件*/
                fs::copy(ImagePath, targetPath, fs::copy_options::overwrite_existing);
                
                /* 验证复制是否成功*/
                if (fs::exists(targetPath) && 
                    fs::file_size(ImagePath) == fs::file_size(targetPath)) {
                    /* 删除原文件*/
                    fs::remove(ImagePath);
                    std::cout << "图片移动成功(复制+删除): " << ImagePath << " -> " << targetPath << std::endl;
                } else {
                    std::cerr << "文件复制后验证失败，已回滚" << std::endl;
                    /* 复制失败，删除可能的部分复制文件*/
                    if (fs::exists(targetPath)) {
                        fs::remove(targetPath);
                    }
                    return -1;
                }
            } else {
                // 其他类型的错误，重新抛出
                throw;
            }
        }
        
        // 更新配置中的路径为新的文件名
        ImagePath = targetPath;
        
        return 0;
        
    } catch (const fs::filesystem_error& ex) {
        std::cerr << "文件移动失败: " << ex.what() << std::endl;
        return -1;
    } catch (const std::exception& ex) {
        std::cerr << "发生未知错误: " << ex.what() << std::endl;
        return -1;
    }
}

/**
 * @brief 判断文件路径是否在指定目录下
 * @param filePath 完整的文件路径
 * @param directory 目录路径
 * @return true-文件在指定目录下, false-文件不在指定目录下
 */
bool isFileInDirectory(const std::string& filePath, const std::string& directory) 
{
    try {
        /* 将路径转换为规范形式*/
        fs::path fileAbsPath = fs::absolute(fs::path(filePath));
        fs::path dirAbsPath = fs::absolute(fs::path(directory));

        /* 规范化目录路径（移除末尾斜杠）*/
        std::string dirStr = dirAbsPath.string();
        if (dirStr.length() > 1 && dirStr.back() == '/') {
            dirStr.pop_back();
        }

        std::string fileParentStr = fileAbsPath.parent_path().string();
        
        /* 检查文件父目录是否以指定目录开头*/
        return fileParentStr.find(dirStr) == 0;
        
    } catch (const std::exception& e) {
        std::cerr << "路径判断错误: " << e.what() << std::endl;
        return false;
    }
}

/**
 * @brief 提取时间秒数 (支持格式: 20260423_092635...)
 * 索引: 01234567 8 90 11 12 13 14
 *       20260423 _ 0  9  2  6  3  5
 */
static inline long long getSec(const char* s) 
{
    if (!s || strlen(s) < 15) 
    {
        return -1;
    }
    
    int h = (s[9] - '0') * 10 + (s[10] - '0');
    int m = (s[11] - '0') * 10 + (s[12] - '0');
    int sec = (s[13] - '0') * 10 + (s[14] - '0');
    return (long long)h * 3600 + m * 60 + sec;
}

/**
 * @brief 提取M3U8时间标签秒数
 * #EXT-X-PROGRAM-DATE-TIME:2026-04-22 14:05:48
 * 索引: 36,37(时) 39,40(分) 42,43(秒)
 */
static inline long long getM3U8Sec(const char* s) {
    if (!s || strlen(s) < 44) 
    {
        return -1;
    }

    int h = (s[36] - '0') * 10 + (s[37] - '0');
    int m = (s[39] - '0') * 10 + (s[40] - '0');
    int sec = (s[42] - '0') * 10 + (s[43] - '0');
    return (long long)h * 3600 + m * 60 + sec;
}

/**
 * @brief 根据日期时间找到具体视频分片ts文件
 * @param recordDir 目标路径
 * @param createTime 日期时间
 * @return 对应文件路径
 */
std::string findClosestTSByM3U8(const std::string& recordDir, const std::string& createTime)
{
    if (createTime.size() < 15) 
    {
        return "";
    }
    
    std::string dateStr = createTime.substr(0, 8);
    std::string folderPath = recordDir + "/" + dateStr;
    std::string m3u8Path = folderPath + "/normal_" + dateStr + ".m3u8";
    long long targetSec = getSec(createTime.c_str());
    if (targetSec < 0) 
    {
        return "";
    }

    /* A方案: M3U8 缓存查找 (利用 EXTINF 时长判定) */
    struct M3U8Item { long long start; float dur; std::string name; };
    struct Cache { std::vector<M3U8Item> items; time_t lastMod = 0; };
    static std::unordered_map<std::string, Cache> s_cache;

    struct stat mStat;
    if (stat(m3u8Path.c_str(), &mStat) == 0) 
    {
        Cache& cache = s_cache[dateStr];
        if (mStat.st_mtime != cache.lastMod) 
        {
            cache.items.clear();
            std::ifstream file(m3u8Path);
            std::string line;
            long long curStart = -1;
            float curDur = 60.0f; 
            while (std::getline(file, line)) 
            {
                if (line.find("#EXT-X-PROGRAM-DATE-TIME:") == 0) 
                {
                    curStart = getM3U8Sec(line.c_str());
                } 
                else if (line.find("#EXTINF:") == 0) 
                {
                    curDur = std::stof(line.substr(8));
                } 
                else if (!line.empty() && line[0] != '#' && line.find(".ts") != std::string::npos) 
                {
                    if (curStart != -1) 
                    {
                        cache.items.push_back({curStart, curDur, line});
                        curStart = -1;
                    }
                }
            }
            cache.lastMod = mStat.st_mtime;
            if(s_cache.size() > 3) 
            {
                s_cache.clear();
            } 
        }

        if (!cache.items.empty()) 
        {
            // 二分查找
            auto it = std::upper_bound(cache.items.begin(), cache.items.end(), targetSec,
                [](long long val, const M3U8Item& item) { return val < item.start; });

            if (it != cache.items.begin()) 
            {
                auto found = std::prev(it);
                //  2 秒冗余防止交界处误差
                if (targetSec >= found->start && targetSec < (found->start + (long long)found->dur + 2)) 
                {
                    return folderPath + "/" + found->name;
                }
               
                if (it != cache.items.end()) 
                {
                    return "";
                }
            }
        }
    }

    /* B方案: 磁盘扫描补偿 (针对正在录制、尚未入库的分片) */
    // 策略：如果查询时间是 09:26:35，该分片起始时间一定在 [09:25:33 ~ 09:26:35] 之间
    std::string bestFile = "";
    DIR* dir = opendir(folderPath.c_str());
    if (dir) 
    {
        struct dirent* entry;
        long long minDiff = 9999;
        // 性能优化：只过滤当前小时的文件
        std::string hourPrefix = createTime.substr(0, 11); // "20260423_09"

        while ((entry = readdir(dir)) != nullptr) 
        {
            const char* name = entry->d_name;
            // 严格过滤文件名长度（标准录制TS是18位），排除备份文件
            if (strlen(name) == 18 && strncmp(name, hourPrefix.c_str(), 11) == 0) 
            {
                long long fStart = getSec(name);
                long long diff = targetSec - fStart;
                
                // 严格包含判定：查询时间必须在分片开始后，且分片时长假定最大 62 秒
                if (diff >= 0 && diff <= 62) 
                {
                    if (diff < minDiff) 
                    {
                        minDiff = diff;
                        bestFile = name;
                    }
                }
            }
        }
        closedir(dir);
    }

    if (!bestFile.empty()) 
    {
        return folderPath + "/" + bestFile;
    }

    // 未匹配
    return ""; 
}

/**
 * @brief 构建用于多模态大模型的视觉输入Prompt
 * @param user_query [in] 用户的原始文本问题
 * @return 返回包含了<image>标签的、符合模型输入规范的完整Prompt字符串
 */
 std::string build_vision_prompt(const std::string& user_query)
 {
    /* 模型要求<image>标签来定位图像特征，通常格式是<image>\n[你的问题] */ 
    return "<image>\n" + user_query;
 }
#ifdef DEVICE_TV_3882TI
/**
 * @brief 构建用于文字预设任务的Prompt
 */
 std::string build_text_preset_prompt(const Alarm::TextPreset_S& stTextPreseCfg)
 {
     std::string prompt = "这张图片是否出现";
     
     /* 使用用户输入的字符串而不是枚举 */
     if (!stTextPreseCfg.strObjectName.empty())
     {
         prompt +=  stTextPreseCfg.strObjectName;
     }
     
     if (!stTextPreseCfg.strConditionName.empty())
     {
         prompt += stTextPreseCfg.strConditionName;
     }
     
     prompt += "的情况?只回答是或否，不要回答其他内容,请用中文回答";
     
     return prompt;
 }

#endif
/**
 * @brief 构建用于画面分析的Prompt
 * @param user_query [in] 用户的原始文本问题
 * @param isText 是否纯文本问题
 * @return 返回包含了<image>标签的、符合模型输入规范的完整Prompt字符串
 */
std::string build_image_analysis_prompt(bool isText,const std::string& user_query)
 {
    std::string textpreset;

    textpreset = user_query;

    /* 纯文本推理，不添加 <image> 标签 */
    if(isText == true){
        return textpreset;
    }
      
    /* 使用 build_vision_prompt 添加 <image> 标签 */
    return build_vision_prompt(textpreset);
 }

 bool fillRGBToCenter(const cv::Mat& src, const cv::Rect& roi, cv::Mat& dst)
{
    /* 1. 输入参数检查 */
    if (src.empty() || dst.empty() || 
        src.type() != CV_8UC3 || dst.type() != CV_8UC3 || // 修改：检查RGB类型
        !roi.area() || 
        roi.x < 0 || roi.y < 0 || 
        roi.x + roi.width > src.cols || 
        roi.y + roi.height > src.rows) // 修改：对于RGB图像，检查整个高度
    {
        printf("Invalid parameters: src(%dx%d type=%d) dst(%dx%d type=%d), roi(%d,%d,%d,%d)\n",
               src.cols, src.rows, src.type(), dst.cols, dst.rows, dst.type(),
               roi.x, roi.y, roi.width, roi.height);
        return false;
    }

    /* 2. 定义目标尺寸 (直接使用目标图像dst的尺寸) */
    const cv::Size target_size(dst.cols, dst.rows); // 修改：目标尺寸就是dst的尺寸
    const cv::Scalar black(0, 0, 0); // 修改：RGB格式的黑色

    /* 3. 提取ROI区域 */
    cv::Mat roiMat = src(roi).clone(); // 直接克隆，简化处理

    /* 4. 计算缩放比例并缩放ROI */
    // 保持宽高比，将ROI缩放到能放入target_size的最大尺寸
    double scale = std::min(static_cast<double>(target_size.width) / roiMat.cols, 
                            static_cast<double>(target_size.height) / roiMat.rows);
    cv::Size scaled_size(static_cast<int>(std::round(roiMat.cols * scale)), 
                         static_cast<int>(std::round(roiMat.rows * scale)));

    cv::Mat resizedRoi;
    cv::resize(roiMat, resizedRoi, scaled_size, 0, 0, cv::INTER_LINEAR);

    /* 5. 创建目标图像并填充背景色 */
    cv::Mat rgbDst(target_size, CV_8UC3, black); // 修改：直接创建RGB图像

    /* 6. 计算居中放置的位置 */
    int x = (target_size.width - resizedRoi.cols) / 2;
    int y = (target_size.height - resizedRoi.rows) / 2;
    cv::Rect place_rect(x, y, resizedRoi.cols, resizedRoi.rows);

    /* 7. 将缩放后的ROI复制到目标图像的中心 */
    resizedRoi.copyTo(rgbDst(place_rect));

    /* 8. 将结果复制到输出参数dst */
    rgbDst.copyTo(dst);

    return true;
}

bool removeBlackBorderAndConvertToBGR(const cv::Mat& src, cv::Mat& dst, int black_threshold)
{
    /* 1. 输入参数有效性检查 */
    if (src.empty()) {
        printf("Input image is empty!\n");
        return false;
    }
    if (src.channels() != 3) {
        printf("Input image must be 3-channel (RGB/BGR), but got %d channels!\n", src.channels());
        return false;
    }

    /* 2. 先将输入图像统一转换为 BGR 格式（兼容 RGB 输入） */
    cv::Mat src_bgr;
    if (src.type() == CV_8UC3) 
    {
        // 若输入已是 CV_8UC3，直接判断通道顺序（简单兼容：假设非 BGR 则视为 RGB）
        // 注：严格区分需额外判断，此处简化处理，适用于常见场景
        src_bgr = src.clone();
        // 若确定输入是 RGB，可手动转换：cv::cvtColor(src, src_bgr, cv::COLOR_RGB2BGR);
    } else {
        // 其他 3 通道格式（如 CV_16UC3），先转为 CV_8UC3 再转 BGR
        src.convertTo(src_bgr, CV_8UC3);
        cv::cvtColor(src_bgr, src_bgr, cv::COLOR_RGB2BGR); // 假设原格式为 RGB
    }

    /* 3. 定义黑色像素判断函数（BGR 格式，三通道均 <= 阈值视为黑色） */
    auto isBlackPixel = [&](const cv::Vec3b& pixel) {
        return (pixel[0] <= black_threshold) && 
               (pixel[1] <= black_threshold) && 
               (pixel[2] <= black_threshold);
    };

    /* 4. 查找有效区域（非黑边的最小矩形） */
    int min_x = src_bgr.cols - 1;  // 有效区域左边界（初始化为最右）
    int max_x = 0;                 // 有效区域右边界（初始化为最左）
    int min_y = src_bgr.rows - 1;  // 有效区域上边界（初始化为最下）
    int max_y = 0;                 // 有效区域下边界（初始化为最上）

    // 遍历图像所有像素，寻找有效区域边界
    for (int y = 0; y < src_bgr.rows; ++y) {
        for (int x = 0; x < src_bgr.cols; ++x) {
            cv::Vec3b pixel = src_bgr.at<cv::Vec3b>(y, x);
            if (!isBlackPixel(pixel)) { // 找到非黑色像素，更新有效边界
                min_x = std::min(min_x, x);
                max_x = std::max(max_x, x);
                min_y = std::min(min_y, y);
                max_y = std::max(max_y, y);
            }
        }
    }

    /* 5. 检查有效区域是否存在（排除全黑图像） */
    if (min_x > max_x || min_y > max_y) {
        printf("Input image is all black! No valid region to crop.\n");
        return false;
    }

    /* 6. 裁剪有效区域（去除黑边） */
    cv::Rect valid_rect(min_x, min_y, max_x - min_x + 1, max_y - min_y + 1);
    dst = src_bgr(valid_rect).clone();

    /* 7. 验证输出格式（确保是 CV_8UC3 BGR） */
    if (dst.type() != CV_8UC3) {
        printf("Output image type error! Expected CV_8UC3, got %d\n", dst.type());
        return false;
    }

    printf("Black border removed successfully. Valid region: (%d,%d) to (%d,%d), output size: %dx%d\n",
           min_x, min_y, max_x, max_y, dst.cols, dst.rows);
    return true;
}

/*RGA 通用图像处理*/
bool rga_image_transform(void* src_vir, int sw, int sh, int src_format,
                                   void* dst_vir, int dw, int dh, int dst_format,
                                   int cx, int cy, int cw, int ch,
                                   int rotation) 
{
    // 基础合法性校验
    if (!src_vir || !dst_vir || sw <= 0 || sh <= 0 || dw <= 0 || dh <= 0) 
    {
        return false;
    }

    // 默认值：如果 cw 或 ch 为 0，为不裁剪
    if (cw <= 0) cw = sw;
    if (ch <= 0) ch = sh;

    //  硬件对齐修正 (针对 NV12 等 YUV 格式采样要求)
    // 起始坐标强转为偶数 (2对齐)，防止色度平面偏位
    cx &= ~1;
    cy &= ~1;
    // 裁剪宽高强转为偶数，确保 YUV 分量完整
    cw &= ~1;
    ch &= ~1;

    // 边界溢出保护：如果起始点 + 裁剪宽高超过了原图，强制收缩并保持偶数
    if (cx + cw > sw) cw = (sw - cx) & ~1;
    if (cy + ch > sh) ch = (sh - cy) & ~1;

    // 目标步长对齐校验,RGB888 硬件限制
    // 对于 RGB888/BGR888 目标，dw 必须是 4 的倍数，否则 Stride 不对齐会导致硬件报错
    if ((dst_format == RK_FORMAT_RGB_888 || dst_format == RK_FORMAT_BGR_888)) 
    {
        if (dw % 4 != 0) {
            dlog_error("[ERROR] RGA: dw(%d) must be 4-aligned for RGB888!\n", dw);
            return false; 
        }
    }

    // 包装 Buffer
    rga_buffer_t src_buf = wrapbuffer_virtualaddr(src_vir, sw, sh, src_format);
    rga_buffer_t dst_buf = wrapbuffer_virtualaddr(dst_vir, dw, dh, dst_format);

    // 定义区域
    im_rect src_rect = {cx, cy, cw, ch};
    im_rect dst_rect = {0, 0, dw, dh};

    // 处理旋转标志位
    int usage_flags = 0;
    switch (rotation) 
    {
        case 90:  usage_flags = IM_HAL_TRANSFORM_ROT_90;  break;
        case 180: usage_flags = IM_HAL_TRANSFORM_ROT_180; break;
        case 270: usage_flags = IM_HAL_TRANSFORM_ROT_270; break;
        default:  usage_flags = 0; break;
    }

    // 执行硬件变换
    IM_STATUS status = improcess(src_buf, dst_buf, {}, src_rect, dst_rect, {}, usage_flags);

    if (status != IM_STATUS_SUCCESS) 
    {
        dlog_error("[ERROR] RGA improcess fail: %s (src:%dx%d, dst:%dx%d, fmt:0x%x->0x%x)\n", 
                imStrError(status), sw, sh, dw, dh, src_format, dst_format);
        return false;
    }

    return true;
}

/**
 * @brief   : 将 cv::Mat 编码为 JPEG 到 EventTvSdkImage_S
 * @param    {cv::Mat} &mat 输入图像
 * @param    {EventTvSdkImage_S} &stImage 输出图像
 * @param    {int} nQuality JPEG质量(1-100)
 * @param    {bool} bInputRgb 输入图片格式是否RGB
 * @return   {bool} true 成功 false 失败
 */
bool encode_mat_to_tvsdk_image(const cv::Mat &mat, EventTvSdkImage_S &stImage, int nQuality, bool bInputRgb)
{
    if (mat.empty())
    {
        return false;
    }

    cv::Mat encodeMat;
    if (bInputRgb && mat.channels() == 3)
    {
        /* detect模块统一使用 RGB 格式，JPEG编码需要 BGR，仅交换通道指针无像素拷贝 */
        cv::cvtColor(mat, encodeMat, cv::COLOR_RGB2BGR);
    }
    else
    {
        encodeMat = mat;
    }

    std::vector<unsigned char> vecBuf;
    std::vector<int> params;
    params.push_back(cv::IMWRITE_JPEG_QUALITY);
    params.push_back(nQuality);

    if (!cv::imencode(".jpg", encodeMat, vecBuf, params))
    {
        return false;
    }

    stImage.vecJpeg = std::move(vecBuf);
    stImage.nWidth = mat.cols;
    stImage.nHeight = mat.rows;
    return true;
}

bool cropTargetImage(const cv::Mat &sourceImage,
                     const cv::Rect2f &detectRect,
                     const cv::Size &detectCoordinateSize,
                     const cv::Size &targetSize,
                     cv::Mat &targetImage)
{
    targetImage.release();

    if (sourceImage.empty() || sourceImage.dims != 2 ||
        detectCoordinateSize.width <= 0 || detectCoordinateSize.height <= 0 ||
        targetSize.width <= 0 || targetSize.height <= 0 ||
        !std::isfinite(detectRect.x) || !std::isfinite(detectRect.y) ||
        !std::isfinite(detectRect.width) || !std::isfinite(detectRect.height) ||
        detectRect.width <= 0.0f || detectRect.height <= 0.0f)
    {
        return false;
    }

    const double scaleX = static_cast<double>(sourceImage.cols) /
                          static_cast<double>(detectCoordinateSize.width);
    const double scaleY = static_cast<double>(sourceImage.rows) /
                          static_cast<double>(detectCoordinateSize.height);

    const double left = std::max(0.0, static_cast<double>(detectRect.x) * scaleX);
    const double top = std::max(0.0, static_cast<double>(detectRect.y) * scaleY);
    const double right = std::min(
        static_cast<double>(sourceImage.cols),
        (static_cast<double>(detectRect.x) + static_cast<double>(detectRect.width)) * scaleX);
    const double bottom = std::min(
        static_cast<double>(sourceImage.rows),
        (static_cast<double>(detectRect.y) + static_cast<double>(detectRect.height)) * scaleY);

    if (!std::isfinite(left) || !std::isfinite(top) ||
        !std::isfinite(right) || !std::isfinite(bottom) ||
        right <= left || bottom <= top)
    {
        return false;
    }

    const int x1 = std::max(0, std::min(sourceImage.cols,
                                        static_cast<int>(std::floor(left))));
    const int y1 = std::max(0, std::min(sourceImage.rows,
                                        static_cast<int>(std::floor(top))));
    const int x2 = std::max(0, std::min(sourceImage.cols,
                                        static_cast<int>(std::ceil(right))));
    const int y2 = std::max(0, std::min(sourceImage.rows,
                                        static_cast<int>(std::ceil(bottom))));

    if (x2 <= x1 || y2 <= y1)
    {
        return false;
    }

    const cv::Mat croppedImage =
        sourceImage(cv::Rect(x1, y1, x2 - x1, y2 - y1));
    const double resizeScale = std::min(
        static_cast<double>(targetSize.width) / static_cast<double>(croppedImage.cols),
        static_cast<double>(targetSize.height) / static_cast<double>(croppedImage.rows));

    if (!std::isfinite(resizeScale) || resizeScale <= 0.0)
    {
        return false;
    }

    const int scaledWidth = std::max(
        1,
        std::min(targetSize.width,
                 static_cast<int>(std::lround(croppedImage.cols * resizeScale))));
    const int scaledHeight = std::max(
        1,
        std::min(targetSize.height,
                 static_cast<int>(std::lround(croppedImage.rows * resizeScale))));

    cv::Mat resizedImage;
    const int interpolation =
        resizeScale < 1.0 ? cv::INTER_AREA : cv::INTER_LINEAR;
    cv::resize(croppedImage,
               resizedImage,
               cv::Size(scaledWidth, scaledHeight),
               0.0,
               0.0,
               interpolation);

    cv::Mat paddedImage = cv::Mat::zeros(targetSize, sourceImage.type());
    const int offsetX = (targetSize.width - scaledWidth) / 2;
    const int offsetY = (targetSize.height - scaledHeight) / 2;
    resizedImage.copyTo(
        paddedImage(cv::Rect(offsetX, offsetY, scaledWidth, scaledHeight)));
    targetImage = std::move(paddedImage);

    return !targetImage.empty() &&
           targetImage.cols == targetSize.width &&
           targetImage.rows == targetSize.height;
}
