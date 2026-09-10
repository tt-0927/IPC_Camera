/**
 * @FilePath     : config_module.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2025-09-30 10:17:07
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2025-10-13 15:13:26
 * @Description  : 通用配置管理器 (模板)
 */

#pragma once

#include <string>
#include <memory>
#include "config_strategy.h"

#include "convert_interface.h"
#include "dlog.h"

/**
* @brief   : 通用配置管理器 (模板)
* @note    : 负责配置的读取、初始化和写入。
*          通过注入的策略对象来实现具体的配置逻辑。
* @tparam  {T} : 配置结构体的类型
*/
template <typename T>
class CConfigModule
{
public:
    /**
    * @brief   : 构造函数
    * @param   {const std::string&} strFilePath : 配置文件路径
    * @param   {std::unique_ptr<CConfigStrategy<T>>} pStrategy : 具体的配置策略实例
    */
    CConfigModule(const std::string &strFilePath, std::unique_ptr<CConfigStrategy<T>> pStrategy) :
        m_strFilePath(strFilePath),
        m_pStrategy(std::move(pStrategy))
    {
        if (!m_pStrategy)
        {
            // 在实际项目中，这里应该记录一个致命错误或抛出异常
            dlog_error("配置策略不能为空!");
        }
    }

    /**
    * @brief   : 初始化配置
    * @note    : 模块启动时调用此函数。它会尝试读取配置文件，
    *          如果失败，则使用策略生成默认配置并回写。
    * @return  {int} 0：成功，非0：失败
    */
    int init()
    {
        if (Convert::read_file(m_strFilePath, m_stConfig) != 0)
        {
            dlog_info("未找到或解析配置文件 [%s] 失败, 生成默认配置。", m_strFilePath.c_str());

            if (!m_pStrategy) return -1;

            /* 1. 使用策略填充默认值 */
            m_pStrategy->fillDefault(m_stConfig);

            /* 2. 使用策略进行业务定制 */
            m_pStrategy->customize(m_stConfig);

            /* 3. 将生成的新配置写入文件 */
            if (Convert::write_file(m_strFilePath, m_stConfig) != 0)
            {
                dlog_error("写入默认配置文件 [%s] 失败!", m_strFilePath.c_str());
                return -2;
            }
            dlog_info("默认配置已生成并写入 [%s]", m_strFilePath.c_str());
        }
        else
        {
            dlog_info("成功加载配置文件 [%s]", m_strFilePath.c_str());
        }
        return 0;
    }

    /**
    * @brief   : 获取当前配置
    * @note    : 获取的是常量引用，防止外部意外修改。
    * @return  {const T&} 配置结构体的常量引用
    */
    const T& getConfig() const
    {
        return m_stConfig;
    }

private:
    /* 配置文件路径 */
    std::string m_strFilePath;
    /* 配置结构体实例 */
    T m_stConfig;
    /* 指向配置策略的智能指针 */
    std::unique_ptr<CConfigStrategy<T>> m_pStrategy;
};
