/**
 * @FilePath     : web_plugin_task.cpp
 * @Author       : zhangjc (zhangjc@kfb.cn)
 * @Date         : 2024-10-06 15:24:15
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2025-06-11 16:44:04
 * @Description  : PC端的插件任务
 */

#include "web_plugin_task.h"
#include "web_plugin_convert.h"
#include "convert_interface.h"
#include "path_define.h"

void Task::WebPlugin::GetParam::handle()
{
    ::WebPlugin::Param_S stParam;
    // Convert::to_struct(m_taskData, stParam);
    if (0 != Convert::read_file(WEB_PLUGIN_CONFIG_FILE, stParam))
    {
        Convert::write_file(WEB_PLUGIN_CONFIG_FILE, stParam);
    }
    std::string str = Convert::to_string(stParam);
    result(str, 0);
}

void Task::WebPlugin::SetParam::handle()
{
    ::WebPlugin::Param_S stParam;
    Convert::to_struct(m_taskData, stParam);
    result(Convert::write_file(WEB_PLUGIN_CONFIG_FILE, stParam));
}
