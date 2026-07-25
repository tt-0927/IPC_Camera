#pragma once

#include <chrono>
#include <fcntl.h>
#include <fstream>
#include <ostream>
#include <sstream>
#include <string.h>
#include <unistd.h>

#include "Convert.h"
#include "dlog.h"
#include "JsonInterfase.h"

class RunTimer
{
public:

    RunTimer(const std::string& jsonData)
        : m_data(jsonData)
    {
        m_startTime = std::chrono::high_resolution_clock::now();
    }

    ~RunTimer()
    {
        auto endTime  = std::chrono::high_resolution_clock::now();
        /*** 打印函数执行时间,单位为微秒 */
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - m_startTime).count();

        /* 超过1秒则打印 */
        if (duration > 1 * 1000 * 1000)
        {
            dlog(LOG_WARN, "解析时间: cost %ld ms, data %s", duration / 1000, m_data.c_str());
        }
    }

private:

    const std::string&                             m_data;
    std::chrono::high_resolution_clock::time_point m_startTime;
};

namespace Ai0630_NS
{
    // 可变模板参数的递归处理函数
    template<typename T>
    void process_data(bool bStruct, Json::Object* pRootJson, T& data)
    {
        dealJson(pRootJson, data, bStruct);
    }

    // 递归展开的函数模板，用于处理多个参数
    template<typename T, typename... Args>
    void process_data(bool bStruct, Json::Object* pRootJson, T& data, Args&... args)
    {
        dealJson(pRootJson, data, bStruct);
        process_data(bStruct, pRootJson, args...);    // 递归调用处理剩余参数
    }

    // 多参模板 to_string 函数
    template<typename... Args>
    inline std::string to_string(Args&... args)
    {
        RunTimer      timer();
        Json::Object* pRootJson = Json::init();
        process_data(false, pRootJson, args...);    // 处理所有传入的参数
        std::string jsonString = Json::to_string(pRootJson);
        Json::deinit(pRootJson);
        return jsonString;
    }

    template<typename... Args>
    inline void to_struct(const std::string& jsonData, Args&... args)
    {
        RunTimer      timer(jsonData);
        Json::Object* pRootJson = Json::init(jsonData);
        process_data(true, pRootJson, args...);    // 处理所有传入的参数
        Json::deinit(pRootJson);
    }

    template<typename T>
    inline void to_struct(const std::string& jsonData, T& data)
    {
        RunTimer      timer(jsonData);
        Json::Object* pRootJson = Json::init(jsonData);
        dealJson(pRootJson, data, true);
        Json::deinit(pRootJson);
    }

    template<typename T>
    inline int write_file(const std::string& path, T& data)
    {
        RunTimer    timer(path);
        std::string tmp_path = path + ".tmp";
        // 1. 写入临时文件
        {
            std::ofstream file(tmp_path, std::ios::out | std::ios::binary | std::ios::trunc);
            if (!file.is_open())
            {
                return -1;
            }

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
        std::string dir    = path.substr(0, pos);
        int         dir_fd = ::open(dir.c_str(), O_DIRECTORY | O_RDONLY);
        if (dir_fd != -1)
        {
            ::fsync(dir_fd);
            ::close(dir_fd);
        }
        return 0;
    }

    template<typename T>
    inline int read_file(const std::string& path, T& data)
    {
        RunTimer      timer(path);
        std::ifstream file(path);
        if (!file.is_open())
        {
            return -1;
        }
        std::stringstream buffer;
        buffer << file.rdbuf();

        to_struct(buffer.str(), data);
        file.close();
        return 0;
    }
}    // namespace Ai0630_NS
