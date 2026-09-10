/*
 * @FilePath     : UploadThread.h
 * @Author       : xiezhh
 * @Date         : 2024-06-04 17:23
 * @LastEditors  : xiezhh
 * @LastEditTime : 2024-06-25 15:24
 * @Description  : 上传文件的线程
 */
#ifndef UPLOADTHREAD_H
#define UPLOADTHREAD_H

#include "MaintenanceThread.h"
#include "common/MaintenanceJsonParse.h"
#include "CurlMultipartHttpPost.h"

class CUploadThread : public CMaintenanceThread
{
public:
    CUploadThread();

    /**
     * @brief 初始化
     * @return [bool] true：初始化成功，false：初始化失败
     */
    bool init();
    /**
     * @brief 是否初始化
     * @return [bool] true：初始化成功，false：未初始化
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
     * @brief 上传前检查和校验，内部调用上传函数
     * @param [RecordInfo] 待上传的记录信息
     * @return [bool] 上传结果，true：成功，false：失败
     */
    bool checkRecordInfo(MaintenanceNS::RecordInfo &stRecordInfo);
    /**
     * @brief 文件分片
     * @param [RecordInfo] 待上传的记录信息
     */
    void sliceFile(MaintenanceNS::RecordInfo &stRecordInfo);
    /**
     * @brief 上传函数
     * @param [string] 请求路径
     * @param [string] 请求的token
     * @param [int]    项目的唯一ID
     * @return [bool] 上传结果，true：成功，false：失败
     */
    bool sendUploadReq(std::string strUploadUrl,
                       std::string &strToken,
                       int &nProjectID);

private:
    /* 是否初始化的标志，true：已初始化 */
    bool m_isInit = false;

    /* 解析Json的类 */
    CMaintenanceJsonParse m_cJson;

    /* 修改Url会用到的互斥锁 */
    std::mutex m_mutex;
    /* 上传文件的Post指针 */
    CurlHttp::CCurlMultipartHttpPost *m_pPost = nullptr;

    /* 上传列表 */
    std::vector<MaintenanceNS::UploadData> m_vecUploadData;
};

#endif // UPLOADTHREAD_H
