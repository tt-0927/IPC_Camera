/**
 * @FilePath     : config_storage.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2024-12-14 11:12:16
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-04-16 15:38:01
 * @Description  : 事件存储
 */

#pragma once

#include <set>
#include "convert_interface.h"

/*存储类型枚举*/
typedef enum class StorageType
{
    Single,    // 单个对象
    Collection // 集合
} StorageType_E;

/*主模板声明*/
template <typename T, StorageType_E Type = StorageType_E::Collection>
class ConfigStorage;

/*集合存储特化*/
template <typename T>
class ConfigStorage<T, StorageType_E::Collection>
{
public:
    ConfigStorage(const std::string &filePath) : m_filePath(filePath)
    {
        Convert::read_file(m_filePath, m_data);
    }

    ~ConfigStorage()
    {
        Convert::write_file(m_filePath, m_data);
    }

    int del(const T &config)
    {
        m_data.erase(config);
        Convert::write_file(m_filePath, m_data);
        return 0;
    }
    int set(const T &config)
    {
        m_data.erase(config);
        m_data.insert(config);
        Convert::write_file(m_filePath, m_data);
        return 0;
    }

    int get(T &config) const
    {
        auto it = m_data.find(config);
        if (it != m_data.end())
        {
            config = *it;
            return 0;
        }
        return -1;
    }

    int get(std::set<T> &config) const
    {
        config = m_data;
        return 0;
    }

    int clear()
    {
        m_data.clear();
        Convert::write_file(m_filePath, m_data);
        return 0;
    }

private:
    std::set<T> m_data;
    std::string m_filePath;
};

/*单个对象存储特化*/
template <typename T>
class ConfigStorage<T, StorageType_E::Single>
{
public:
    ConfigStorage(const std::string &filePath) : m_filePath(filePath)
    {
        if (Convert::read_file(m_filePath, m_data) != 0)
        {
            /* 文件不存在或读取失败时，初始化默认值 */
            initializeDefault();
            Convert::write_file(m_filePath, m_data);
        }
    }

    ~ConfigStorage()
    {
        Convert::write_file(m_filePath, m_data);
    }

    int set(const T &config)
    {
        m_data = config;
        Convert::write_file(m_filePath, m_data);
        return 0;
    }

    int get(T &config) const
    {
        config = m_data;
        return 0;
    }

    /*获取配置的引用*/
    const T &getData() const
    {
        return m_data;
    }
    T &getData()
    {
        return m_data;
    }

    // /*删除操作对单个对象意义不大，但为了接口一致性保留 可以重置为默认值*/
    // int del(const T &config = T{})
    // {
    //     m_data = config;  // 重置为传入值或默认值
    //     Convert::write_file(m_filePath, m_data);
    //     return 0;
    // }

private:
    T m_data;
    std::string m_filePath;

    /* 使用 std::void_t 进行 SFINAE 检测 (C++14 compatible) */
    template <typename, typename = std::void_t<>>
    struct has_create_default_rule : std::false_type
    {
    };

    template <typename U>
    struct has_create_default_rule<U, std::void_t<decltype(U::CreateWithDefaultRule())>> : std::true_type
    {
    };

    void initializeDefault()
    {
        initializeDefaultImpl(has_create_default_rule<T>{});
    }

    void initializeDefaultImpl(std::true_type)
    {
        /* 有 CreateWithDefaultRule 方法 */
        m_data = T::CreateWithDefaultRule();
    }

    void initializeDefaultImpl(std::false_type)
    {
        /*  没有 CreateWithDefaultRule 方法，使用默认构造 */
        m_data = T{};
    }
};