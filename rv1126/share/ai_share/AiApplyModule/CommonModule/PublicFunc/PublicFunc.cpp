/*
 * @FilePath     : PublicFunc.cpp
 * @Author       : 严泽辉 yanzeh@kfb.cn
 * @Date         : 2024-04-01 16:55:26
 * @LastEditors  : 严泽辉 yanzeh@kfb.cn
 * @LastEditTime : 2024-08-07 17:14:52
 * @Description  :
 */
#include "PublicFunc.hpp"

#include <sys/stat.h>

#include "dlog.h"
#include "opencv2/opencv.hpp"

#ifdef _WIN32
    #include <direct.h>
    #define mkdir(dir, mode) _mkdir(dir)
#else
    #include <unistd.h>
#endif


/* 创建目录 */
bool CPublicFunc::makeDirectory(std::string strPath)
{
    if (strPath.empty())
    {
        std::cerr << "创建文件夹失败-传入参数为空" << std::endl;
        return false;
    }

    int nRet = 0;

    struct stat stStat = { 0 };

    nRet = stat(strPath.c_str(), &stStat);

    if (nRet != 0 || !S_ISDIR(stStat.st_mode))
    {
        char achCommand[1024] = { 0 };
        snprintf(achCommand, sizeof(achCommand), "mkdir -p %s", strPath.c_str());

        /* 使用 system 函数调用命令行 */
        int nStatus = system(achCommand);

        if (nStatus != 0)
        {
            std::cerr << "创建目录失败: " << strPath << std::endl;
            return false;
        }

        return false;
    }

    return true;
}

/* 删除目录 */
bool CPublicFunc::rmDirectory(std::string strPath)
{
    struct stat buffer;
    if (stat(strPath.c_str(), &buffer) == 0)
    {
        if (S_ISREG(buffer.st_mode))
        {
            /*文件存在*/
            if (unlink(strPath.c_str()) == 0)
            {
                return true;
            }
            else
            {
                return false;
            }
        }
        else if (S_ISDIR(buffer.st_mode))
        {
            /*目录存在*/
            if (rmdir(strPath.c_str()) == 0)
            {
                return true;
            }
            else
            {
                return false;
            }
        }
        else
        {
            return false;
        }
    }
    else
    {
        return false;
    }
    return true;
}

/* 打开文件并清空原有的数据，写入文件 */
bool CPublicFunc::writeDataToFile(const char* pchFileName, const void* pData, size_t nDataSize)
{
    if (pData == NULL || nDataSize <= 0 || pchFileName == NULL)
    {
        dlog(LOG_ERROR, "参数异常");
        return false;
    }

    /* 打开文件以追加数据 */
    FILE* pFile = fopen(pchFileName, "w+");
    if (!pFile)
    {
        dlog(LOG_ERROR, "无法打开文件 %s", pchFileName);
        return false;
    }

    /* 写入数据到文件 */
    size_t nBytesWritten = fwrite(pData, 1, nDataSize, pFile);
    if (nBytesWritten != nDataSize)
    {
        dlog(LOG_ERROR, "写入数据到文件 %s 失败", pchFileName);

        if (pFile)
        {
            fclose(pFile);
        }
        return false;
    }

    /* 关闭文件 */
    if (pFile)
    {
        fclose(pFile);
    }

    return true;
}

/* 调整图片大小 */
bool CPublicFunc::resizeImage(
    const std::string& strSrcImagePath,
    const std::string& strDstImagePath,
    int                nTargetWidth,
    int                nTargetHeight)
{
    /* 加载原始图片 */
    cv::Mat srcImage = cv::imread(strSrcImagePath);
    if (srcImage.empty())
    {
        return false;
    }

    /* 获取原始图片的尺寸 */
    int nSrcWidth  = srcImage.cols;
    int nSrcHeight = srcImage.rows;

    /* 计算缩放比例 */
    float fScaleWidth  = static_cast<float>(nTargetWidth) / nSrcWidth;
    float fScaleHeight = static_cast<float>(nTargetHeight) / nSrcHeight;
    float fScaleFactor = std::min(fScaleWidth, fScaleHeight);

    /* 计算缩放后的尺寸 */
    int nScaledWidth  = static_cast<int>(nSrcWidth * fScaleFactor);
    int nScaledHeight = static_cast<int>(nSrcHeight * fScaleFactor);

    /* 如果原始图片的尺寸大于目标尺寸，则进行缩放 */
    if (fScaleFactor < 1.0f)
    {
        cv::resize(srcImage, srcImage, cv::Size(nScaledWidth, nScaledHeight));
    }

    /* 创建一个新图片，用于容纳扩展后的图像 */
    cv::Mat newImage(nTargetHeight, nTargetWidth, srcImage.type(), cv::Scalar(128, 128, 128));

    /* 计算原始图片在新图片中的位置 */
    int nXOffset = (nTargetWidth - srcImage.cols) / 2;
    int nYOffset = (nTargetHeight - srcImage.rows) / 2;

    /* 将原始图片复制到新图片的中心位置 */
    cv::Mat roi = newImage(cv::Rect(nXOffset, nYOffset, srcImage.cols, srcImage.rows));
    srcImage.copyTo(roi);

    /* 保存结果 */
    cv::imwrite(strDstImagePath, newImage);

    return true;
}
