/*
 * @FilePath     : ImageProcessor.cpp
 * @Author       : yanzeh yanzeh@kfb.cn
 * @Date         : 2024-01-12 10:16:42
 * @LastEditors  : 严泽辉 yanzeh@kfb.cn
 * @LastEditTime : 2024-03-14 11:34:03
 * @Description  : 图片处理器
 */
#include "ImageProcessor.hpp"

#include <fstream>

#include "cvxFont.h"
#include "dlog.h"

/* 在cv::Mat的基础上划线 */
BlError_E CImageProcessor::drawLines(cv::Mat& editImage, std::list<LineInfo_S> listLineInfo)
{
    if (editImage.empty())
    {
        dlog(LOG_ERROR, "传入的参数异常");
        return ERR_PARAM;
    }

    for (auto item : listLineInfo)
    {
        /* 定义线的起点和终点 */
        cv::Point startPoint(item.nX1, item.nY1);
        cv::Point endPoint(item.nX2, item.nY2);

        /* 定义线的颜色 */
        cv::Scalar lineColor(item.nR, item.nG, item.nB, item.nA);

        /* 使用cv::line函数在图像上绘制线段 */
        cv::line(editImage, startPoint, endPoint, lineColor, item.nThickness);
    }

    return OK;
}

/* 写字 */
BlError_E CImageProcessor::drawLabel(cv::Mat& editImage, std::list<LabelInfo_S> listLabelInfo)
{
    if (editImage.empty())
    {
        dlog(LOG_ERROR, "传入的参数异常");
        return ERR_PARAM;
    }

    /* 画字 */
    for (auto item : listLabelInfo)
    {
        cv::putText(editImage,
                    item.strLabel,
                    cv::Point(item.nX, item.nY),
                    item.nFontFace,
                    item.dFontScale,
                    cv::Scalar(item.nLabelR, item.nLabelG, item.nLabelB, item.nLabelA),
                    item.nThickness);
    }

    return OK;
}

/* 写字 */
BlError_E CImageProcessor::drawLabel(
    cv::Mat&               editImage,
    std::list<LabelInfo_S> listLabelInfo,
    std::string            strFontPath)
{
    if (editImage.empty())
    {
        dlog(LOG_ERROR, "传入的参数异常");
        return ERR_PARAM;
    }

    std::ifstream file(strFontPath);
    if (!file.good())
    {
        dlog(LOG_ERROR, "字库路径不存在 [%s]", strFontPath.c_str());
        return ERR_FILE_ERR;
    }

    cvx::CvxFont font(strFontPath);

    /* 画字 */
    for (auto item : listLabelInfo)
    {
        cvx::putText(editImage,
                     item.strLabel,
                     cv::Point(item.nX, item.nY),
                     font,
                     item.nFontSize,
                     cv::Scalar(item.nLabelR, item.nLabelG, item.nLabelB, item.nLabelA));
    }

    return OK;
}

/* 在cv::Mat的基础上划圆 */
BlError_E CImageProcessor::drawCenter(
    cv::Mat&                editImage,
    std::list<CircleInfo_S> listCircleInfo,
    std::string             strFontPath)
{
    if (editImage.empty())
    {
        dlog(LOG_ERROR, "传入的参数异常");
        return ERR_PARAM;
    }

    std::ifstream file(strFontPath);
    if (!file.good())
    {
        dlog(LOG_ERROR, "字库路径不存在 [%s]", strFontPath.c_str());
        return ERR_FILE_ERR;
    }

    cvx::CvxFont font(strFontPath);

    /* 画字 */
    for (auto item : listCircleInfo)
    {
        cv::Point center(item.nCenterX, item.nCenterY);

        cv::Scalar color(item.nR, item.nG, item.nB, item.nA);

        /* 绘制圆形 */
        cv::circle(editImage, center, item.nRadius, color, item.nThickness);
    }

    return OK;
}

/* 画框 */
BlError_E CImageProcessor::drawBox(cv::Mat& editImage, std::list<BoxInfo_S> listBoxInfo)
{
    if (editImage.empty())
    {
        dlog(LOG_ERROR, "传入的参数异常");
        return ERR_PARAM;
    }

    /* 画框 */
    for (auto item : listBoxInfo)
    {
        cv::Rect rect(item.nX,
                      item.nY,
                      item.nW,
                      item.nH);
        /* 在图片上画框 */
        cv::rectangle(editImage,
                      rect,
                      cv::Scalar(item.nBoxR, item.nBoxG, item.nBoxB, item.nBoxA),
                      item.nThickness);


        if (!item.stLabel.strLabel.empty())
        {
            /* 画框的标签 */
            cv::putText(editImage,
                        item.stLabel.strLabel,
                        cv::Point(item.stLabel.nX, item.stLabel.nY),
                        item.stLabel.nFontFace,
                        item.stLabel.dFontScale,
                        cv::Scalar(item.stLabel.nLabelR, item.stLabel.nLabelG, item.stLabel.nLabelB, item.stLabel.nLabelA),
                        item.stLabel.nThickness);
        }
    }
    return OK;
}

/* 画框 */
BlError_E CImageProcessor::drawBox(
    cv::Mat&             editImage,
    std::list<BoxInfo_S> listBoxInfo,
    std::string          strFontPath)
{
    if (editImage.empty())
    {
        dlog(LOG_ERROR, "传入的参数异常");
        return ERR_PARAM;
    }

    std::ifstream file(strFontPath);
    if (!file.good())
    {
        dlog(LOG_ERROR, "字库路径不存在 [%s]", strFontPath.c_str());
        return ERR_FILE_ERR;
    }

    cvx::CvxFont font(strFontPath);

    for (auto item : listBoxInfo)
    {
        cv::Rect rect(item.nX,
                      item.nY,
                      item.nW,
                      item.nH);
        /* 在图片上画框 */
        cv::rectangle(editImage,
                      rect,
                      cv::Scalar(item.nBoxR, item.nBoxG, item.nBoxB, item.nBoxA),
                      item.nThickness);

        if (!item.stLabel.strLabel.empty())
        {
            /* 画框的标签 */
            cvx::putText(editImage,
                         item.stLabel.strLabel,
                         cv::Point(item.stLabel.nX, item.stLabel.nY),
                         font,
                         item.stLabel.nFontSize,
                         cv::Scalar(item.stLabel.nLabelR, item.stLabel.nLabelG, item.stLabel.nLabelB, item.stLabel.nLabelA));
        }
    }

    return OK;
}

/* 创建一个透明的图片，画框 */
BlError_E CImageProcessor::draw(
    DrawParam_S          stParam,
    std::list<BoxInfo_S> listBoxInfo,
    char**               pchOutData,
    int&                 nOutDataSize)
{
    if (nullptr == pchOutData)
    {
        dlog(LOG_ERROR, "传入参数为空");
        return ERR_IN_PARAM_NULL;
    }

    BlError_E enRetCode = OK;

    cv::Mat img(stParam.nOutHeight, stParam.nOutWidth, CV_8UC4, cv::Scalar(stParam.nR, stParam.nG, stParam.nB, stParam.nA));

    /* 画框 */
    drawBox(img, listBoxInfo);

    /* 使用cv::cvtColor函数将透明图像转换格式 */
    cv::Mat imgOut;
    switch (stParam.enOutType)
    {
        case RGB888:
        {
            cv::cvtColor(img, imgOut, cv::COLOR_RGBA2RGB);
            break;
        }
        case BGR888:
        {
            cv::cvtColor(img, imgOut, cv::COLOR_RGBA2BGR);
            break;
        }
        case RGBA8888:
        {
            imgOut = img;
            break;
        }
        default:
        {
            dlog(LOG_ERROR, "输出图片格式设置异常，未定义该类型[%d]", stParam.enOutType);
            enRetCode = NOK;
            break;
        }
    }

    if (enRetCode <= OK)
    {
        nOutDataSize = imgOut.total() * imgOut.elemSize();
        *pchOutData  = new char[nOutDataSize];
        memcpy(*pchOutData, imgOut.data, nOutDataSize);
    }

    return enRetCode;
}

/* 创建一个透明的图片，画框 */
BlError_E CImageProcessor::draw(
    DrawParam_S          stParam,
    std::list<BoxInfo_S> listBoxInfo,
    char**               pchOutData,
    int&                 nOutDataSize,
    std::string          strFontPath)
{
    if (nullptr == pchOutData)
    {
        dlog(LOG_ERROR, "传入参数为空");
        return ERR_IN_PARAM_NULL;
    }

    BlError_E enRetCode = OK;

    cv::Mat img(stParam.nOutHeight, stParam.nOutWidth, CV_8UC4, cv::Scalar(stParam.nR, stParam.nG, stParam.nB, stParam.nA));

    /* 画框 */
    drawBox(img, listBoxInfo, strFontPath);

    /* 使用cv::cvtColor函数将透明图像转换格式 */
    cv::Mat imgOut;
    switch (stParam.enOutType)
    {
        case RGB888:
        {
            cv::cvtColor(img, imgOut, cv::COLOR_RGBA2RGB);
            break;
        }
        case BGR888:
        {
            cv::cvtColor(img, imgOut, cv::COLOR_RGBA2BGR);
            break;
        }
        case RGBA8888:
        {
            imgOut = img;
            break;
        }
        default:
        {
            dlog(LOG_ERROR, "输出图片格式设置异常，未定义该类型[%d]", stParam.enOutType);
            enRetCode = NOK;
            break;
        }
    }

    if (enRetCode <= OK)
    {
        nOutDataSize = imgOut.total() * imgOut.elemSize();
        *pchOutData  = new char[nOutDataSize];
        memcpy(*pchOutData, imgOut.data, nOutDataSize);
    }

    return enRetCode;
}

/* 创建一个透明的图片，画框和写标签 */
BlError_E CImageProcessor::draw(
    DrawParam_S            stParam,
    std::list<BoxInfo_S>   listBoxInfo,
    std::list<LabelInfo_S> listLabelInfo,
    char**                 pchOutData,
    int&                   nOutDataSize)
{
    if (nullptr == pchOutData)
    {
        dlog(LOG_ERROR, "传入参数为空");
        return ERR_IN_PARAM_NULL;
    }

    BlError_E enRetCode = OK;

    cv::Mat img(stParam.nOutHeight, stParam.nOutWidth, CV_8UC4, cv::Scalar(stParam.nR, stParam.nG, stParam.nB, stParam.nA));

    /* 画框 */
    drawBox(img, listBoxInfo);

    /* 画字 */
    drawLabel(img, listLabelInfo);

    /* 使用cv::cvtColor函数将透明图像转换格式 */
    cv::Mat imgOut;
    switch (stParam.enOutType)
    {
        case RGB888:
        {
            cv::cvtColor(img, imgOut, cv::COLOR_RGBA2RGB);
            break;
        }
        case BGR888:
        {
            cv::cvtColor(img, imgOut, cv::COLOR_RGBA2BGR);
            break;
        }
        case RGBA8888:
        {
            imgOut = img;
            break;
        }
        default:
        {
            dlog(LOG_ERROR, "输出图片格式设置异常，未定义该类型[%d]", stParam.enOutType);
            enRetCode = NOK;
            break;
        }
    }

    if (enRetCode <= OK)
    {
        nOutDataSize = imgOut.total() * imgOut.elemSize();
        *pchOutData  = new char[nOutDataSize];
        memcpy(*pchOutData, imgOut.data, nOutDataSize);
    }

    return enRetCode;
}

/* 创建一个透明的图片，画框和写标签 */
BlError_E CImageProcessor::draw(
    DrawParam_S            stParam,
    std::list<BoxInfo_S>   listBoxInfo,
    std::list<LabelInfo_S> listLabelInfo,
    char**                 pchOutData,
    int&                   nOutDataSize,
    std::string            strFontPath)
{
    if (nullptr == pchOutData)
    {
        dlog(LOG_ERROR, "传入参数为空");
        return ERR_IN_PARAM_NULL;
    }


    BlError_E enRetCode = OK;

    cv::Mat img(stParam.nOutHeight, stParam.nOutWidth, CV_8UC4, cv::Scalar(stParam.nR, stParam.nG, stParam.nB, stParam.nA));

    /* 画框 */
    drawBox(img, listBoxInfo, strFontPath);

    /* 画字 */
    drawLabel(img, listLabelInfo, strFontPath);


    /* 使用cv::cvtColor函数将透明图像转换格式 */
    cv::Mat imgOut;
    switch (stParam.enOutType)
    {
        case RGB888:
        {
            cv::cvtColor(img, imgOut, cv::COLOR_RGBA2RGB);
            break;
        }
        case BGR888:
        {
            cv::cvtColor(img, imgOut, cv::COLOR_RGBA2BGR);
            break;
        }
        case RGBA8888:
        {
            imgOut = img;
            break;
        }
        default:
        {
            dlog(LOG_ERROR, "输出图片格式设置异常，未定义该类型[%d]", stParam.enOutType);
            enRetCode = NOK;
            break;
        }
    }

    if (enRetCode <= OK)
    {
        nOutDataSize = imgOut.total() * imgOut.elemSize();
        *pchOutData  = new char[nOutDataSize];
        memcpy(*pchOutData, imgOut.data, nOutDataSize);
    }

    return enRetCode;
}

/* 创建一个透明的图片，画框、写标签和画直线 */
BlError_E CImageProcessor::draw(
    DrawParam_S            stParam,
    std::list<BoxInfo_S>   listBoxInfo,
    std::list<LabelInfo_S> listLabelInfo,
    std::list<LineInfo_S>  listLineInfo,
    char**                 pchOutData,
    int&                   nOutDataSize)
{
    if (nullptr == pchOutData)
    {
        dlog(LOG_ERROR, "传入参数为空");
        return ERR_IN_PARAM_NULL;
    }

    BlError_E enRetCode = OK;

    cv::Mat img(stParam.nOutHeight,
                stParam.nOutWidth,
                CV_8UC4,
                cv::Scalar(stParam.nR, stParam.nG, stParam.nB, stParam.nA));

    /* 画框 */
    drawBox(img, listBoxInfo);

    /* 画字 */
    drawLabel(img, listLabelInfo);

    /* 划线 */
    drawLines(img, listLineInfo);

    /* 使用cv::cvtColor函数将透明图像转换格式 */
    cv::Mat imgOut;
    switch (stParam.enOutType)
    {
        case RGB888:
        {
            cv::cvtColor(img, imgOut, cv::COLOR_RGBA2RGB);
            break;
        }
        case BGR888:
        {
            cv::cvtColor(img, imgOut, cv::COLOR_RGBA2BGR);
            break;
        }
        case RGBA8888:
        {
            imgOut = img;
            break;
        }
        default:
        {
            dlog(LOG_ERROR, "输出图片格式设置异常，未定义该类型[%d]", stParam.enOutType);
            enRetCode = NOK;
            break;
        }
    }

    if (enRetCode <= OK)
    {
        nOutDataSize = imgOut.total() * imgOut.elemSize();
        *pchOutData  = new char[nOutDataSize];
        memcpy(*pchOutData, imgOut.data, nOutDataSize);
    }

    return enRetCode;
}

/* 创建一个透明的图片，画框、写标签和画直线 */
BlError_E CImageProcessor::draw(
    DrawParam_S            stParam,
    std::list<BoxInfo_S>   listBoxInfo,
    std::list<LabelInfo_S> listLabelInfo,
    std::list<LineInfo_S>  listLineInfo,
    char**                 pchOutData,
    int&                   nOutDataSize,
    std::string            strFontPath)
{
    if (nullptr == pchOutData)
    {
        dlog(LOG_ERROR, "传入参数为空");
        return ERR_IN_PARAM_NULL;
    }

    BlError_E enRetCode = OK;

    cv::Mat img(stParam.nOutHeight,
                stParam.nOutWidth,
                CV_8UC4,
                cv::Scalar(stParam.nR, stParam.nG, stParam.nB, stParam.nA));

    /* 画框 */
    drawBox(img, listBoxInfo, strFontPath);

    /* 画字 */
    drawLabel(img, listLabelInfo, strFontPath);

    /* 划线 */
    drawLines(img, listLineInfo);

    /* 使用cv::cvtColor函数将透明图像转换格式 */
    cv::Mat imgOut;
    switch (stParam.enOutType)
    {
        case RGB888:
        {
            cv::cvtColor(img, imgOut, cv::COLOR_RGBA2RGB);
            break;
        }
        case BGR888:
        {
            cv::cvtColor(img, imgOut, cv::COLOR_RGBA2BGR);
            break;
        }
        case RGBA8888:
        {
            imgOut = img;
            break;
        }
        default:
        {
            dlog(LOG_ERROR, "输出图片格式设置异常，未定义该类型[%d]", stParam.enOutType);
            enRetCode = NOK;
            break;
        }
    }

    if (enRetCode <= OK)
    {
        nOutDataSize = imgOut.total() * imgOut.elemSize();
        *pchOutData  = new char[nOutDataSize];
        memcpy(*pchOutData, imgOut.data, nOutDataSize);

        // cv::imwrite("111.jpg", imgOut);
    }

    return enRetCode;
}

/* 创建一个透明的图片，画画 */
BlError_E CImageProcessor::draw(
    DrawParam_S             stParam,
    std::list<BoxInfo_S>    listBoxInfo,
    std::list<LabelInfo_S>  listLabelInfo,
    std::list<LineInfo_S>   listLineInfo,
    std::list<CircleInfo_S> listCircleInfo,
    char**                  pchOutData,
    int&                    nOutDataSize,
    std::string             strFontPath)
{
    if (nullptr == pchOutData)
    {
        dlog(LOG_ERROR, "传入参数为空");
        return ERR_IN_PARAM_NULL;
    }

    BlError_E enRetCode = OK;

    cv::Mat img(stParam.nOutHeight,
                stParam.nOutWidth,
                CV_8UC4,
                cv::Scalar(stParam.nR, stParam.nG, stParam.nB, stParam.nA));

    /* 画框 */
    drawBox(img, listBoxInfo, strFontPath);

    /* 画字 */
    drawLabel(img, listLabelInfo, strFontPath);

    /* 画线 */
    drawLines(img, listLineInfo);

    /* 画圆 */
    drawCenter(img, listCircleInfo, strFontPath);

    /* 使用cv::cvtColor函数将透明图像转换格式 */
    cv::Mat imgOut;
    switch (stParam.enOutType)
    {
        case RGB888:
        {
            cv::cvtColor(img, imgOut, cv::COLOR_RGBA2RGB);
            break;
        }
        case BGR888:
        {
            cv::cvtColor(img, imgOut, cv::COLOR_RGBA2BGR);
            break;
        }
        case RGBA8888:
        {
            imgOut = img;
            break;
        }
        default:
        {
            dlog(LOG_ERROR, "输出图片格式设置异常，未定义该类型[%d]", stParam.enOutType);
            enRetCode = NOK;
            break;
        }
    }

    if (enRetCode <= OK)
    {
        nOutDataSize = imgOut.total() * imgOut.elemSize();
        *pchOutData  = new char[nOutDataSize];
        memcpy(*pchOutData, imgOut.data, nOutDataSize);
    }

    return enRetCode;
}

/* 将二进值数据保存成图片 */
BlError_E CImageProcessor::saveImage(
    DataFormatType_E   enDataType,
    const char*        pchData,
    int                nWidth,
    int                nHeigh,
    const std::string& strFilePath)
{
    if (nullptr == pchData || 0 >= nWidth || 0 >= nHeigh)
    {
        dlog(LOG_ERROR, "传入参数为空");
        return ERR_IN_PARAM_NULL;
    }

    /* 检查文件路径是否有效 */
    if (strFilePath.empty())
    {
        dlog(LOG_ERROR, "文件路径为空");
        return ERR_IN_PARAM_NULL;
    }

    /* 将二进制数据转换为cv::Mat */
    cv::Mat imageMat;
    switch (enDataType)
    {
        case RGB888:
        case BGR888:
        {
            imageMat = cv::Mat(nHeigh, nWidth, CV_8UC3, (void*)pchData);
            break;
        }
        case RGBA8888:
        {
            imageMat = cv::Mat(nHeigh, nWidth, CV_8UC4, (void*)pchData);
            break;
        }
        default:
        {
            dlog(LOG_ERROR, "输入数据格式设置异常，未定义该类型[%d]", enDataType);
            return NOK;
        }
    }

    /* 检查图像是否成功加载 */
    if (imageMat.empty())
    {
        dlog(LOG_ERROR, "保存图片失败-图像加载失败");
        return NOK;
    }

    /* 保存图像到文件 */
    try
    {
        if (cv::imwrite(strFilePath, imageMat))
        {
            dlog(LOG_TRACE, "保存图片成功 [%s]", strFilePath.c_str());
            return OK;
        }
        else
        {
            dlog(LOG_ERROR, "保存图片失败 [%s]", strFilePath.c_str());
            return NOK;
        }
    }
    catch (cv::Exception& e)
    {
        /* 获取错误信息 */
        dlog(LOG_ERROR, "保存图片失败 [%s]", e.what());
        return NOK;
    }

    return NOK;
}

/* 将二进值数据保存成图片 */
BlError_E CImageProcessor::saveImage(
    DataFormatType_E   enDataType,
    const char*        pchData,
    int                nWidth,
    int                nHeigh,
    const std::string& strFilePath,
    FileFormatType_E   enFileType)
{
    if (nullptr == pchData || 0 >= nWidth || 0 >= nHeigh)
    {
        dlog(LOG_ERROR, "传入参数为空");
        return ERR_IN_PARAM_NULL;
    }

    /* 检查文件路径是否有效 */
    if (strFilePath.empty())
    {
        dlog(LOG_ERROR, "文件路径为空");
        return ERR_IN_PARAM_NULL;
    }

    /* 将二进制数据转换为cv::Mat */
    cv::Mat imageMat;
    switch (enDataType)
    {
        case RGB888:
        case BGR888:
        {
            imageMat = cv::Mat(nHeigh, nWidth, CV_8UC3, (void*)pchData);
            break;
        }
        case RGBA8888:
        {
            imageMat = cv::Mat(nHeigh, nWidth, CV_8UC4, (void*)pchData);
            break;
        }
        default:
        {
            dlog(LOG_ERROR, "输入数据格式设置异常，未定义该类型[%d]", enDataType);
            return NOK;
        }
    }

    /* 检查图像是否成功加载 */
    if (imageMat.empty())
    {
        dlog(LOG_ERROR, "保存图片失败-图像加载失败");
        return NOK;
    }

    std::vector<int> vnParams;
    switch (enFileType)
    {
        case JPEG:
        {
            vnParams.push_back(cv::IMWRITE_JPEG_QUALITY);
            /* 设置JPEG质量，范围0-100 */
            vnParams.push_back(95);
            break;
        }
        case PNG:
        {
            vnParams.push_back(cv::IMWRITE_PNG_COMPRESSION);
            /* 设置PNG压缩级别，范围0-9 */
            vnParams.push_back(3);
            break;
        }
        default:
        {
            dlog(LOG_ERROR, "输出文件格式设置异常，未定义该类型[%d]", enDataType);
            return NOK;
        }
    }

    /* 保存图像到文件 */
    try
    {
        if (cv::imwrite(strFilePath, imageMat, vnParams))
        {
            dlog(LOG_TRACE, "保存图片成功 [%s]", strFilePath.c_str());
            return OK;
        }
        else
        {
            dlog(LOG_ERROR, "保存图片失败 [%s]", strFilePath.c_str());
            return NOK;
        }
    }
    catch (cv::Exception& e)
    {
        /* 获取错误信息 */
        dlog(LOG_ERROR, "保存图片失败 [%s]", e.what());
        return NOK;
    }

    return NOK;
}

/* 将二进值数据保存成文件 */
BlError_E CImageProcessor::saveBinaryDataToFile(
    const char* pchData,
    size_t      nSize,
    const char* pchFilePath)
{
    /* 打开文件以写入二进制数据 */
    FILE* pFile = fopen(pchFilePath, "wb");

    if (pFile == NULL)
    {
        dlog(LOG_ERROR, "打开文件失败 [%s]", pchFilePath);
        return NOK;
    }

    /* 写入二进制数据到文件 */
    size_t nWritten = fwrite(pchData, 1, nSize, pFile);

    /* 关闭文件 */
    fclose(pFile);

    if (nWritten != nSize)
    {
        dlog(LOG_ERROR, "写入文件失败 [%s]", pchFilePath);
        return NOK;
    }

    return OK;
}

/* 转换图片数据 */
BlError_E CImageProcessor::transition(
    DataFormatType_E enInType,
    int              nInWidth,
    int              nInHeight,
    const char*      pchInData,
    int              nInDataSize,
    DataFormatType_E enOutType,
    int              nOutWidth,
    int              nOutHeight,
    char**           pchOutData,
    int&             nOutDataSize)
{
    if (nullptr == pchInData || nullptr == pchOutData || nInDataSize <= 0)
    {
        dlog(LOG_ERROR, "传入参数为空");
        return ERR_IN_PARAM_NULL;
    }

    cv::Mat inImage;
    cv::Mat outImage;

    switch (enInType)
    {
        case RGB888:
        case BGR888:
        {
            inImage = cv::Mat(nInHeight, nInWidth, CV_8UC3, (void*)pchInData);
            break;
        }
        case RGBA8888:
        {
            inImage = cv::Mat(nInHeight, nInWidth, CV_8UC4, (void*)pchInData);
            break;
        }
        default:
        {
            dlog(LOG_ERROR, "输入数据格式设置异常，未定义该类型[%d]", enInType);
            return NOK;
        }
    }


    switch (enInType)
    {
        case RGB888:
        {
            switch (enOutType)
            {
                case RGB888:
                {
                    outImage = inImage;
                    break;
                }
                case BGR888:
                {
                    cv::cvtColor(inImage, outImage, cv::COLOR_RGB2BGR);
                    break;
                }
                case RGBA8888:
                {
                    cv::cvtColor(inImage, outImage, cv::COLOR_RGB2RGBA);
                    break;
                }
                default:
                {
                    dlog(LOG_ERROR, "输入数据格式设置异常，未定义该类型[%d]", enInType);
                    return NOK;
                }
            }
            break;
        }
        case BGR888:
        {
            switch (enOutType)
            {
                case RGB888:
                {
                    cv::cvtColor(inImage, outImage, cv::COLOR_BGR2RGB);
                    break;
                }
                case BGR888:
                {
                    outImage = inImage;
                    break;
                }
                case RGBA8888:
                {
                    cv::cvtColor(inImage, outImage, cv::COLOR_BGR2RGBA);
                    break;
                }
                default:
                {
                    dlog(LOG_ERROR, "输入数据格式设置异常，未定义该类型[%d]", enInType);
                    return NOK;
                }
            }
            break;
        }
        case RGBA8888:
        {
            switch (enOutType)
            {
                case RGB888:
                {
                    cv::cvtColor(inImage, outImage, cv::COLOR_RGBA2RGB);
                    break;
                }
                case BGR888:
                {
                    cv::cvtColor(inImage, outImage, cv::COLOR_RGBA2BGR);
                    break;
                }
                case RGBA8888:
                {
                    outImage = inImage;
                    break;
                }
                default:
                {
                    dlog(LOG_ERROR, "输入数据格式设置异常，未定义该类型[%d]", enInType);
                    return NOK;
                }
            }
            break;
        }
        default:
        {
            dlog(LOG_ERROR, "输入数据格式设置异常，未定义该类型[%d]", enInType);
            return NOK;
        }
    }

    cv::Mat scaledImage;
    if (nInWidth != nOutWidth || nInHeight != nOutHeight)
    {
        cv::resize(outImage, scaledImage, cv::Size(nOutWidth, nOutHeight));
    }
    else
    {
        scaledImage = outImage;
    }


    nOutDataSize = scaledImage.total() * scaledImage.elemSize();
    *pchOutData  = new char[nOutDataSize];
    memcpy(*pchOutData, scaledImage.data, nOutDataSize);

    return OK;
}

/* 转换图片数据 */
BlError_E CImageProcessor::transition(
    DataFormatType_E enInType,
    int              nInWidth,
    int              nInHeight,
    const char*      pchInData,
    int              nInDataSize,
    DataFormatType_E enOutType,
    int              nOutWidth,
    int              nOutHeight,
    cv::Mat&         outImage)
{
    if (nullptr == pchInData || nInDataSize <= 0)
    {
        dlog(LOG_ERROR, "传入参数为空");
        return ERR_IN_PARAM_NULL;
    }

    cv::Mat inImage;
    cv::Mat outTmpImage;

    switch (enInType)
    {
        case RGB888:
        case BGR888:
        {
            inImage = cv::Mat(nInHeight, nInWidth, CV_8UC3, (void*)pchInData);
            break;
        }
        case RGBA8888:
        {
            inImage = cv::Mat(nInHeight, nInWidth, CV_8UC4, (void*)pchInData);
            break;
        }
        default:
        {
            dlog(LOG_ERROR, "输入数据格式设置异常，未定义该类型[%d]", enInType);
            return NOK;
        }
    }


    switch (enInType)
    {
        case RGB888:
        {
            switch (enOutType)
            {
                case RGB888:
                {
                    outTmpImage = inImage;
                    break;
                }
                case BGR888:
                {
                    cv::cvtColor(inImage, outTmpImage, cv::COLOR_RGB2BGR);
                    break;
                }
                case RGBA8888:
                {
                    cv::cvtColor(inImage, outTmpImage, cv::COLOR_RGB2RGBA);
                    break;
                }
                default:
                {
                    dlog(LOG_ERROR, "输入数据格式设置异常，未定义该类型[%d]", enInType);
                    return NOK;
                }
            }
            break;
        }
        case BGR888:
        {
            switch (enOutType)
            {
                case RGB888:
                {
                    cv::cvtColor(inImage, outTmpImage, cv::COLOR_BGR2RGB);
                    break;
                }
                case BGR888:
                {
                    outTmpImage = inImage;
                    break;
                }
                case RGBA8888:
                {
                    cv::cvtColor(inImage, outTmpImage, cv::COLOR_BGR2RGBA);
                    break;
                }
                default:
                {
                    dlog(LOG_ERROR, "输入数据格式设置异常，未定义该类型[%d]", enInType);
                    return NOK;
                }
            }
            break;
        }
        case RGBA8888:
        {
            switch (enOutType)
            {
                case RGB888:
                {
                    cv::cvtColor(inImage, outTmpImage, cv::COLOR_RGBA2RGB);
                    break;
                }
                case BGR888:
                {
                    cv::cvtColor(inImage, outTmpImage, cv::COLOR_RGBA2BGR);
                    break;
                }
                case RGBA8888:
                {
                    outTmpImage = inImage;
                    break;
                }
                default:
                {
                    dlog(LOG_ERROR, "输入数据格式设置异常，未定义该类型[%d]", enInType);
                    return NOK;
                }
            }
            break;
        }
        default:
        {
            dlog(LOG_ERROR, "输入数据格式设置异常，未定义该类型[%d]", enInType);
            return NOK;
        }
    }

    if (nInWidth != nOutWidth || nInHeight != nOutHeight)
    {
        cv::resize(outTmpImage, outImage, cv::Size(nOutWidth, nOutHeight));
    }
    else
    {
        outImage = outTmpImage;
    }

    return OK;
}
