/**
 * @FilePath     : command_line_parser.hpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2025-06-06 16:02:10
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2025-06-09 10:05:13
 * @Description  : 命令行解析器
 */

#pragma once

#include <iostream>
#include <optional>
#include <string>
#include <unordered_map>

class CCommandLineParser
{
public:

    void parse(int argc, char const* argv[])
    {
        for (int i = 1; i < argc; ++i)
        {
            std::string arg = argv[i];

            if (arg.size() > 1 && arg[0] == '-')
            {
                /* 去除横线前缀 */
                std::string key = arg.substr(1);
                if (i + 1 < argc && argv[i + 1][0] != '-')
                {
                    std::string value = argv[i + 1];
                    i++;
                    m_params[key] = value;
                }
                else
                {
                    // 处理参数后面没有值的情况
                    m_params[key] = "";
                }
            }
        }
    }

    bool getStringParam(const std::string& key, std::string& strValue) const
    {
        if (m_params.count(key) > 0)
        {
            strValue = m_params.at(key);
            return true;
        }
        return false;
    }

    bool getIntParam(const std::string& key, int& nValue) const
    {
        if (m_params.count(key) > 0)
        {
            try
            {
                nValue = std::stoi(m_params.at(key));
                return true;
            }
            catch (...)
            {
                // 处理无法转换为整数的情况
            }
        }
        return false;
    }

    bool isParamSet(const std::string& key) const
    {
        return m_params.count(key) > 0;
    }

    bool isParamWithValue(const std::string& key) const
    {
        if (m_params.count(key) > 0)
        {
            return !m_params.at(key).empty();
        }
        return false;
    }

private:

    std::unordered_map<std::string, std::string> m_params;
};
