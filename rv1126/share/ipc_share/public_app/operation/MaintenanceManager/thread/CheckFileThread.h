/*
 * @FilePath     : CheckFileThread.h
 * @Author       : xiezhh
 * @Date         : 2024-06-04 17:23
 * @LastEditors  : xiezhh
 * @LastEditTime : 2024-06-25 15:24
 * @Description  : 根据初始化的路径查找该路径下有还有什么文件
 *                 未进行上传的一个线程
 */
#ifndef CHECKFILETHREAD_H
#define CHECKFILETHREAD_H

#include "MaintenanceThread.h"
#include "common/MaintenanceStruct.h"
#include "common/MaintenanceJsonParse.h"

class CCheckFileThread : public CMaintenanceThread
{
public:
    CCheckFileThread();

    /**
     * @brief 初始化
     */
    void init();
    /**
     * @brief 是否初始化
     * @return [bool] true：已初始化，false：未初始化
     */
    bool isInit();

protected:
    /**
     * @brief 线程运行函数
     * @note 在这个函数中进行while循环
     */
    virtual void run() override;

private:
    /**
     * @brief 获取本日生成的本地记录文件路径
     * @return [string] 本日的记录文件路径
     */
    std::string getRecordFileName();

    /**
     * @brief 遍历文件夹
     * @param [string] 文件夹路径
     */
    void iterateDir(const std::string &strFilePath);

    /**
     * @brief 匹配文件
     * @param [string] 文件名称
     * @param [out][string] 文件名称中匹配出的日期
     * @param [int] 文件类型
     * @return [boot] true：匹配成功，false：当前文件匹配失败
     * @note 过滤不需要的文件
     */
    bool matchFile(std::string strFileName, std::string &strDate, int &fileType);

private:
    /* 是否初始化的标志 */
    bool m_bIsInit = false;

    /* 设备唯一机器码 */
    std::string m_strDeviceCode;

    /* 是否需要读取过滤列表的标志 */
    std::atomic<bool> m_bIsReadFilterFileFlag = false;
    /* 是否要读取本地记录文件的标志 */
    std::atomic<bool> m_bIsReadRecordFileFlag = false;

    /* 记录文件路径，不包含文件名称 */
    std::string m_strRecordFilePath;
    /* 记录文件路径，包含文件名称 */
    std::string m_strRecordFileFullPath;
    /* 过滤文件路径，包含文件名称 */
    std::string m_strFilterFileFullPath;

    /*今天日期*/
    std::string m_strCurDate;

    /* 查找的目录列表 */
    std::vector<std::string> m_vecPaths;
    /* 需要匹配的文件名称 */
    std::vector<MaintenanceNS::UploadConfig> m_vecRegex;
    /* 过滤的日期 */
    std::vector<std::string> m_vecFilterDate;
};

#endif // CHECKFILETHREAD_H
