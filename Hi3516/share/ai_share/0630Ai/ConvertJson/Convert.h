/* 不加唯一宏，只能包含在.cpp文件,且放在最后包含 */

#include <list>
#include <set>
#include <string>
#include <vector>

#include "JsonInterfase.h"

namespace Ai0630_NS
{
    /* 数据 <--> json, bOutStruct：0输出数据，1输出json*/
    class Convert
    {
    public:

        Convert(bool bOutStruct)
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
        template<typename T>
        void field(Json::Object* pRootJson, std::string key, T& value)
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

        template<typename T>
        void field(Json::Object* pRootJson, T& value)
        {
            if (m_bOutValue)
            {
                Json::Object* pDataJson = Json::get(pRootJson, "Data");
                if (pDataJson)
                {
                    value = Json::to_string(pDataJson);
                }
            }
            else
            {
                Json::Object* pDataJson = Json::init(value);
                if (pDataJson)
                {
                    Json::add(pRootJson, "Data", pDataJson);
                }
            }
        }

        /**
         * @brief 数组转换转换
         * @tparam T 模板类型，支持类型见Json.h中get、add的value
         * @param pRootJson Json句柄
         * @param key 键
         * @param value 值
         */
        template<typename T>
        void field(Json::Object* pRootJson, const std::string key, std::vector<T>& vec)
        {
            /* 调用已实现的结构体转换 */
            if (m_bOutStruct)
            {
                Json::Object* items = Json::get(pRootJson, key);
                Json::Array::get(items, vec);
            }
            else
            {
                Json::Object* items = Json::Array::init();
                for (size_t i = 0; i < vec.size(); i++)
                {
                    Json::Object* item = Json::init();
                    if (!item)
                    {
                        continue;
                    }
                    T& data = vec[i];
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
        template<typename T>
        void field(Json::Object* pRootJson, const std::string key, std::set<T>& setData)
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
         * @brief 结构体转换
         * @tparam T 模板类型，支持类型基于已实现的dealJson
         * @param pRootJson Json句柄
         * @param stStruct 结构体
         */
        template<typename T>
        void structure(Json::Object* pRootJson, T& stStruct)
        {
            /* 调用已实现的结构体转换 */
            dealJson(pRootJson, stStruct, m_bOutStruct);
        }

        /**
         * @brief 结构体转换
         * @tparam T 模板类型，支持类型基于已实现的dealJson
         * @param pRootJson Json句柄
         * @param key 键
         * @param stStruct 结构体
         */
        template<typename T>
        void structure(Json::Object* pRootJson, const std::string key, T& stStruct)
        {
            /* 调用已实现的结构体转换 */
            Json::Object* pJsonData = nullptr;
            if (m_bOutStruct)
            {
                /* json -> struct */
                pJsonData = Json::get(pRootJson, key);
                dealJson(pJsonData, stStruct, m_bOutStruct);
            }
            else
            {
                /* json <- struct */
                pJsonData = Json::init();
                dealJson(pJsonData, stStruct, m_bOutStruct);
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
        template<typename T>
        void structure(Json::Object* pRootJson, const std::string key, std::vector<T>& vec)
        {
            /* 调用已实现的结构体转换 */
            if (m_bOutStruct)
            {
                Json::Object* pArray = Json::get(pRootJson, key);
                int           nSize  = Json::Array::size(pArray);
                for (int i = 0; i < nSize; i++)
                {
                    Json::Object* item = Json::Array::get(pArray, i);
                    if (!item)
                    {
                        continue;
                    }
                    T info;
                    structure(item, info);

                    vec.push_back(info);
                }
            }
            else
            {
                Json::Object* items = Json::Array::init();

                for (size_t i = 0; i < vec.size(); i++)
                {
                    Json::Object* item = Json::init();
                    if (!item)
                    {
                        continue;
                    }
                    T& info = vec[i];
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
        template<typename T>
        void structure(Json::Object* pRootJson, const std::string key, std::list<T>& list)
        {
            /* 调用已实现的结构体转换 */
            if (m_bOutStruct)
            {
                Json::Object* pArray = Json::get(pRootJson, key);
                int           nSize  = Json::Array::size(pArray);
                for (int i = 0; i < nSize; i++)
                {
                    Json::Object* item = Json::Array::get(pArray, i);
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
                Json::Object* items = Json::Array::init();

                for (auto& data : list)
                {
                    Json::Object* item = Json::init();
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
        template<typename T>
        void structure(Json::Object* pRootJson, const std::string key, std::set<T>& set)
        {
            /* 调用已实现的结构体转换 */
            if (m_bOutStruct)
            {
                Json::Object* pArray = Json::get(pRootJson, key);
                int           nSize  = Json::Array::size(pArray);
                for (int i = 0; i < nSize; i++)
                {
                    Json::Object* item = Json::Array::get(pArray, i);
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
                Json::Object* items = Json::Array::init();

                for (auto& data : set)
                {
                    Json::Object* item = Json::init();
                    if (!item)
                    {
                        continue;
                    }
                    T t = data;
                    structure(item, t);
                    Json::Array::add(items, item);
                }
                Json::add(pRootJson, key.c_str(), items);
            }
        }

    private:

        bool m_bOutValue  = false;
        bool m_bOutStruct = false;
    };
}    // namespace Ai0630_NS
