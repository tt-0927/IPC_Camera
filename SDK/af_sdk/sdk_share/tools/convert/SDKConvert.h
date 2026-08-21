
#pragma once
#include <list>
#include <chrono>
#include <set>
#include <algorithm>
#include <cstring>
#include "Json.h"
#include "NetTVSDKHttpUrl.h"

// 库通用头文件
#ifdef NET_TV_SDK_SERVER_API
    #include "NetTVSDKServerInterface.h"
#elif defined(NET_TV_SDK_CLIENT_API)
    #include "NetTVSDKClientInterface.h"
#else
    #include "NetTVSDKCommon.h"
#endif


//关键字段定义
#define JSON_REQ_KEY                "Req"               /* 请求内容 */
#define JSON_RESP_KEY               "Resp"              /* 响应内容 */
#define JSON_RESP_CODE_KEY          "RespCode"          /* 响应码字段 */
#define JSON_DATA_KEY               "Data"              /* 数据字段 */

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
            printf("RunTimer: cost %lld ms, data %s", duration / 1000, m_data.c_str());
        }
    }
private:
    const std::string &m_data;
    std::chrono::high_resolution_clock::time_point m_startTime;
};


namespace SDKConvert
{
   
	class CSDKConvert
    {
    public:
        CSDKConvert(bool bOutStruct)
            : m_bOutValue(bOutStruct),
              m_bOutStruct(bOutStruct)
        {
        }
        /**
         * @brief 字段转换
         * @tparam T 模板类型，支持类型见Json.h中get、add的value
         * @param pRootJson Json句柄
         * @param key 键
         * @param value 值
         */
        template <typename T>
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

        template <size_t N>
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
         * @brief 数组转换转换
         * @tparam T 模板类型，支持类型见Json.h中get、add的value
         * @param pRootJson Json句柄
         * @param key 键
         * @param value 值
         */
        template <typename T>
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
         * @brief 数组转换转换
         * @tparam T 模板类型，支持类型见Json.h中get、add的value
         * @param pRootJson Json句柄
         * @param key 键
         * @param value 值
         */
        template <typename T>
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
         * @brief 数组字段转换 (custom for fixed size array)
         */
        template <typename T>
        void field_array(Json::Object *pRootJson, std::string key, T *value, UINT32 count, UINT32 max_len)
        {
            if (m_bOutValue) // Json -> Array
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
            else // Array -> Json
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
         * @brief 结构体转换
         * @tparam T 模板类型，支持类型基于已实现的deal
         * @param pRootJson Json句柄
         * @param stStruct 结构体
         */
        template <typename T>
        void structure(Json::Object *pRootJson, T &stStruct)
        {
            /* 调用已实现的结构体转换 */
            deal(pRootJson, stStruct, m_bOutStruct);
        }

        /**
         * @brief 结构体转换
         * @tparam T 模板类型，支持类型基于已实现的deal
         * @param pRootJson Json句柄
         * @param key 键
         * @param stStruct 结构体
         */
        template <typename T>
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
         * @brief 结构体数组转换
         * @tparam T 模板类型，支持类型基于已实现的deal
         * @param pRootJson Json句柄
         * @param key 键
         * @param vec 结构体数组
         */
        template <typename T>
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
         * @brief 结构体链表转换
         * @tparam T 模板类型，支持类型基于已实现的deal
         * @param pRootJson Json句柄
         * @param key 键
         * @param vec 结构体数组
         */
        template <typename T>
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
         * @brief 结构体set容器转换
         * @tparam T 模板类型，支持类型基于已实现的deal
         * @param pRootJson Json句柄
         * @param key 键
         * @param vec 结构体数组
         */
        template <typename T>
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

     inline void deal(Json::Object* pRootJson, SeesionMessage_S& stInfo, bool bOutStruct)
    {
        if (!pRootJson)
        {
            return;
        }
        CSDKConvert convert(bOutStruct);
        convert.field(pRootJson, "SeesionId", stInfo.SeesionId);
    }


    // 可变模板参数的递归处理函数
    template <typename T>
    void process_data(bool bStruct, Json::Object *pRootJson, T &data)
    {
        deal(pRootJson, data, bStruct);
    }

    
    // 空参数包的终止版本
    inline void process_data(bool /*bStruct*/, Json::Object * /*pRootJson*/)
    {
        // 空参数包，什么也不做
    }

    // 递归展开的函数模板，用于处理多个参数
    template <typename T, typename... Args>
    void process_data(bool bStruct, Json::Object *pRootJson, T &data, Args &... args)
    {
        deal(pRootJson, data, bStruct);
        process_data(bStruct, pRootJson, args...); // 递归调用处理剩余参数
    }


    // 多参模板 to_string 函数
    template <typename... Args>
    inline std::string to_string(Args &... args)
    {
        RunTimer timer();
        Json::Object *pRootJson = Json::init();
        process_data(false, pRootJson, args...);  // 处理所有传入的参数
        std::string jsonString = Json::to_string(pRootJson);
        Json::deinit(pRootJson);
        return jsonString;
    }

    template <typename... Args>
    inline bool from_string(const std::string& jsonData, Args &... args)
    {
        RunTimer timer(jsonData);
        Json::Object *pRootJson = Json::init(jsonData);
        if (!pRootJson)
        {
            return false;
        }

        process_data(true, pRootJson, args...);
        Json::deinit(pRootJson);
        return true;
    }

    template <typename... Args>
    inline std::string to_respString(int nRespCode,Args &... args)
    {
        RunTimer timer();
          fprintf(stderr, "[SDKConvert::to_respString] enter respCode=%d argc=%zu\n",
                    nRespCode, (size_t)sizeof...(Args));
        Json::Object *pRootJson = Json::init();
         fprintf(stderr, "[SDKConvert::to_respString] root init=%p\n", (void*)pRootJson);
        Json::add(pRootJson, JSON_RESP_CODE_KEY, nRespCode);

        Json::Object *pRespJson = Json::init();
         fprintf(stderr, "[SDKConvert::to_respString] resp init=%p\n", (void*)pRespJson);
        process_data(false, pRespJson, args...);  

        Json::add(pRootJson, JSON_RESP_KEY, pRespJson);

        fprintf(stderr, "[SDKConvert::to_respString] before to_string root=%p resp=%p\n",
                    (void*)pRootJson, (void*)pRespJson);

        std::string data = Json::to_string(pRootJson);
          fprintf(stderr, "[SDKConvert::to_respString] after to_string len=%zu\n", data.size());

        Json::deinit(pRootJson);

         fprintf(stderr, "[SDKConvert::to_respString] leave\n");

        return data;
    }

    template <typename... Args>
    inline std::string to_respString(NET_TV_COMMON_ECODE_E enCode)
    {
        RunTimer timer();
        Json::Object *pRootJson = Json::init();
        Json::add(pRootJson, JSON_RESP_CODE_KEY, (int &)enCode);

        Json::Object *pRespJson = Json::init();
        // process_data(false, pRespJson, args...);  

        Json::add(pRootJson, JSON_RESP_KEY, pRespJson);

        std::string data = Json::to_string(pRootJson);

        Json::deinit(pRootJson);

        return data;
    }

     template <typename... Args>
    inline void to_respStruct(const std::string &jsonData, Args &... args)
    {
        RunTimer timer(jsonData);
        Json::Object *pRootJson = Json::init(jsonData);
        Json::Object *pRespJson = Json::get(pRootJson, JSON_RESP_KEY);

        process_data(true, pRespJson, args...);  // 处理所有传入的参数
        // Json::deinit(pRespJson);
        Json::deinit(pRootJson);
    }

    template <typename... Args>
    inline void to_struct(const std::string &jsonData, Args &... args)
    {
        RunTimer timer(jsonData);
        Json::Object *pRootJson = Json::init(jsonData);
        Json::Object *pJsonData = Json::get(pRootJson, "DATA");

        process_data(true, pJsonData, args...);  // 处理所有传入的参数
        Json::deinit(pRootJson);
    }
    template <typename T>
    inline void to_struct(const std::string &jsonData, T &data)
    {
        //RunTimer timer(jsonData);
        printf("jsonData[%s]\n",jsonData.c_str());
        Json::Object *pRootJson = Json::init(jsonData);
        Json::Object *pJsonData = Json::get(pRootJson, "DATA");

        deal(pJsonData, data, true);
        Json::deinit(pRootJson);
    }

    inline void fill_head(std::string &data,int nCode)
    {
        Json::Object *pJsonRoot = Json::init();
        Json::add(pJsonRoot,JSON_RESP_CODE_KEY, nCode);

        Json::Object *pJsonData = Json::init(data);
        if (pJsonData)
        {
            Json::add(pJsonRoot, JSON_DATA_KEY, pJsonData);
        }

        data = Json::to_string(pJsonRoot);
        Json::deinit(pJsonRoot);
        return;
    }

    inline void fill_resp(std::string &data,int nCode)
    {
        Json::Object *pJsonRoot = Json::init();
        Json::add(pJsonRoot,JSON_RESP_CODE_KEY, nCode);

        Json::Object *pJsonData = Json::init(data);
        if (pJsonData)
        {
            Json::add(pJsonRoot, JSON_DATA_KEY, pJsonData);
        }

        data = Json::to_string(pJsonRoot);
        Json::deinit(pJsonRoot);
        return;
    }

    inline int get_respCode(const std::string &data)
    {
        int nCode = 0;

        Json::Object *pJsonData = Json::init(data);
        if (pJsonData)
        {
            Json::get(pJsonData, JSON_RESP_CODE_KEY, nCode);
            Json::deinit(pJsonData);
        }

        return nCode;
    }

}
