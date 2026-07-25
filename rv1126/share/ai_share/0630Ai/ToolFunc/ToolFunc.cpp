/*
 * @FilePath     : ToolFunc.cpp
 * @Author       : 严泽辉 yanzeh@kfb.cn
 * @Date         : 2024-04-01 16:55:26
 * @LastEditors  : 严泽辉 yanzeh@kfb.cn
 * @LastEditTime : 2024-08-07 17:14:52
 * @Description  :
 */
#include "ToolFunc.hpp"

#include <fcntl.h>
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
bool ToolFunc::makeDirectory(std::string strPath)
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
bool ToolFunc::rmDirectory(std::string strPath)
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
bool ToolFunc::writeDataToFile(const char* pchFileName, const void* pData, size_t nDataSize)
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
bool ToolFunc::resizeImage(
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

/* 获取微秒级时间戳 */
long long ToolFunc::getTimeStampUs()
{
    return std::chrono::duration_cast<std::chrono::microseconds>(
               std::chrono::system_clock::now().time_since_epoch())
        .count();
}

/* 文件锁 */
static void lock_file(FILE* pFp, int nLockType)
{
    struct flock stFl;
    /* F_RDLCK, F_WRLCK, F_UNLCK */
    stFl.l_type   = nLockType;
    stFl.l_whence = SEEK_SET;
    stFl.l_start  = 0;
    /*锁定整个文件*/
    stFl.l_len    = 0;
    stFl.l_pid    = getpid();

    if (fcntl(fileno(pFp), F_SETLKW, &stFl) == -1)
    {
        dlog(LOG_ERROR, "fcntl error\n");
        return;
    }
    return;
}

/* 将Json数据写入文件中 */
bool ToolFunc::writeJson_to_file(const char* pchFilePath, const char* pchJsonData)
{
    if (pchFilePath == NULL || pchJsonData == NULL)
    {
        dlog(LOG_ERROR, "[写入文件] 传入参数异常");
        return false;
    }

    struct stat stFileStat = { 0 };
    if (stat(pchFilePath, &stFileStat) == 0)
    {
        if (S_ISDIR(stFileStat.st_mode))
        {
            dlog(LOG_ERROR, "[写入文件] 传入的文件路径为文件夹[%s]", pchFilePath);
            return false;
        }
        else if (access(pchFilePath, W_OK) != 0)
        {
            if (chmod(pchFilePath, S_IWUSR) != 0)
            {
                dlog(LOG_ERROR, "[写入文件] 没有写权限，添加写权限失败[%s]", pchFilePath);
                return false;
            }
        }
    }

    FILE* pFp = fopen(pchFilePath, "w+");
    if (pFp == NULL)
    {
        dlog(LOG_ERROR, "[写入文件] 打开文件失败[%s]", pchFilePath);
        return false;
    }

    size_t nLen = strlen(pchJsonData) + 1;
    /*设置写锁*/
    lock_file(pFp, F_WRLCK);
    size_t nWritten = fwrite(pchJsonData, sizeof(char), nLen, pFp);
    /*解锁*/
    lock_file(pFp, F_UNLCK);

    fclose(pFp);

    if (nWritten != nLen)
    {
        dlog(LOG_ERROR, "[写入文件] 写文件失败[%s]", pchFilePath);
        return false;
    }

    return true;
}

/* 读取文件中的Json数据 */
char* ToolFunc::readJson_from_file(const char* pchFilePath)
{
    if (pchFilePath == NULL)
    {
        dlog(LOG_ERROR, "[读取文件] 传入参数异常");
        return NULL;
    }

    struct stat stFileStat = { 0 };

    if (stat(pchFilePath, &stFileStat) != 0)
    {
        dlog(LOG_ERROR, "[读取文件] 文件[%s]信息异常[%s]", pchFilePath, strerror(errno));
        return NULL;
    }

    if (S_ISDIR(stFileStat.st_mode))
    {
        dlog(LOG_ERROR, "[读取文件] 传入的文件路径为文件夹[%s]", pchFilePath);
        return NULL;
    }

    FILE* pFp = fopen(pchFilePath, "r");
    if (pFp == NULL)
    {
        dlog(LOG_ERROR, "[读取文件] 打开文件失败[%s]", pchFilePath);
        return NULL;
    }

    size_t nSize       = stFileStat.st_size;
    char*  pchJsonData = (char*)malloc(nSize + 1);
    if (pchJsonData == NULL)
    {
        dlog(LOG_ERROR, "[读取文件] 创建空间失败");
        fclose(pFp);
        return NULL;
    }

    size_t nReadSize = fread(pchJsonData, sizeof(char), nSize, pFp);
    if (nReadSize != nSize)
    {
        dlog(LOG_ERROR, "[读取文件] 读取数据长度异常");
        free(pchJsonData);
        fclose(pFp);
        return NULL;
    }

    pchJsonData[nSize] = '\0';

    fclose(pFp);

    return pchJsonData;
}