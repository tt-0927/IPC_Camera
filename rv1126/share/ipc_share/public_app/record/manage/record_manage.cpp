/**
 * @FilePath     : record_manage.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2025-06-28 10:36:11
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2025-07-08 17:26:53
 * @Description  : 录制管理
 */

#include "record_manage.h"

CRecordManage::CRecordManage()
{
    /* 创建流数据接受通信 */
    m_pStreamClient = std::make_shared<StreamClient>();
    m_pStreamClient->init(IN_RECORD_STREAM_PROT_ONE);
}

CRecordManage::~CRecordManage()
{
    // for (auto it: m_processInfos)
    // {
    //     it.pRecordHandler.reset();
    // }
    // m_pStreamClient->deinit();
    // m_pStreamClient.reset();
}

int CRecordManage::set_record_info(Record_NS::Info_S &stRecordInfo)
{
    auto pRecord = get_recordHander(stRecordInfo);
    if (pRecord == nullptr)
    {
        return ERR_PTR_NULL;
    }
    switch (stRecordInfo.nRecordStatus)
    {
    case Record_NS::RECORD_OPERATION: // 录制操作
        pRecord->start(stRecordInfo);
        break;
    case Record_NS::PAUSE_OPERATION: // 暂停操作
        pRecord->pause();
        break;
    case Record_NS::STOP_OPERATION: // 停止操作
        pRecord->stop();
        /* 删除非常规的录像文件 */
        if (stRecordInfo.nEventType != 0)
        {
            // ProcessInfo_S stProcessInfo;
            // stProcessInfo.nChnId = stRecordInfo.nChnId;
            // stProcessInfo.nEventType = stRecordInfo.nEventType;
            // m_processInfos.erase(stProcessInfo);

            // m_stProcessInfo.nEventType = stRecordInfo.nEventType;
        }
        break;
    default:
        break;
    }
    return 0;
}

std::shared_ptr<CRecordFile> CRecordManage::get_recordHander(Record_NS::Info_S &stRecordInfo)
{
    ProcessInfo_S stProcessInfo;
    stProcessInfo.nChnId = stRecordInfo.nChnId;
    stProcessInfo.nEventType = stRecordInfo.nEventType;
    auto it = m_processInfos.find(stProcessInfo);
    if (it != m_processInfos.end())
    {
        return it->pRecordHandler;
    }
    stProcessInfo.nStreamType = stRecordInfo.nStreamType;
    /* 创建录制句柄 */
    stProcessInfo.pRecordHandler = std::make_shared<CRecordFile>(stRecordInfo.nChnId);
    m_pStreamClient->add_recordHandler(stProcessInfo.pRecordHandler.get());
    m_processInfos.insert(stProcessInfo);
    return stProcessInfo.pRecordHandler;
}
