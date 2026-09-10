/**
 * @FilePath     : record_configure.cpp
 * @Author       : zhangjc (zhangjc@kfb.cn)
 * @Date         : 2024-12-16
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2025-06-30 20:30:55
 * @Description  : 录制配置
 */

#include "record_configure.h"
#include "path_define.h"

RecordConfigure::RecordConfigure()
    : m_advancedParam(RECORD_ADVANCED_PARAM_CONFIG_FILE)
    , m_schedule(RECORD_SCHEDULE_CONFIG_FILE)
{
}

RecordConfigure::~RecordConfigure()
{
}

int RecordConfigure::set_configure(const Record_NS::AdvancedParam_S &data)
{
    return m_advancedParam.set(data);
}

int RecordConfigure::get_configure(Record_NS::AdvancedParam_S &data) const
{
    return m_advancedParam.get(data);
}

int RecordConfigure::set_configure(const Record_NS::Schedule_S &data)
{
    return m_schedule.set(data);
}

int RecordConfigure::get_configure(Record_NS::Schedule_S &data) const
{
    return m_schedule.get(data);
}
