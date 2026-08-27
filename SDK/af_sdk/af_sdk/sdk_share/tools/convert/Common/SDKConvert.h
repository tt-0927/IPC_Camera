/**
 * @file SDKConvert.h
 * @author tianl (tianl@kfb.cn)
 * @date 2026-07-28
 * @LastEditors  : qinjt@kfb.cn
 * @LastEditTime : 2026-07-28
 *
 * @brief SDKConvert 模块接口与类型定义
 * 功能说明：
 * 1. 声明 SDKConvert 模块对外接口和数据类型
 * 2. 定义模块依赖的常量、回调或辅助类型
 * 3. 为调用方提供明确且稳定的编译期契约
 */
#pragma once
#include <list>
#include <chrono>
#include <set>
#include <algorithm>
#include <cstring>
#include <type_traits>
#include "Json.h"
#include "NetTVSDKHttpUrl.h"

/* 库通用头文件 */
#ifdef NET_SDK_SERVER_API
    #include "NetTVSDKServerInterface.h"
#elif defined(NET_SDK_CLIENT_API)
    #include "NetTVSDKClientInterface.h"
#else
    #include "NetTVSDKCommon.h"
#endif

/* Convert 头文件由各调用方自行 include，确保在 SDKConvert.h 之前。
 * 原因：process_data/to_respString 模板内部调用 deal()，
 * deal 重载必须在模板定义点之前可见。
 * SDKConvert.h 本身零依赖，不 include 任何 Convert 头。 */

//关键字段定义
#define NETSDK_JSON_MESSAGE_KEY            "message"           /* 响应消息 */
#define NETSDK_JSON_ACTIONCODE_KEY         "actioncode"        /* 命令码 */
#define NETSDK_JSON_DEVICE_NAME_KEY        "device_name"       /* 设备名称 */
#define NETSDK_JSON_INNER_DATA_KEY         "data"              /* 具体业务数据 */
#define NETSDK_JSON_RETURN_KEY             "return"            /* 返回码 */
#define NETSDK_JSON_CHANNEL_KEY            "channel"           /* 通道号（-1表示NVR本机） */

// 全局设备名称变量，在NetTVSDKServerImpl.cpp中定义
extern std::string g_sdkDeviceName;

class CRunTimer
{
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 CRunTimer 定义的内联处理。
 * @param [in] jsonData 函数处理参数。
 * @return 返回该处理的状态或结果。
 */
public:
    CRunTimer(const std::string &jsonData)
        : m_strData(jsonData)
    {
        m_stStartTime = std::chrono::high_resolution_clock::now();
    }
    ~CRunTimer()
    {
        auto endTime = std::chrono::high_resolution_clock::now();
        /*** 打印函数执行时间,单位为微秒 */
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - m_stStartTime).count();

        /* 超过1秒则打印 */
        if (duration > 1 * 1000 * 1000)
        {
            printf("CRunTimer: cost %lld ms, data %s", duration / 1000, m_strData.c_str());
        }
    }
private:
    const std::string &m_strData;
    std::chrono::high_resolution_clock::time_point m_stStartTime;
};


namespace SDKConvert
{

	class CSDKConvert
    {
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 CSDKConvert 定义的内联处理。
 * @param [in] bOutStruct 函数处理参数。
 * @return 返回该处理的状态或结果。
 */
    public:
        CSDKConvert(bool bOutStruct)
            : m_bOutValue(bOutStruct),
              m_bOutStruct(bOutStruct)
        {
        }
        /**
 * @author tianl (tianl@kfb.cn)
         * @brief 字段转换
         * @tparam T 模板类型，支持类型见Json.h中get、add的value
         * @param pRootJson Json句柄
         * @param key 键
         * @param value 值
         */
        template <typename T, typename std::enable_if<!std::is_enum<T>::value, int>::type = 0>
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 field 定义的内联处理。
 * @param [in,out] pRootJson 函数处理参数。
 * @param [in] key 函数处理参数。
 * @param [in,out] value 函数处理参数。
 * @return 无返回值。
 */
        void field(Json::Object *pRootJson, std::string key, T &value)
        {
            if (m_bOutValue)
            {
                Json::get(pRootJson, key, value);
            }
            else
            {
                Json::add(pRootJson, key, value);
            }
        }

        /**
         * @brief 枚举类型字段转换（SFINAE 重载）
         * @details Json::get/add 仅支持 int 等基础类型，C 风格枚举需经 int 中转，
         *          否则枚举→int 产生右值无法绑定 Json::get 的 int& 参数。
         *          仅当 T 为枚举类型时启用，不影响其它类型的 field 重载。
         * @tparam T 枚举类型（std::is_enum 约束）
         * @param pRootJson Json 句柄
         * @param key 键
         * @param value 枚举字段引用
         */
        template <typename T, typename std::enable_if<std::is_enum<T>::value, int>::type = 0>
        void field(Json::Object *pRootJson, std::string key, T &value)
        {
            int nVal = static_cast<int>(value);
            if (m_bOutValue)
            {
                Json::get(pRootJson, key, nVal);
                value = static_cast<T>(nVal);
            }
            else
            {
                Json::add(pRootJson, key, nVal);
            }
        }

        template <size_t N>
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 field 定义的内联处理。
 * @param [in,out] pRootJson 函数处理参数。
 * @param [in] key 函数处理参数。
 * @param [in,out] parameter 函数处理参数。
 * @return 无返回值。
 */
        void field(Json::Object *pRootJson, std::string key, char (&value)[N])
        {
            if (m_bOutValue)
            {
                std::string json_str;
                field(pRootJson, key, json_str);
                size_t copy_len = (std::min)(json_str.size(), static_cast<size_t>(N - 1));
                std::memcpy(value, json_str.c_str(), copy_len);
                value[copy_len] = '\0';
            }
            else
            {
                std::string str_value(value);
                Json::add(pRootJson, key, str_value);
            }
        }

        /**
 * @author tianl (tianl@kfb.cn)
         * @brief 数组转换转换
         * @tparam T 模板类型，支持类型见Json.h中get、add的value
         * @param pRootJson Json句柄
         * @param key 键
         * @param value 值
         */
        template <typename T>
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 field 定义的内联处理。
 * @param [in,out] pRootJson 函数处理参数。
 * @param [in] key 函数处理参数。
 * @param [in,out] vec 函数处理参数。
 * @return 无返回值。
 */
        void field(Json::Object *pRootJson, const std::string key, std::vector<T> &vec)
        {
            /* 调用已实现的结构体转换 */
            if (m_bOutStruct)
            {
                Json::Object *items = Json::get(pRootJson, key);
                Json::Array::get(items, vec);
            }
            else
            {
                Json::Object *items = Json::Array::init();
                for (size_t i = 0; i < vec.size(); i++)
                {
                    Json::Object *item = Json::init();
                    if (!item)
                    {
                        continue;
                    }
                    T &data = vec[i];
                    Json::Array::add(items, data);
                }
                Json::add(pRootJson, key.c_str(), items);
            }
        }

        /**
 * @author tianl (tianl@kfb.cn)
         * @brief 数组转换转换
         * @tparam T 模板类型，支持类型见Json.h中get、add的value
         * @param pRootJson Json句柄
         * @param key 键
         * @param value 值
         */
        template <typename T>
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 field 定义的内联处理。
 * @param [in,out] pRootJson 函数处理参数。
 * @param [in] key 函数处理参数。
 * @param [in,out] setData 函数处理参数。
 * @return 无返回值。
 */
        void field(Json::Object *pRootJson, const std::string key, std::set<T> &setData)
        {
            /* 调用已实现的结构体转换 */
            if (m_bOutStruct)
            {
                std::vector<T> vecData;
                field(pRootJson, key, vecData);
                setData = std::set<T>(vecData.begin(), vecData.end());
            }
            else
            {
                std::vector<T> vecData(setData.begin(), setData.end());
                field(pRootJson, key, vecData);
            }
        }



        /**
 * @author tianl (tianl@kfb.cn)
         * @brief 数组字段转换 (custom for fixed size array)
         */
        template <typename T>
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 field_array 定义的内联处理。
 * @param [in,out] pRootJson 函数处理参数。
 * @param [in] key 函数处理参数。
 * @param [in,out] value 函数处理参数。
 * @param [in] count 函数处理参数。
 * @param [in] max_len 函数处理参数。
 * @return 无返回值。
 */
        void field_array(Json::Object *pRootJson, std::string key, T *value, UINT32 count, UINT32 max_len)
        {
            if (m_bOutValue) /* Json -> Array */
            {
                Json::Object *pArray = Json::get(pRootJson, key);
                int nSize = Json::Array::size(pArray);
                for (int i = 0; i < nSize && (UINT32)i < max_len; i++)
                {
                    int nVal = 0;
                    Json::Object *pItem = Json::Array::get(pArray, i);
                    if (pItem)
                    {
                        Json::Value::get(pItem, nVal);
                        value[i] = (T)nVal;
                    }
                }
            }
            else /* Array -> Json */
            {
                Json::Object *pArray = Json::Array::init();
                for (UINT32 i = 0; i < count && i < max_len; i++)
                {
                    Json::Array::add(pArray, (int)value[i]);
                }
                Json::add(pRootJson, key, pArray);
            }
        }

        /**
 * @author tianl (tianl@kfb.cn)
         * @brief 结构体转换
         * @tparam T 模板类型，支持类型基于已实现的deal
         * @param pRootJson Json句柄
         * @param stStruct 结构体
         */
        template <typename T>
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 structure 定义的内联处理。
 * @param [in,out] pRootJson 函数处理参数。
 * @param [in,out] stStruct 函数处理参数。
 * @return 无返回值。
 */
        void structure(Json::Object *pRootJson, T &stStruct)
        {
            /* 调用已实现的结构体转换 */
            deal(pRootJson, stStruct, m_bOutStruct);
        }

        /**
 * @author tianl (tianl@kfb.cn)
         * @brief 结构体转换
         * @tparam T 模板类型，支持类型基于已实现的deal
         * @param pRootJson Json句柄
         * @param key 键
         * @param stStruct 结构体
         */
        template <typename T>
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 structure 定义的内联处理。
 * @param [in,out] pRootJson 函数处理参数。
 * @param [in] key 函数处理参数。
 * @param [in,out] stStruct 函数处理参数。
 * @return 无返回值。
 */
        void structure(Json::Object *pRootJson, const std::string key, T &stStruct)
        {
            /* 调用已实现的结构体转换 */
            Json::Object *pJsonData = nullptr;
            if (m_bOutStruct)
            {
                /* json -> struct */
                pJsonData = Json::get(pRootJson, key);
                deal(pJsonData, stStruct, m_bOutStruct);
            }
            else
            {
                /* json <- struct */
                pJsonData = Json::init();
                deal(pJsonData, stStruct, m_bOutStruct);
                Json::add(pRootJson, key, pJsonData);
            }
        }

        /**
 * @author tianl (tianl@kfb.cn)
         * @brief 结构体数组转换
         * @tparam T 模板类型，支持类型基于已实现的deal
         * @param pRootJson Json句柄
         * @param key 键
         * @param vec 结构体数组
         */
        template <typename T>
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 structure 定义的内联处理。
 * @param [in,out] pRootJson 函数处理参数。
 * @param [in] key 函数处理参数。
 * @param [in,out] vec 函数处理参数。
 * @return 无返回值。
 */
        void structure(Json::Object *pRootJson, const std::string key, std::vector<T> &vec)
        {
            /* 调用已实现的结构体转换 */
            if (m_bOutStruct)
            {
                Json::Object *pArray = Json::get(pRootJson, key);
                vec.clear();
                if (!pArray)
                {
                    return;
                }

                int nSize = Json::Array::size(pArray);
                if (nSize <= 0)
                {
                    return;
                }
                vec.resize(static_cast<size_t>(nSize));
                for (int i = 0; i < nSize; i++)
                {
                    Json::Object *item = Json::Array::get(pArray, i);
                    if (!item)
                    {
                        continue;
                    }
                    T info;
                    structure(item, info);

                    vec[static_cast<size_t>(i)] = info;
                }
            }
            else
            {
                Json::Object *items = Json::Array::init();

                for (size_t i = 0; i < vec.size(); i++)
                {
                    Json::Object *item = Json::init();
                    if (!item)
                    {
                        continue;
                    }
                    T &info = vec[i];
                    structure(item, info);
                    Json::Array::add(items, item);
                }
                Json::add(pRootJson, key.c_str(), items);
            }
        }

        /**
 * @author tianl (tianl@kfb.cn)
         * @brief 结构体链表转换
         * @tparam T 模板类型，支持类型基于已实现的deal
         * @param pRootJson Json句柄
         * @param key 键
         * @param vec 结构体数组
         */
        template <typename T>
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 structure 定义的内联处理。
 * @param [in,out] pRootJson 函数处理参数。
 * @param [in] key 函数处理参数。
 * @param [in,out] list 函数处理参数。
 * @return 无返回值。
 */
        void structure(Json::Object *pRootJson, const std::string key, std::list<T> &list)
        {
            /* 调用已实现的结构体转换 */
            if (m_bOutStruct)
            {
                Json::Object *pArray = Json::get(pRootJson, key);
                list.clear();
                if (!pArray)
                {
                    return;
                }

                int nSize = Json::Array::size(pArray);
                if (nSize <= 0)
                {
                    return;
                }
                for (int i = 0; i < nSize; i++)
                {
                    Json::Object *item = Json::Array::get(pArray, i);
                    if (!item)
                    {
                        continue;
                    }
                    T info;
                    structure(item, info);

                    list.push_back(info);
                }
            }
            else
            {
                Json::Object *items = Json::Array::init();

                for (auto &data :list)
                {
                    Json::Object *item = Json::init();
                    if (!item)
                    {
                        continue;
                    }
                    structure(item, data);
                    Json::Array::add(items, item);
                }
                Json::add(pRootJson, key.c_str(), items);
            }
        }

        /**
 * @author tianl (tianl@kfb.cn)
         * @brief 结构体set容器转换
         * @tparam T 模板类型，支持类型基于已实现的deal
         * @param pRootJson Json句柄
         * @param key 键
         * @param vec 结构体数组
         */
        template <typename T>
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 structure 定义的内联处理。
 * @param [in,out] pRootJson 函数处理参数。
 * @param [in] key 函数处理参数。
 * @param [in,out] set 函数处理参数。
 * @return 无返回值。
 */
        void structure(Json::Object *pRootJson, const std::string key, std::set<T> &set)
        {
            /* 调用已实现的结构体转换 */
            if (m_bOutStruct)
            {
                Json::Object *pArray = Json::get(pRootJson, key);
                int nSize = Json::Array::size(pArray);
                for (int i = 0; i < nSize; i++)
                {
                    Json::Object *item = Json::Array::get(pArray, i);
                    if (!item)
                    {
                        continue;
                    }
                    T info;
                    structure(item, info);

                    set.insert(info);
                }
            }
            else
            {
                Json::Object *items = Json::Array::init();

                for (auto &data : set)
                {
                    Json::Object *item = Json::init();
                    if (!item)
                    {
                        continue;
                    }
                    T t= data;
                    structure(item, t);
                    Json::Array::add(items, item);
                }
                Json::add(pRootJson, key.c_str(), items);
            }
        }
    private:
        bool m_bOutValue = false;
        bool m_bOutStruct = false;
    };
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 deal 定义的内联处理。
 * @param [in,out] pRootJson 函数处理参数。
 * @param [in,out] stInfo 函数处理参数。
 * @param [in] bOutStruct 函数处理参数。
 * @return 无返回值。
 */

     inline void deal(Json::Object* pRootJson, SessionMessage_S& stInfo, bool bOutStruct)
    {
        if (!pRootJson)
        {
            return;
        }
        CSDKConvert convert(bOutStruct);
        convert.field(pRootJson, "SessionId", stInfo.SessionId);
    }


    /* 可变模板参数的递归处理函数 */
    template <typename T>
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 process_data 定义的内联处理。
 * @param [in] bStruct 函数处理参数。
 * @param [in,out] pRootJson 函数处理参数。
 * @param [in,out] data 函数处理参数。
 * @return 无返回值。
 */
    void process_data(bool bStruct, Json::Object *pRootJson, T &data)
    {
        deal(pRootJson, data, bStruct);
    }


    /* 空参数包的终止版本 */
    inline void process_data(bool /*bStruct*/, Json::Object * /*pRootJson*/)
    {
        /* 空参数包，什么也不做 */
    }

    /* 递归展开的函数模板，用于处理多个参数 */
    template <typename T, typename... Args>
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 process_data 定义的内联处理。
 * @param [in] bStruct 函数处理参数。
 * @param [in,out] pRootJson 函数处理参数。
 * @param [in,out] data 函数处理参数。
 * @param [in,out] args 函数处理参数。
 * @return 无返回值。
 */
    void process_data(bool bStruct, Json::Object *pRootJson, T &data, Args &... args)
    {
        deal(pRootJson, data, bStruct);
        process_data(bStruct, pRootJson, args...); /* 递归调用处理剩余参数 */
    }


    /* 多参模板 to_string 函数 */
    template <typename... Args>
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 to_string 定义的内联处理。
 * @param [in,out] args 函数处理参数。
 * @return 返回该处理的状态或结果。
 */
    inline std::string to_string(Args &... args)
    {
        CRunTimer timer();
        Json::Object *pRootJson = Json::init();
        process_data(false, pRootJson, args...);  /* 处理所有传入的参数 */
        std::string jsonString = Json::to_string(pRootJson);
        Json::deinit(pRootJson);
        return jsonString;
    }

    template <typename... Args>
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 from_string 定义的内联处理。
 * @param [in] jsonData 函数处理参数。
 * @param [in,out] args 函数处理参数。
 * @return 返回该处理的状态或结果。
 */
    inline bool from_string(const std::string& jsonData, Args &... args)
    {
        CRunTimer timer(jsonData);
        Json::Object *pRootJson = Json::init(jsonData);
        if (!pRootJson)
        {
            return false;
        }

        process_data(true, pRootJson, args...);
        Json::deinit(pRootJson);
        return true;
    }

    /**
     * @brief 根据错误码获取对应的中文描述
     * @param nCode 错误码，参见 NET_COMMON_ECODE_E
     * @return 错误码对应的描述字符串
     */
    inline std::string get_errMessage(int nCode)
    {
        switch (nCode)
        {
        case NET_E_FAILED:                     return "Failed";
        case NET_E_SUCCEED:                    return "Succeeded";
        case NET_E_SVC_FAILED:                 return "Server failed";
        case NET_E_NOT_AUTHORIZED:             return "User not authorized";
        case NET_E_NO_USER:                    return "User does not exist";
        case NET_E_SDK_NOT_INIT:               return "SDK not initialized";
        case NET_E_NO_RESULT:                  return "No results found";
        case NET_E_NOENOUGH_BUF:               return "Buffer too small";
        case NET_E_SDK_SOCKET_LSN_FAIL:        return "Failed to create socket listener";
        case NET_E_INIT_MUTEX_FAIL:            return "Failed to initialize mutex";
        case NET_E_INIT_SEMA_FAIL:             return "Failed to initialize semaphore";
        case NET_E_ALLOC_RESOURCE_ERROR:       return "SDK resource allocation error";
        case NET_E_HAVEDATA:                   return "Not all data sent";
        case NET_E_NEEDMOREDATA:               return "More data needed";
        case NET_E_TRANSFILE_FAIL:             return "File transfer failed";
        case NET_E_DEVICE_TYPE_ERR:            return "Unsupported device type";
        case NET_E_NONCE_TIMEOUT:              return "Nonce expired";
        case NET_E_INNER_ERR:                  return "Internal system error";
        case NET_E_BINDNOTIFY_FAIL:            return "Failed to bind alarm notification";
        case NET_E_SYSCALL_FALIED:             return "System call failed";
        case NET_E_NULL_POINT:                 return "Null pointer";
        case NET_E_INVALID_PARAM:              return "Invalid parameter";
        case NET_E_INVALID_MODULEID:           return "Invalid module ID";
        case NET_E_INVALID_HANDLE:             return "Invalid handle";
        case NET_E_NO_MEMORY:                  return "Memory allocation failed";
        case NET_E_FILE_NO_EXIST:              return "File does not exist";
        case NET_E_NO_DEV:                     return "Device does not exist";
        case NET_E_NO_FIT_LOG:                 return "No matching log found";
        case NET_E_BUSY:                       return "Device busy";
        case NET_E_TIMER_REG_FAILED:           return "Failed to register timer";
        case NET_E_COMMON_FAILED:              return "General error";
        case NET_E_CMD_NOT_SUPPORT:            return "Command not supported";
        case NET_E_NOT_SUPPORT:                return "Feature not supported by device";
        case NET_E_TIMEOUT:                    return "Timeout";
        case NET_E_MSG_ERR:                    return "Message mismatch";
        case NET_E_MODULE_INEXIST:             return "Module does not exist";
        case NET_E_SOCKET_RECV_ERR:            return "Failed to receive message";
        case NET_E_DECODE_IE_FAILED:           return "Failed to get message IE";
        case NET_E_ENCODE_IE_FAILED:           return "Failed to add message IE";
        case NET_E_SDK_NOINTE_ERROR:           return "SDK not initialized";
        case NET_E_ALREDY_INIT_ERROR:          return "SDK already initialized";
        case NET_E_DEVICE_FACTURER_ERR:        return "Unsupported device manufacturer";
        case NET_E_NAME_EXIST:                 return "Name already exists";
        case NET_E_GET_CFG_FAILED:             return "Failed to get configuration";
        case NET_E_SET_CFG_FAILED:             return "Failed to set configuration";
        case NET_E_CHANNEL_OVER_SPEC:          return "Channel count exceeds specification";
        case NET_E_CALL_DRV_COMMON:            return "Driver call failed";
        case NET_E_TOTAL_QUOTA_FULL:           return "Allocable quota space insufficient";
        case NET_E_CALL_DB_COMMON:             return "Database call failed";
        case NET_E_NEED_MORE_MEMORY:           return "Insufficient memory allocation";
        case NET_E_T2U_CONNECT_FAILED:         return "T2U connection failed";
        case NET_E_FUNC_IS_INITIALIZING:       return "Feature is initializing";
        case NET_E_CONNECT_ERROR:              return "Failed to create connection";
        case NET_E_SEND_MSG_ERROR:             return "Failed to send message";
        case NET_E_DECODE_RSP_ERROR:           return "Failed to parse response message";
        case NET_E_NONSUPPORT:                 return "Feature not implemented";
        case NET_E_JSON_ERROR:                 return "JSON general error";
        case NET_E_NORESULT:                   return "Query result is empty";
        case NET_E_SOCKET_RECV_ERROR:          return "Socket receive message failed";
        case NET_E_CREATE_THREAD_FAIL:         return "Failed to create thread";
        case NET_E_RESCODE_NO_EXIST:           return "Resource code does not exist";
        case NET_E_MSG_DATA_INVALID:           return "Message content error";
        case NET_E_JSON_NO_IMAGE:              return "Image data is empty";
        case NET_E_IMAGE_SIZE_BEYOND_THE_LIMIT:return "Image size exceeds limit";
        default:                                  return "Unknown error code";
        }
    }

    template <typename... Args>
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 to_respString 定义的内联处理。
 * @param [in] nRespCode 函数处理参数。
 * @param [in,out] args 函数处理参数。
 * @return 返回该处理的状态或结果。
 */
    inline std::string to_respString(int nRespCode, int nActionCode, Args &... args)
    {
        Json::Object *pRootJson = Json::init();
        Json::add(pRootJson, NETSDK_JSON_DEVICE_NAME_KEY, g_sdkDeviceName);
        if (nActionCode != 0)
        {
            Json::add(pRootJson, NETSDK_JSON_ACTIONCODE_KEY, nActionCode);
        }

        Json::Object *pInnerData = Json::init();
        if (nRespCode == 0)
        {
            process_data(false, pInnerData, args...);
        }
        Json::add(pRootJson, NETSDK_JSON_INNER_DATA_KEY, pInnerData);

        Json::add(pRootJson, NETSDK_JSON_RETURN_KEY, nRespCode);
        Json::add(pRootJson, NETSDK_JSON_MESSAGE_KEY, get_errMessage(nRespCode));

        std::string data = Json::to_string(pRootJson);
        Json::deinit(pRootJson);
        return data;
    }

    /**
     * @brief 带通道号的响应JSON生成重载
     * @details 在顶层响应中追加Channel字段，便于调用方识别响应数据归属通道；
     *          nChannel == NET_API_PARAM_NVRCHN(-1) 表示NVR本机参数。
     * @param nRespCode 返回码
     * @param nActionCode 命令码
     * @param nChannel 通道号
     * @param args 业务数据结构体
     * @return JSON格式响应字符串
     */
    template <typename... Args>
    inline std::string to_respString(int nRespCode, int nActionCode, int nChannel, Args &... args)
    {
        Json::Object *pRootJson = Json::init();
        Json::add(pRootJson, NETSDK_JSON_DEVICE_NAME_KEY, g_sdkDeviceName);
        if (nActionCode != 0)
        {
            Json::add(pRootJson, NETSDK_JSON_ACTIONCODE_KEY, nActionCode);
        }
        if (nChannel >= 0)
        {
            Json::add(pRootJson, NETSDK_JSON_CHANNEL_KEY, nChannel);
        }

        Json::Object *pInnerData = Json::init();
        if (nRespCode == 0)
        {
            process_data(false, pInnerData, args...);
        }
        Json::add(pRootJson, NETSDK_JSON_INNER_DATA_KEY, pInnerData);

        Json::add(pRootJson, NETSDK_JSON_RETURN_KEY, nRespCode);
        Json::add(pRootJson, NETSDK_JSON_MESSAGE_KEY, get_errMessage(nRespCode));

        std::string data = Json::to_string(pRootJson);
        Json::deinit(pRootJson);
        return data;
    }

/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 to_respString 定义的内联处理。
 * @param [in] enCode 函数处理参数。
 * @return 返回该处理的状态或结果。
 */
    inline std::string to_respString(NET_COMMON_ECODE_E enCode, int nActionCode = 0)
    {
        int nCode = (int)enCode;
        Json::Object *pRootJson = Json::init();
        Json::add(pRootJson, NETSDK_JSON_DEVICE_NAME_KEY, g_sdkDeviceName);
        if (nActionCode != 0)
        {
            Json::add(pRootJson, NETSDK_JSON_ACTIONCODE_KEY, nActionCode);
        }
        Json::add(pRootJson, NETSDK_JSON_INNER_DATA_KEY, Json::init());
        Json::add(pRootJson, NETSDK_JSON_RETURN_KEY, nCode);
        Json::add(pRootJson, NETSDK_JSON_MESSAGE_KEY, get_errMessage(nCode));

        std::string data = Json::to_string(pRootJson);
        Json::deinit(pRootJson);
        return data;
    }

     template <typename... Args>
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 to_respStruct 定义的内联处理。
 * @param [in] jsonData 函数处理参数。
 * @param [in,out] args 函数处理参数。
 * @return 无返回值。
 */
    inline void to_respStruct(const std::string &jsonData, Args &... args)
    {
        Json::Object *pRootJson = Json::init(jsonData);
        Json::Object *pInnerData = Json::get(pRootJson, NETSDK_JSON_INNER_DATA_KEY);
        if (pInnerData)
        {
            process_data(true, pInnerData, args...);
        }
        Json::deinit(pRootJson);
    }

    template <typename... Args>
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 to_struct 定义的内联处理。
 * @param [in] jsonData 函数处理参数。
 * @param [in,out] args 函数处理参数。
 * @return 无返回值。
 */
    inline void to_struct(const std::string &jsonData, Args &... args)
    {
        CRunTimer timer(jsonData);
        Json::Object *pRootJson = Json::init(jsonData);
        Json::Object *pJsonData = Json::get(pRootJson, NETSDK_JSON_INNER_DATA_KEY);
        if (pJsonData)
        {
            process_data(true, pJsonData, args...);
        }
        Json::deinit(pRootJson);
    }
    template <typename T>
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 to_struct 定义的内联处理。
 * @param [in] jsonData 函数处理参数。
 * @param [in,out] data 函数处理参数。
 * @return 无返回值。
 */
    inline void to_struct(const std::string &jsonData, T &data)
    {
        CRunTimer timer(jsonData);
        Json::Object *pRootJson = Json::init(jsonData);
        Json::Object *pJsonData = Json::get(pRootJson, NETSDK_JSON_INNER_DATA_KEY);
        if (pJsonData)
        {
            deal(pJsonData, data, true);
        }
        Json::deinit(pRootJson);
    }
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 get_respCode 定义的内联处理。
 * @param [in] data 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

    inline int get_respCode(const std::string &data)
    {
        int nCode = 0;

        Json::Object *pJsonData = Json::init(data);
        if (pJsonData)
        {
            Json::get(pJsonData, NETSDK_JSON_RETURN_KEY, nCode);
            Json::deinit(pJsonData);
        }

        return nCode;
    }

}
