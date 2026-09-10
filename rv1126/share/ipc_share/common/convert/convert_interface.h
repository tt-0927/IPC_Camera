#pragma once

#include <string.h>
#include <ostream>
#include <fstream>
#include <sstream>
#include <string>

#include "Json.h"
#include "path_define.h"
// #include "event_define.h"
#include "alarm_convert.h"
#include "audio_convert.h"
#include "capture_convert.h"
#include "event_convert.h"
#include "log_convert.h"
#include "layout_convert.h"
// #include "ShowParamConvert.h"
#include "web_plugin_convert.h"
#include "base_convert.h"
// #include "IpcConvert.h"
#include "network_convert.h"
// #include "RegisterConvert.h"
#include "system_convert.h"
#include "common_convert.h"
#include "preview_convert.h"
#include "record_convert.h"
#include "replay_convert.h"
#include "user_convert.h"
#include "video_convert.h"
#include "osd_convert.h"
#include "isp_convert.h"
#include "convert.h"
#include "storage_manage_convert.h"
#include "dlog.h"

#include <fcntl.h>   
#include <unistd.h>  
#include <chrono>
#ifdef ENABLE_AI_STUDENT
#include "ai_student_convert.h"
#endif

class RunTimer
{
public:
    RunTimer(const std::string &jsonData)
        : m_data(jsonData)
    {
        m_startTime = std::chrono::high_resolution_clock::now();
    }
    ~RunTimer()
    {
        auto endTime = std::chrono::high_resolution_clock::now();
        /*** 打印函数执行时间,单位为微秒 */
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - m_startTime).count();

        /* 超过1秒则打印 */
        if (duration > 1 * 1000 * 1000)
        {
            dlog_warn("RunTimer: cost %d ms, data %s", (int)(duration / 1000), m_data.c_str());
        }
    }   
private:
    // 防止悬空引用，导致程序异常,更改为拷贝
    //const std::string &m_data;
    std::string m_data;
    std::chrono::high_resolution_clock::time_point m_startTime;
};

namespace Convert
{
    // 可变模板参数的递归处理函数
    template <typename T>
    void process_data(bool bStruct, Json::Object *pRootJson, T &data)
    {
        deal(pRootJson, data, bStruct);
    }

    // 递归展开的函数模板，用于处理多个参数
    template <typename T, typename... Args>
    void process_data(bool bStruct, Json::Object *pRootJson, T &data, Args &...args)
    {
        deal(pRootJson, data, bStruct);
        process_data(bStruct, pRootJson, args...); // 递归调用处理剩余参数
    }

    // 多参模板 to_string 函数
    template <typename... Args>
    inline std::string to_string(Args &...args)
    {
        Json::Object *pRootJson = Json::init();
        process_data(false, pRootJson, args...); // 处理所有传入的参数
        std::string jsonString = Json::to_string(pRootJson);
        Json::deinit(pRootJson);
        return jsonString;
    }
    template <typename... Args>
    inline void to_struct(const std::string &jsonData, Args &...args)
    {
        Json::Object *pRootJson = Json::init(jsonData);
        process_data(true, pRootJson, args...); // 处理所有传入的参数
        Json::deinit(pRootJson);
    }
    template <typename T>
    inline void to_struct(const std::string &jsonData, T &data)
    {
        Json::Object *pRootJson = Json::init(jsonData);
        deal(pRootJson, data, true);
        Json::deinit(pRootJson);
    }

    template <typename T>
    inline int write_file(const std::string &path, T &data)
    {
        RunTimer timer(path);
        std::string tmp_path = path + ".tmp";
        // 1. 写入临时文件
        {
            std::ofstream file(tmp_path, std::ios::out | std::ios::binary | std::ios::trunc);
            if (!file.is_open())
                return -1;
    
            std::string jsonData = to_string(data);
            file << jsonData << std::endl;
            file.flush();
            file.close();
    
            // 2. 强制写入磁盘
            int fd = ::open(tmp_path.c_str(), O_WRONLY);
            if (fd != -1)
            {
                ::fsync(fd);
                ::close(fd);
            }
        }
    
        // 3. 原子替换原文件
        ::rename(tmp_path.c_str(), path.c_str());

        // 4. 同步目录项，防止目录结构未更新
        std::string::size_type pos = path.rfind('/');  
        if (pos == std::string::npos)
        {
            return 0;
        }
        std::string dir = path.substr(0, pos);
        int dir_fd = ::open(dir.c_str(), O_DIRECTORY | O_RDONLY);
        if (dir_fd != -1)
        {
            ::fsync(dir_fd);
            ::close(dir_fd);
        }
        return 0;
    }

    template <typename T>
    inline int read_file(const std::string &path, T &data)
    {
        std::ifstream file(path, std::ios::binary | std::ios::ate);
        if (!file.is_open())
        {
            return -1;
        }
        std::ifstream::pos_type file_size = file.tellg();
        /* 判断是否为空文件 */
        if (file_size == 0)
        {
            return -1;
        }

        /* 复位读指针 */
        file.seekg(0, std::ios::beg);  

        std::stringstream buffer;
        buffer << file.rdbuf();

        to_struct(buffer.str(), data);
        file.close();
        return 0;
    }
}
