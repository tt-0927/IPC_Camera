/*
 * @FilePath     : PPOCRDetectPostProcess.cpp
 * @Author       : 严泽辉 yanzeh@kfb.cn
 * @Date         : 2024-05-30 16:01:33
 * @LastEditors  : 严泽辉 yanzeh@kfb.cn
 * @LastEditTime : 2024-09-26 20:07:54
 * @Description  :
 */

#include "PPOCRDetectPostProcess.hpp"

#include <set>

bool PostProcess_NS::cPPOCRDetectPostProcess::postPolyProcess(
    std::vector<float *> vInput,
    int nOutuutWidth,
    int nOutputHeight,
    float fMaskThreshold,
    float fBoxThreshold,
    const float &fUnclipRatio,
    bool bPolyType,
    std::vector<Inference_NS::PointData_S> &vPointDatas)
{
    if (vInput.size() == 0 ||
        nOutuutWidth <= 0 ||
        nOutputHeight <= 0)
    {
        printf("传入参数为空\n");
        return false;
    }

    vPointDatas.clear();

    int nOutSize = nOutuutWidth * nOutputHeight;

    /* prepare bitmap */ 
    std::vector<float> vPred(nOutSize, 0.0);
    std::vector<unsigned char> vCbuf(nOutSize, ' ');

    for (int i = 0; i < nOutSize; i++)
    {
        vPred[i] = float(vInput[0][i]);
        vCbuf[i] = (unsigned char)((vInput[0][i]) * 255);
    }
    cv::Mat aCbufMap(nOutputHeight, nOutuutWidth, CV_8UC1, (unsigned char *)vCbuf.data());
    cv::Mat aPredMap(nOutputHeight, nOutuutWidth, CV_32F, (float *)vPred.data());

    float fThreshold = fMaskThreshold * 255;
    float maxvalue = 255;
    cv::Mat aBitMap;
    cv::threshold(aCbufMap, aBitMap, fThreshold, maxvalue, cv::THRESH_BINARY);
    /* 膨胀操作 */
    if (m_bUseDilation)
    {
        cv::Mat aDilaEle = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(2, 2));
        cv::dilate(aBitMap, aBitMap, aDilaEle);
    }

    /* find polygon Contours */
    const int nMinSize = 3;
    const int NmaxCandidates = 1000;
    std::vector<std::vector<cv::Point>> vContours;
    std::vector<cv::Vec4i> vHierarchy;

    cv::findContours(aBitMap, vContours, vHierarchy, cv::RETR_LIST, cv::CHAIN_APPROX_SIMPLE);

    int nNumContours = vContours.size() >= NmaxCandidates ? NmaxCandidates : vContours.size();

    std::vector<std::vector<std::vector<int>>> vvBoxes;
    std::vector<float> vScores;

    for (int i = 0; i < nNumContours; i++)
    {
        if(bPolyType)
        {
            float fScore;
            float fEpsilon = 0.002 * cv::arcLength(vContours[i], true);
            std::vector<cv::Point> vPoints;
            cv::approxPolyDP(vContours[i], vPoints, fEpsilon, true);
            if (vPoints.size() < 4)
            {
                continue;
            }

            fScore = polygonScoreAcc(vPoints, aPredMap);
            if (fScore < fBoxThreshold)
            {
                continue;
            }

            std::vector<std::vector<float>> vBoxForUnclip;
            for (int _k = 0; _k < vPoints.size(); _k++)
            {
                std::vector<float> _box;
                _box.push_back(vPoints[_k].x);
                _box.push_back(vPoints[_k].y);
                vBoxForUnclip.push_back(_box);
            }
            /* start for unclip */
            cv::RotatedRect clipbox = unClip(vBoxForUnclip, fUnclipRatio);
            if (clipbox.size.height < 1.001 && clipbox.size.width < 1.001)
            {
                continue;
            }
            /* end for unclip */
            cv::Point2f vertex[4];
            clipbox.points(vertex);

            float fSsid;
            auto cliparray = getMiniBoxes(clipbox, fSsid);
            if (fSsid < nMinSize + 2)
            {
                continue;
            }

            std::vector<std::vector<int>> intcliparray;
            for (int nNumPt = 0; nNumPt < 4; nNumPt++)
            {
                std::vector<int> a{int(clamp(vertex[nNumPt].x, 0, float(nOutuutWidth))), int(clamp(vertex[nNumPt].y, 0, float(nOutputHeight)))};
                intcliparray.push_back(a);
            }
            vvBoxes.push_back(intcliparray);
            vScores.push_back(fScore);
        }
        else
        {
            float fScore;
            if (vContours[i].size() <= 2)
            {
                continue;
            }

            float fSsid;
            cv::RotatedRect box = cv::minAreaRect(vContours[i]);
            auto array = getMiniBoxes(box, fSsid);
            auto vBoxForUnclip = array;
            /* end get_mini_box */

            if (fSsid < nMinSize)
            {
                continue;
            }

            cv::Point2f vertex[4];
            box.points(vertex);

            fScore = boxScoreFast(array, aPredMap);
            if (fScore < fBoxThreshold)
            {
                continue;
            }

            /* start for unclip */ 
            cv::RotatedRect points = unClip(vBoxForUnclip, fUnclipRatio);
            if (points.size.height < 1.001 && points.size.width < 1.001)
            {
                continue;
            }
            /* end for unclip */

            points.points(vertex);
            cv::RotatedRect clipbox = points;
            auto cliparray = getMiniBoxes(clipbox, fSsid);

            if (fSsid < nMinSize + 2)
            {
                continue;
            }

            std::vector<std::vector<int>> intcliparray;

            for (int nNumPt = 0; nNumPt < 4; nNumPt++)
            {
                std::vector<int> a{
                    int(clamp(cliparray[nNumPt][0], 0, float(nOutuutWidth))),
                    int(clamp(cliparray[nNumPt][1], 0, float(nOutputHeight)))};
                intcliparray.push_back(a);
            }
            vvBoxes.push_back(intcliparray);
            vScores.push_back(fScore);
        }
    }

    /* box valid detect target */
    for (int i = 0; i < vvBoxes.size(); i++)
    {
        vvBoxes[i] = orderPointsClockwise(vvBoxes[i]);
        for (int m = 0; m < vvBoxes[0].size(); m++)
        {
            vvBoxes[i][m][0] = clamp(vvBoxes[i][m][0], 0, nOutuutWidth - 1);
            vvBoxes[i][m][1] = clamp(vvBoxes[i][m][1], 0, nOutputHeight - 1);
        }

        int nRectWidth, nRectHeight;
        nRectWidth = int(sqrt(pow(vvBoxes[i][0][0] - vvBoxes[i][1][0], 2) +
                              pow(vvBoxes[i][0][1] - vvBoxes[i][1][1], 2)));
        nRectHeight = int(sqrt(pow(vvBoxes[i][0][0] - vvBoxes[i][3][0], 2) +
                               pow(vvBoxes[i][0][1] - vvBoxes[i][3][1], 2)));

        if (nRectWidth <= 4 || nRectHeight <= 4)
        {
            continue;
        }

        float fX1 = vvBoxes[i][0][0];
        float fY1 = vvBoxes[i][0][1];
        float fX2 = vvBoxes[i][2][0];
        float fY2 = vvBoxes[i][2][1];
        
        /* 存储结果 */
        Inference_NS::PointData_S stPointData;
        stPointData.stBoxs.nX1 =  fX1;
        stPointData.stBoxs.nY1 =  fY1;
        stPointData.stBoxs.nX2 =  fX2;
        stPointData.stBoxs.nY2 =  fY2;
        stPointData.fConfidence =  vScores[i];
        stPointData.nLabel = 0;

        for (int nP = 0; nP < vvBoxes[i].size(); nP++)
        {
            Inference_NS::Point_S stPoint;
            stPoint.nX = vvBoxes[i][nP][0];
            stPoint.nY = vvBoxes[i][nP][1];
            stPointData.vPoints.push_back(stPoint);
        }
        vPointDatas.push_back(stPointData);
    }
    
    return 0;
}

const bool xsortFp32(std::vector<float> a, std::vector<float> b) 
{
    if (a[0] != b[0]) return a[0] < b[0];
    return false;
}

const bool xsortInt(std::vector<int> a, std::vector<int> b) 
{
    if (a[0] != b[0]) return a[0] < b[0];
    return false;
}

const std::vector<std::vector<float>> mat2Vector(cv::Mat mat) 
{
    std::vector<std::vector<float>> img_vec;
    std::vector<float> tmp;

    for (int i = 0; i < mat.rows; ++i) {
        tmp.clear();
        for (int j = 0; j < mat.cols; ++j) {
            tmp.push_back(mat.at<float>(i, j));
        }
        img_vec.push_back(tmp);
    }
    return img_vec;
}

std::vector<std::vector<int>> PostProcess_NS::cPPOCRDetectPostProcess::orderPointsClockwise(std::vector<std::vector<int>> vvPts)
{
    std::vector<std::vector<int>> box = vvPts;
    std::sort(box.begin(), box.end(), xsortInt);

    std::vector<std::vector<int>> leftmost = {box[0], box[1]};
    std::vector<std::vector<int>> rightmost = {box[2], box[3]};

    if (leftmost[0][1] > leftmost[1][1])
        std::swap(leftmost[0], leftmost[1]);

    if (rightmost[0][1] > rightmost[1][1])
        std::swap(rightmost[0], rightmost[1]);

    std::vector<std::vector<int>> rect = {leftmost[0], rightmost[0], rightmost[1], leftmost[1]};
    return rect;
}

const void getContourArea(const std::vector<std::vector<float>> &box, float unclip_ratio, float &distance) 
{
    int pts_num = box.size();
    float area = 0.0f;
    float dist = 0.0f;
    for (int i = 0; i < pts_num; i++) {
        area += box[i][0] * box[(i + 1) % pts_num][1] - box[i][1] * box[(i + 1) % pts_num][0];
        dist += sqrtf((box[i][0] - box[(i + 1) % pts_num][0]) * (box[i][0] - box[(i + 1) % pts_num][0]) +
                    (box[i][1] - box[(i + 1) % pts_num][1]) * (box[i][1] - box[(i + 1) % pts_num][1]));
    }
    area = fabs(float(area / 2.0));

    distance = area * unclip_ratio / dist;
}

float PostProcess_NS::cPPOCRDetectPostProcess::boxScoreFast(std::vector<std::vector<float>> vBoxArray, cv::Mat aPred)
{
    auto array = vBoxArray;
    int width = aPred.cols;
    int height = aPred.rows;

    float box_x[4] = {array[0][0], array[1][0], array[2][0], array[3][0]};
    float box_y[4] = {array[0][1], array[1][1], array[2][1], array[3][1]};

    int xmin = clamp(int(std::floor(*(std::min_element(box_x, box_x + 4)))), 0, width - 1);
    int xmax = clamp(int(std::ceil(*(std::max_element(box_x, box_x + 4)))), 0, width - 1);
    int ymin = clamp(int(std::floor(*(std::min_element(box_y, box_y + 4)))), 0, height - 1);
    int ymax = clamp(int(std::ceil(*(std::max_element(box_y, box_y + 4)))), 0, height - 1);

    cv::Mat mask;
    mask = cv::Mat::zeros(ymax - ymin + 1, xmax - xmin + 1, CV_8UC1);

    cv::Point root_point[4];
    root_point[0] = cv::Point(int(array[0][0]) - xmin, int(array[0][1]) - ymin);
    root_point[1] = cv::Point(int(array[1][0]) - xmin, int(array[1][1]) - ymin);
    root_point[2] = cv::Point(int(array[2][0]) - xmin, int(array[2][1]) - ymin);
    root_point[3] = cv::Point(int(array[3][0]) - xmin, int(array[3][1]) - ymin);
    const cv::Point *ppt[1] = {root_point};
    int npt[] = {4};
    cv::fillPoly(mask, ppt, npt, 1, cv::Scalar(1));

    cv::Mat croppedImg;
    aPred(cv::Rect(xmin, ymin, xmax - xmin + 1, ymax - ymin + 1)).copyTo(croppedImg);

    float score = cv::mean(croppedImg, mask)[0];
    return score;
}

std::vector<std::vector<float>> PostProcess_NS::cPPOCRDetectPostProcess::getMiniBoxes(cv::RotatedRect aBox, float &fSsid)
{
    fSsid = std::max(aBox.size.width, aBox.size.height);

    cv::Mat points;
    cv::boxPoints(aBox, points);
    auto array = mat2Vector(points);
    std::sort(array.begin(), array.end(), xsortFp32);
    std::vector<float> idx1 = array[0], idx2 = array[1], idx3 = array[2], idx4 = array[3];
    if (array[3][1] <= array[2][1])
    {
        idx2 = array[3];
        idx3 = array[2];
    }
    else
    {
        idx2 = array[2];
        idx3 = array[3];
    }
    if (array[1][1] <= array[0][1])
    {
        idx1 = array[1];
        idx4 = array[0];
    }
    else
    {
        idx1 = array[0];
        idx4 = array[1];
    }

    array[0] = idx1;
    array[1] = idx2;
    array[2] = idx3;
    array[3] = idx4;

    return array;
}

cv::RotatedRect PostProcess_NS::cPPOCRDetectPostProcess::unClip(std::vector<std::vector<float>> &vvBox, const float &fUnclipRatio)
{
    float fDistance = 1.0;
    getContourArea(vvBox, fUnclipRatio, fDistance);

    ClipperLib::ClipperOffset offset;
    ClipperLib::Path p;
    int nPtsNum = vvBox.size();
    for (int i = 0; i < nPtsNum; i++)
    {
        p << ClipperLib::IntPoint(int(vvBox[i][0]), int(vvBox[i][1]));
    }
    offset.AddPath(p, ClipperLib::jtRound, ClipperLib::etClosedPolygon);

    ClipperLib::Paths soln;
    offset.Execute(soln, fDistance);
    std::vector<cv::Point2f> points;

    for (int j = 0; j < soln.size(); j++)
    {
        for (int i = 0; i < soln[soln.size() - 1].size(); i++)
        {
            points.emplace_back(soln[j][i].X, soln[j][i].Y);
        }
    }
    cv::RotatedRect aRes;
    if (points.size() <= 0)
    {
        aRes = cv::RotatedRect(cv::Point2f(0, 0), cv::Size2f(1, 1), 0);
    }
    else
    {
        aRes = cv::minAreaRect(points);
    }
    return aRes;
}

float PostProcess_NS::cPPOCRDetectPostProcess::polygonScoreAcc(std::vector<cv::Point> vContour, cv::Mat aPred)
{
    int nWidth = aPred.cols;
    int nHeight = aPred.rows;
    std::vector<float> vBoxX;
    std::vector<float> vBoxY;
    for (int i = 0; i < vContour.size(); i++)
    {
        vBoxX.push_back(vContour[i].x);
        vBoxY.push_back(vContour[i].y);
    }

    int nXMin = clamp(int(std::floor(*(std::min_element(vBoxX.begin(), vBoxX.end())))), 0, nWidth - 1);
    int nXMax = clamp(int(std::ceil(*(std::max_element(vBoxX.begin(), vBoxX.end())))), 0, nWidth - 1);
    int nYMin = clamp(int(std::floor(*(std::min_element(vBoxY.begin(), vBoxY.end())))), 0, nHeight - 1);
    int nYMax = clamp(int(std::ceil(*(std::max_element(vBoxY.begin(), vBoxY.end())))), 0, nHeight - 1);

    cv::Mat aMask;
    aMask = cv::Mat::zeros(nYMax - nYMin + 1, nXMax - nXMin + 1, CV_8UC1);

    cv::Point *pRookPoint = new cv::Point[vContour.size()];

    for (int i = 0; i < vContour.size(); i++)
    {
        pRookPoint[i] = cv::Point(int(vBoxX[i]) - nXMin, int(vBoxY[i]) - nYMin);
    }
    const cv::Point *pPt[1] = {pRookPoint};
    int nPt[] = {int(vContour.size())};

    cv::fillPoly(aMask, pPt, nPt, 1, cv::Scalar(1));

    cv::Mat aCroppedImg;
    aPred(cv::Rect(nXMin, nYMin, nXMax - nXMin + 1, nYMax - nYMin + 1)).copyTo(aCroppedImg);
    float fScore = cv::mean(aCroppedImg, aMask)[0];

    delete[] pRookPoint;
    return fScore;
}