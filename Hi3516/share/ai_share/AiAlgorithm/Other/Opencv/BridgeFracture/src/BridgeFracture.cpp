#include "BridgeFracture.h"

#include <algorithm>
#include <chrono>
#include <cmath>
using namespace std;
using namespace BRIDGEFRACTURE_NS;

/* 构造函数 -- 初始化变量 */
CBridgeFracture::CBridgeFracture()
{
}

CBridgeFracture::CBridgeFracture(int nDistanceThreshold, int nAngleThreshold, int nBrokenBridgeNumThreshold)
{
    m_nDistanceThreshold        = nDistanceThreshold;
    m_nAngleThreshold           = nAngleThreshold;
    m_nBrokenBridgeNumThreshold = nBrokenBridgeNumThreshold;
}

/* 销毁创建的模型 */
CBridgeFracture::~CBridgeFracture()
{
}

/* 设置 标准线 的坐标 */
bool CBridgeFracture::setData(std::vector<int> vnInputLinePoints)
{
    if (vnInputLinePoints.size() != 4)
    {
        return false;
    }

    std::map<int, int> mPoints;
    for (int y = std::min(vnInputLinePoints[1], vnInputLinePoints[3]); y <= std::max(vnInputLinePoints[1], vnInputLinePoints[3]); y++)
    {
        mPoints[y] = 0;
    }
    m_vLinesCounts.push_back(mPoints);
    m_vLinePoints.push_back(vnInputLinePoints);
    return true;
}

/* 设置 标准线 的坐标 */
bool CBridgeFracture::setData(std::vector<std::vector<int>> vvnInputLinePoints)
{

    clearData();

    for (auto item : vvnInputLinePoints)
    {
        if (item.size() != 4)
        {
            continue;
        }

        std::map<int, int> mPoints;
        for (int y = std::min(item[1], item[3]); y <= std::max(item[1], item[3]); y++)
        {
            mPoints[y] = 0;
        }
        printf("设置的基准线： %d %d %d %d\n", item[0], item[1], item[2], item[3]);
        m_vLinesCounts.push_back(mPoints);
        m_vLinePoints.push_back(item);
    }

    return true;
}

/* 清除基准线 */
void BRIDGEFRACTURE_NS::CBridgeFracture::clearData()
{
    m_vLinesCounts.clear();
    m_vLinePoints.clear();
}

/* 判断是否平行 */
bool Px(cv::Vec4i vPoint1, std::vector<int> vPoint2)
{
    /* 计算直线1的斜率 */
    float fSlope1;
    if (vPoint1[2] - vPoint1[0] != 0)
    {
        fSlope1 = static_cast<float>(vPoint1[3] - vPoint1[1]) / (vPoint1[2] - vPoint1[0]);
    }
    else
    {
        fSlope1 = INFINITY;
    }

    /* 计算直线2的斜率 */
    float fSlope2;
    if (vPoint2[2] - vPoint2[0] != 0)
    {

        fSlope2 = static_cast<float>(vPoint2[3] - vPoint2[1]) / (vPoint2[2] - vPoint2[0]);
    }
    else
    {
        fSlope2 = INFINITY;
    }
    // 判断直线是否平行
    if (fSlope1 == fSlope2)
    {
        return true;
    }
    else
    {
        return false;
    }
}

/* 求两条直线的距离 */
float JL(std::vector<int> point, cv::Vec4i p)
{
    float A = point[3] - point[1];
    float B = point[0] - point[2];
    float C = point[2] * point[1] - point[0] * point[3];

    float x1        = p[0];
    float y1        = p[1];
    float distance1 = std::abs(A * x1 + B * y1 + C) * 1.0 / std::sqrt(A * A + B * B);
    float x2        = p[2];
    float y2        = p[3];
    float distance2 = std::abs(A * x2 + B * y2 + C) * 1.0 / std::sqrt(A * A + B * B);
    return std::min(distance1, distance2);
    // return distance1;
}

/* 求两条直线的夹角 */
double JD(cv::Vec4i point1, std::vector<int> point2)
{
    double slope1 = (point1[2] - point1[0] != 0) ? (point1[3] - point1[1]) * 1.0 / (point1[2] - point1[0]) : std::numeric_limits<double>::infinity();
    double slope2 = (point2[2] - point2[0] != 0) ? (point2[3] - point2[1]) * 1.0 / (point2[2] - point2[0]) : std::numeric_limits<double>::infinity();

    if (slope1 == std::numeric_limits<double>::infinity() && slope2 == std::numeric_limits<double>::infinity())
    {
        return 0;
    }
    if (slope1 == 0 || slope2 == 0)
    {
        return 90;
    }
    if (slope1 == std::numeric_limits<double>::infinity() || slope2 == std::numeric_limits<double>::infinity())
    {
        return 0;
    }
    if (slope1 * slope2 == -1)
    {
        return 90;
    }

    double angle = std::atan(std::abs((slope2 - slope1) * 1.0 / (1 + slope1 * slope2))) * 180.0 / M_PI;
    return angle;
}

/* 给x求y */
int getYFromX(std::vector<int> point, int px)
{
    int x1 = point[0], y1 = point[1], x2 = point[2], y2 = point[3];
    printf("===%d %d %d %d\n", x1, y1, x2, y2);
    /* 斜率不存在 */
    if (x2 - x1 == 0)
    {
        int py = (px > 1.0 * (x1 + x2) / 2) ? std::max(y1, y2) : std::min(y1, y2);
        return py;
    }
    else
    {
        int py = y1 + (px - x1) * 1.0 * (y2 - y1) / (x2 - x1);
        return py;
    }
}

/* 给y求x */
int getXFromY(std::vector<int> point, int y)
{
    int x1 = point[0], y1 = point[1], x2 = point[2], y2 = point[3];
    /* 斜率不存在 */
    if (x2 - x1 == 0)
    {
        return x1;
    }
    /* 点在水平线上 */
    if (y1 == y2)
    {
        return 1.0 * (x1 + x2) / 2;
    }
    /* y等于第一个点的y坐标 */
    if (y == y1)
    {
        return x1;
    }
    /* y等于第二个点的y坐标 */
    if (y == y2)
    {
        return x2;
    }
    double x = x1 + ((y - y1) * 1.0 / (y2 - y1)) * (x2 - x1);
    return round(x);
}

int getXFromY(cv::Vec4i point, int y)
{
    int x1 = point[0], y1 = point[1], x2 = point[2], y2 = point[3];
    /* 斜率不存在 */
    if (x2 - x1 == 0)
    {
        return x1;
    }
    /* 点在水平线上 */
    if (y1 == y2)
    {
        return 1.0 * (x1 + x2) / 2;
    }
    /* y等于第一个点的y坐标 */
    if (y == y1)
    {
        return x1;
    }
    /* y等于第二个点的y坐标 */
    if (y == y2)
    {
        return x2;
    }
    double x = x1 + ((y - y1) * 1.0 / (y2 - y1)) * (x2 - x1);
    return round(x);
}

/* 求x在直线的映射点 */
void getX(std::vector<int> Npoint1, cv::Vec4i point2, int& minX, int& maxX)
{
    /* 直线1两点坐标 */
    int x1  = Npoint1[0];
    int y1  = Npoint1[1];
    int x2  = Npoint1[2];
    int y2  = Npoint1[3];
    /* 要映射的点的坐标 */
    int px0 = point2[0];
    int py0 = point2[1];
    int px1 = point2[2];
    int py1 = point2[3];

    /* 计算直线1的向量 */
    std::vector<double> v  = { x2 - x1, y2 - y1 };
    /* 计算点到直线起点的向量 */
    std::vector<double> u0 = { px0 - x1, py0 - y1 };
    std::vector<double> u1 = { px1 - x1, py1 - y1 };

    /* 计算投影的比例 */
    double              projection_ratio0 = (u0[0] * v[0] + u0[1] * v[1]) * 1.0 / (v[0] * v[0] + v[1] * v[1]);
    double              projection_ratio1 = (u1[0] * v[0] + u1[1] * v[1]) * 1.0 / (v[0] * v[0] + v[1] * v[1]);
    /* 计算投影点的坐标 */
    std::vector<double> projected_point0  = { x1, y1 };
    std::vector<double> projected_point1  = { x1, y1 };

    projected_point0[0] += projection_ratio0 * v[0];
    projected_point0[1] += projection_ratio0 * v[1];

    projected_point1[0] += projection_ratio1 * v[0];
    projected_point1[1] += projection_ratio1 * v[1];

    minX = std::min(round(projected_point0[0]), round(projected_point1[0]));
    maxX = std::max(round(projected_point0[0]), round(projected_point1[0]));
}

/* 求y在直线的映射点 */
void getY(std::vector<int> Npoint1, cv::Vec4i point2, int& minY, int& maxY)
{
    /* 直线1两点坐标 */
    int x1  = Npoint1[0];
    int y1  = Npoint1[1];
    int x2  = Npoint1[2];
    int y2  = Npoint1[3];
    /* 要映射的点的坐标 */
    int px0 = point2[0];
    int py0 = point2[1];
    int px1 = point2[2];
    int py1 = point2[3];

    /* 计算直线1的向量 */
    std::vector<double> v  = { x2 - x1, y2 - y1 };
    /* 计算点到直线起点的向量 */
    std::vector<double> u0 = { px0 - x1, py0 - y1 };
    std::vector<double> u1 = { px1 - x1, py1 - y1 };

    /* 计算投影的比例 */
    double              projection_ratio0 = (u0[0] * v[0] + u0[1] * v[1]) * 1.0 / (v[0] * v[0] + v[1] * v[1]);
    double              projection_ratio1 = (u1[0] * v[0] + u1[1] * v[1]) * 1.0 / (v[0] * v[0] + v[1] * v[1]);
    /* 计算投影点的坐标 */
    std::vector<double> projected_point0  = { x1, y1 };
    std::vector<double> projected_point1  = { x1, y1 };

    projected_point0[0] += projection_ratio0 * v[0];
    projected_point0[1] += projection_ratio0 * v[1];

    projected_point1[0] += projection_ratio1 * v[0];
    projected_point1[1] += projection_ratio1 * v[1];

    minY = std::min(round(projected_point0[1]), round(projected_point1[1]));
    maxY = std::max(round(projected_point0[1]), round(projected_point1[1]));
}

/* 筛选出跟 基准线同一个方向且在附近的 线 */
void CBridgeFracture::filterLines(cv::Mat aHfLines, std::vector<int> vLinePoints, std::map<int, int>& mLinesCount, cv::Mat& aFrame)
{
    std::map<int, int> mOnePoints;
    for (int y = std::min(vLinePoints[1], vLinePoints[3]); y <= std::max(vLinePoints[1], vLinePoints[3]); y++)
    {
        mOnePoints[y] = 0;
    }
    /* 遍历识别的直线，筛选出符合条件的直线 */
    for (int nHf = 0; nHf < aHfLines.rows; nHf++)
    {
        cv::Vec4i vLine = aHfLines.at<cv::Vec4i>(nHf);
        // cv::line(aFrame, cv::Point(vLine[0], vLine[1]), cv::Point(vLine[2], vLine[3]), cv::Scalar(0, 255, 0), 2);

        /* 计算直线距离， 筛选出距离都符合阈值的直线 */
        float fDistance = JL(vLinePoints, vLine);
        if (fDistance > m_nDistanceThreshold)
        {
            continue;
        }
        /* 计算平行和角度 */
        float fAngle = JD(vLine, vLinePoints);
        bool  bPx    = Px(vLine, vLinePoints);

        /* 筛选出平行和角度符合阈值的直线 */
        if (bPx || fAngle < m_nAngleThreshold)
        {
            /* 绘制直线，只绘制标准线内的红线 */
            int x0, y0, x1, y1;
            if (vLine[1] > vLine[3])
            {
                x0 = vLine[2];
                y0 = vLine[3];
                x1 = vLine[0];
                y1 = vLine[1];
            }
            else
            {
                x0 = vLine[0];
                y0 = vLine[1];
                x1 = vLine[2];
                y1 = vLine[3];
            }
            int miny = std::min(vLinePoints[1], vLinePoints[3]);
            int maxy = std::max(vLinePoints[1], vLinePoints[3]);

            if (y1 < miny || y0 > maxy)
            {
                continue;
            }
            if (y0 < miny)
            {
                y0 = miny;
                x0 = getXFromY(vLine, y0);
            }
            if (y1 > maxy)
            {
                y1 = maxy;
                x1 = getXFromY(vLine, y1);
            }
            // std::cout<<"3333333333333333333333333333333"<<std::endl;
            // cv::line(aFrame, cv::Point(x0, y0), cv::Point(x1, y1), cv::Scalar(255, 255, 0), 2);
            m_vBridgeLines.push_back(x0);
            m_vBridgeLines.push_back(y0);
            m_vBridgeLines.push_back(x1);
            m_vBridgeLines.push_back(y1);

            /* 识别的线段在 标准线 的x坐标投影范围 */
            int minY;
            int maxY;
            getY(vLinePoints, vLine, minY, maxY);
            for (auto it = mOnePoints.begin(); it != mOnePoints.end(); ++it)
            {
                if (it->first <= maxY && it->first >= minY)
                {
                    mOnePoints[it->first] += 1;
                }
            }
        }
    }
    /* 判断直线是否空缺 */
    for (auto it = mOnePoints.begin(); it != mOnePoints.end(); it++)
    {

        auto keyItem = mLinesCount.find(it->first);
        if (keyItem == mLinesCount.end())
        {
            printf("mLinesCount not find %d\n", it->first);
            continue;
        }

        keyItem = mOnePoints.find(it->first);
        if (keyItem == mOnePoints.end())
        {
            printf("mOnePoints not find %d\n", it->first);
            continue;
        }

        if (mOnePoints[it->first] > 0)
        {
            if (mLinesCount[it->first] < 0)
            {
                mLinesCount[it->first] = 0;
                /*mLinesCount[it->first] -=1;
                if (mLinesCount[it->first] < 0-m_nBrokenBridgeNumThreshold)
                {
                    mLinesCount[it->first] = 0-m_nBrokenBridgeNumThreshold;
                }*/
            }

            else
            {
                mLinesCount[it->first] += 1;
                if (mLinesCount[it->first] > m_nBrokenBridgeNumThreshold)
                {
                    mLinesCount[it->first] = m_nBrokenBridgeNumThreshold;
                }
            }
        }
        else
        {
            mLinesCount[it->first] -= 1;
        }
    }
}

cv::Mat houghLines(cv::Mat image, double threshold, double minLineLength, double maxLineGap)
{
    cv::cvtColor(image, image, cv::COLOR_RGB2BGR);
    cv::Mat gray;
    cv::cvtColor(image, gray, cv::COLOR_BGR2GRAY);
    cv::GaussianBlur(gray, gray, cv::Size(3, 3), 0);

    cv::Mat sobel_x;
    cv::Sobel(gray, sobel_x, CV_64F, 1, 0, 3);
    cv::convertScaleAbs(sobel_x, gray);

    cv::Mat edges;
    cv::Canny(gray, edges, 100, 200, 3);
    // cv::imwrite("mask.jpg", edges);
    cv::Mat lines;
    cv::HoughLinesP(edges, lines, 1, 1.0 * CV_PI / 180, threshold, minLineLength, maxLineGap);

    return lines;
}

bool CBridgeFracture::detectBridgeFracture(cv::Mat& aFrame)
{
    /* 获取图像的宽度和高度 */
    int nWidth  = aFrame.cols;
    int nHeight = aFrame.rows;
    for (auto item : m_vLinePoints)
    {
        if (item.size() != 4)
        {
            return false;
        }
        if (nWidth < std::min(item.at(0), item.at(2)) || nWidth < std::max(item.at(0), item.at(2)) || std::min(item.at(0), item.at(2)) < 0 || std::max(item.at(0), item.at(2)) < 0)
        {
            return false;
        }
        if (nHeight < std::min(item.at(1), item.at(3)) || nHeight < std::max(item.at(1), item.at(3)) || std::min(item.at(1), item.at(3)) < 0 || std::max(item.at(1), item.at(3)) < 0)
        {
            return false;
        }
    }

    // printf("m_nBrokenBridgeNumThreshold=%d\n", m_nBrokenBridgeNumThreshold);

    /* 清空上一帧的数据 */
    m_nBridgeFractureNum = 0;
    m_vBridgeFractureAreas.clear();
    m_vBridgeLines.clear();
    /* 霍夫直线算法 */
    cv::Mat aHfLines = houghLines(aFrame, m_dThreshold, m_dMinLineLength, m_dMaxLineGap);
    for (int nLineIndex = 0; nLineIndex < m_vLinePoints.size(); nLineIndex++)
    {

        filterLines(aHfLines, m_vLinePoints[nLineIndex], m_vLinesCounts[nLineIndex], aFrame);
        /* 判断是否有断裂 */
        std::vector<int> DResult;
        for (auto it = m_vLinesCounts[nLineIndex].begin(); it != m_vLinesCounts[nLineIndex].end(); it++)
        {
            if (m_vLinesCounts[nLineIndex][it->first] < 0 - m_nBrokenBridgeNumThreshold)
            {
                DResult.push_back(it->first);
            }
            else
            {
                /* 如果 标准线 连续5个点以上，都没上则判断有事故 */
                if (DResult.size() != 0 && DResult.size() > 5)
                {
                    std::vector<int> oneArea;
                    int              xx0 = getXFromY(m_vLinePoints[nLineIndex], DResult[0]);
                    int              xx1 = getXFromY(m_vLinePoints[nLineIndex], DResult.back());
                    /* 如何xx0和xx1相等，则xx0和xx1 相差1，保证可以绘制出矩形 */
                    if (xx0 == xx1)
                    {
                        if ((xx1 - 1) < 0)
                        {
                            xx1 += 1;
                        }
                        else
                        {
                            xx0 -= 1;
                        }
                    }
                    /* 将识别到的矩形框插入容器 */
                    oneArea.insert(oneArea.end(), { xx0, DResult[0], xx1, DResult.back() });
                    m_vBridgeFractureAreas.push_back(oneArea);
                    m_nBridgeFractureNum += 1;
                    DResult.clear();
                }
            }
        }
    }

    /* 用于测试的代码 */
    /*cv::line(aFrame, cv::Point(m_vLinePoints[0][0], m_vLinePoints[0][1]), cv::Point(m_vLinePoints[0][2], m_vLinePoints[0][3]), cv::Scalar(0, 0, 255), 2);
    int fontFace = cv::FONT_HERSHEY_SIMPLEX;
    double fontScale = 2.0;
    int thickness = 2;
    int baseline = 0;
    std::string text = std::to_string(m_nBridgeFractureNum);
    cv::Size textSize = cv::getTextSize(text, fontFace, fontScale, thickness, &baseline);
    cv::Point textOrg((aFrame.cols - textSize.width) / 2, (aFrame.rows + textSize.height) / 2);
    cv::putText(aFrame, text, textOrg, fontFace, fontScale, cv::Scalar(0, 0, 0), thickness);
    for(auto box : m_vBridgeFractureAreas)
    {
            cv::rectangle(aFrame, cv::Point(box[0],box[1]), cv::Point(box[2],box[3]), cv::Scalar(0, 255, 0), 2);
    }*/
    return true;
}
