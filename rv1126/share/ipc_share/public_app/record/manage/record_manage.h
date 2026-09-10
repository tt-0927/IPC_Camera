/**
 * @FilePath     : record_manage.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2025-06-28 10:36:11
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2025-07-08 17:27:00
 * @Description  : 录制管理
 */

#pragma once

#include <set>

#include "Singleton.h"
#include "common_define.h"
#include "replay_define.h"
#include "record_file.h"
#include "action_code.h"
#include "stream_client.h"

/*录制管理*/
class CRecordManage
{
public:
    /**
     * @brief   : 处理信息结构体，用于存储通道ID、流类型、端口及相关指针
     */
    typedef struct ProcessInfo
    {
        int nChnId = 0;      // 通道ID
        int nStreamType = 0; // 流类型
        // int nPort = 0;                                         // 端口
        int nEventType = 0;                                    // 事件类型
        std::shared_ptr<CRecordFile> pRecordHandler = nullptr; // 录像处理句柄
        // std::shared_ptr<StreamClient> pStreamClient = nullptr; // 流客户端指针
        ProcessInfo() = default; // 默认构造函数

        /**
         * @brief   : 重载小于运算符，用于比较ProcessInfo对象。
         * @param   other 另一个ProcessInfo对象
         * @return   {bool}如果当前对象小于other，则返回true
         */
        bool operator<(const ProcessInfo &other) const
        {
            if (nEventType != other.nEventType) // 比较事件类型
            {
                return nEventType < other.nEventType;
            }
            return nChnId < other.nChnId; // 比较通道ID
        }
    } ProcessInfo_S;

    CRecordManage();
    ~CRecordManage();

    /**
     * 设置记录信息
     * @param stRecordInfo 记录信息结构体引用
     * @return 操作结果
     */
    int set_record_info(Record_NS::Info_S &stRecordInfo);

    /**
     * 获取记录文件
     * @return 返回共享指针指向RecordFile的实例
     */
    std::shared_ptr<CRecordFile> get_recordHander(Record_NS::Info_S &stRecordInfo);

private:
    /**
     * @brief   : 处理信息结构体set容器
     */
    std::set<ProcessInfo_S> m_processInfos;
    /*流客户端指针*/
    std::shared_ptr<StreamClient> m_pStreamClient = nullptr;
};
