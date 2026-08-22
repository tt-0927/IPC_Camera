/**
 * @file Json.cpp
 * @author tianl (tianl@kfb.cn)
 * @date 2026-07-28
 * @LastEditors  : qinjt@kfb.cn
 * @LastEditTime : 2026-07-28
 *
 * @brief Json 模块实现
 * 功能说明：
 * 1. 实现 Json 模块核心逻辑
 * 2. 校验输入参数并管理模块资源生命周期
 * 3. 向上层提供可复用的 SDK 能力
 */
/*
 *  File Name: JsonInterfase.cpp
 *  Created on: 2022年12月02日
 *  Author: zjc
 *  description: json封装接口
 */

#include "Json.h"
#include <cstring>

using namespace Json;

Object *Json::init()
{
    return cJSON_CreateObject();
}

Object *Json::init(char *pObj)
{
    return cJSON_Parse(pObj);
}

Object *Json::init(const char *pObj)
{
    return cJSON_Parse(pObj);
}

Object *Json::init(std::string strObj)
{
    return cJSON_Parse(strObj.c_str());
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 deinit 定义的内部处理。
 * @param [in,out] pObj 函数处理参数。
 * @return 无返回值。
 */

void Json::deinit(Object *&pObj)
{
    if (pObj)
    {
        cJSON_Delete(pObj);
        pObj = nullptr;
    }
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 add 定义的内部处理。
 * @param [in,out] pObj 函数处理参数。
 * @param [in] key 函数处理参数。
 * @param [in] nValue 函数处理参数。
 * @return 无返回值。
 */

void Json::add(Object *pObj, std::string key, bool nValue)
{

    if (!pObj || key.empty())
    {
        return;
    }
    if (cJSON_AddBoolToObject(pObj, key.c_str(), nValue) == nullptr)
    {
        return;
    }
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 add 定义的内部处理。
 * @param [in,out] pObj 函数处理参数。
 * @param [in] key 函数处理参数。
 * @param [in] nValue 函数处理参数。
 * @return 无返回值。
 */
void Json::add(Object *pObj, std::string key, int nValue)
{
    if (!pObj || key.empty())
    {
        return;
    }
    if (cJSON_AddNumberToObject(pObj, key.c_str(), nValue) == nullptr)
    {
        return;
    }
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 add 定义的内部处理。
 * @param [in,out] pObj 函数处理参数。
 * @param [in] key 函数处理参数。
 * @param [in] nValue 函数处理参数。
 * @return 无返回值。
 */

void Json::add(Object *pObj, std::string key, unsigned int nValue)
{
    add(pObj, key, (int)nValue);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 add 定义的内部处理。
 * @param [in,out] pObj 函数处理参数。
 * @param [in] key 函数处理参数。
 * @param [in] lValue 函数处理参数。
 * @return 无返回值。
 */
void Json::add(Object *pObj, std::string key, long long lValue)
{
    if (!pObj || key.empty())
    {
        return;
    }
    if (cJSON_AddNumberToObject(pObj, key.c_str(), lValue) == nullptr)
    {
        return;
    }
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 add 定义的内部处理。
 * @param [in,out] pObj 函数处理参数。
 * @param [in] key 函数处理参数。
 * @param [in] dValue 函数处理参数。
 * @return 无返回值。
 */

void Json::add(Object *pObj, std::string key, double dValue)
{
    if (!pObj || key.empty())
    {
        return;
    }
    if (cJSON_AddNumberToObject(pObj, key.c_str(), dValue) == nullptr)
    {
        return;
    }
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 add 定义的内部处理。
 * @param [in,out] pObj 函数处理参数。
 * @param [in] key 函数处理参数。
 * @param [in,out] pItem 函数处理参数。
 * @return 无返回值。
 */

void Json::add(Object *pObj, std::string key, Object *pItem)
{
    if (!pObj || key.empty())
    {
        return;
    }
    cJSON_AddItemToObject(pObj, key.c_str(), pItem);
}

/*新增:在 JSON 对象中添加一个键值对，值为null*/
void Json::add(Object *pObj, std::string key)
{
    if (!pObj || key.empty())
    {
        return;
    }
    cJSON_AddNullToObject(pObj, key.c_str());
}

Object *Json::get(Object *pObj, std::string item)
{
    if (!pObj || item.empty())
    {
        return nullptr;
    }
    return cJSON_GetObjectItem(pObj, item.c_str());
}

bool Json::Value::get(Object *pObj, bool &bValue)
{
    if (!pObj)
    {
        return false;
    }
    if (pObj->valueint == 0)
    {
        bValue = false;
    }
    else
    {
        bValue = true;
    }
    return true;
}

bool Json::Value::get(Object *pObj, int &nValue)
{
    if (!pObj)
    {
        return false;
    }
    nValue = pObj->valueint;
    return true;
}

bool Json::Value::get(Object *pObj, double &dValue)
{
    if (!pObj)
    {
        return false;
    }
    dValue = pObj->valuedouble;
    return true;
}

bool Json::Value::get(Object *pObj, std::string &value)
{
    if (!pObj)
    {
        return false;
    }
    value = pObj->valuestring;
    return true;
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 查询或校验 get 对应的数据。
 * @param [in,out] pObj 函数处理参数。
 * @param [in] key 函数处理参数。
 * @param [in,out] bValue 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

bool Json::get(Object *pObj, std::string key, bool &bValue)
{
    if (!pObj || key.empty())
    {
        return false;
    }
    bool bRet = false;
    do
    {
        auto pItem = cJSON_GetObjectItem(pObj, key.c_str());
        if (!pItem)
        {
            break;
        }

        if (pItem->valueint == 0)
        {
            bValue = false;
        }
        else
        {
            bValue = true;
        }
        bRet = true;
    } while (0);
    return bRet;
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 查询或校验 get 对应的数据。
 * @param [in,out] pObj 函数处理参数。
 * @param [in] key 函数处理参数。
 * @param [in,out] nValue 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

bool Json::get(Object *pObj, std::string key, int &nValue)
{
    if (!pObj || key.empty())
    {
        return false;
    }
    bool bRet = false;
    do
    {
        auto pItem = cJSON_GetObjectItem(pObj, key.c_str());
        if (!pItem)
        {
            break;
        }
        nValue = pItem->valueint;
        bRet = true;
    } while (0);
    return bRet;
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 查询或校验 get 对应的数据。
 * @param [in,out] pObj 函数处理参数。
 * @param [in] key 函数处理参数。
 * @param [in,out] nValue 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

bool Json::get(Object *pObj, std::string key, int16_t &nValue)
{
    if (!pObj || key.empty())
    {
        return false;
    }
    bool bRet = false;
    do
    {
        auto pItem = cJSON_GetObjectItem(pObj, key.c_str());
        if (!pItem)
        {
            break;
        }
        nValue = (int16_t)pItem->valueint;
        bRet = true;
    } while (0);
    return bRet;
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 查询或校验 get 对应的数据。
 * @param [in,out] pObj 函数处理参数。
 * @param [in] key 函数处理参数。
 * @param [in,out] nValue 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

bool Json::get(Object *pObj, std::string key, unsigned int &nValue)
{
    return get(pObj, key, (int &)nValue);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 查询或校验 get 对应的数据。
 * @param [in,out] pObj 函数处理参数。
 * @param [in] key 函数处理参数。
 * @param [in,out] lValue 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

bool Json::get(Object *pObj, std::string key, long long &lValue)
{
    if (!pObj || key.empty())
    {
        return false;
    }
    bool bRet = false;
    do
    {
        auto pItem = cJSON_GetObjectItem(pObj, key.c_str());
        if (!pItem)
        {
            break;
        }
        lValue = pItem->valuedouble;
        bRet = true;
    } while (0);
    return bRet;
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 查询或校验 get 对应的数据。
 * @param [in,out] pObj 函数处理参数。
 * @param [in] key 函数处理参数。
 * @param [in,out] dValue 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

bool Json::get(Object *pObj, std::string key, double &dValue)
{
    if (!pObj || key.empty())
    {
        return false;
    }
    bool bRet = false;
    do
    {
        auto pItem = cJSON_GetObjectItem(pObj, key.c_str());
        if (!pItem)
        {
            break;
        }
        dValue = pItem->valuedouble;
        bRet = true;
    } while (0);
    return bRet;
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 查询或校验 get 对应的数据。
 * @param [in,out] pObj 函数处理参数。
 * @param [in] key 函数处理参数。
 * @param [in,out] fValue 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

bool Json::get(Object *pObj, std::string key, float &fValue)
{
    if (!pObj || key.empty())
    {
        return false;
    }
    bool bRet = false;
    do
    {
        auto pItem = cJSON_GetObjectItem(pObj, key.c_str());
        if (!pItem)
        {
            break;
        }
        fValue = pItem->valuedouble;
        bRet = true;
    } while (0);
    return bRet;
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 查询或校验 get 对应的数据。
 * @param [in,out] pObj 函数处理参数。
 * @param [in] key 函数处理参数。
 * @param [in,out] value 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

bool Json::get(Object *pObj, std::string key, std::string &value)
{
    if (!pObj || key.empty())
    {
        return false;
    }
    bool bRet = false;
    do
    {
        auto pItem = cJSON_GetObjectItem(pObj, key.c_str());
        if (!pItem)
        {
            return false;
        }
        if (pItem->type == cJSON_NULL)
        {
            return false;
        }
        value = pItem->valuestring;
        bRet = true;
    } while (0);

    return bRet;
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 查询或校验 get 对应的数据。
 * @param [in,out] pObj 函数处理参数。
 * @param [in] key 函数处理参数。
 * @param [in] nSize 函数处理参数。
 * @param [in,out] pValue 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

bool Json::get(Object *pObj, std::string key, size_t nSize, char *pValue)
{
    if (!pObj || key.empty())
    {
        return false;
    }
    std::string value;
    if (!get(pObj, key, value))
    {
        return false;
    }
    strncpy(pValue, value.c_str(), nSize);
    return true;
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 查询或校验 get 对应的数据。
 * @param [in] pObj 函数处理参数。
 * @param [in] key 函数处理参数。
 * @param [in,out] nValue 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

bool Json::get(const char *pObj, std::string key, int &nValue)
{
    if (!pObj || key.empty())
    {
        return false;
    }
    Object *pRoot = cJSON_Parse(pObj);
    if (pRoot == nullptr)
    {
        return false;
    }
    bool bRet = get(pRoot, key, nValue);
    deinit(pRoot);
    return bRet;
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 查询或校验 get 对应的数据。
 * @param [in] pObj 函数处理参数。
 * @param [in] key 函数处理参数。
 * @param [in,out] dValue 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

bool Json::get(const char *pObj, std::string key, double &dValue)
{
    if (!pObj || key.empty())
    {
        return false;
    }
    Object *pRoot = cJSON_Parse(pObj);
    if (pRoot == nullptr)
    {
        return false;
    }
    bool bRet = get(pRoot, key, dValue);
    deinit(pRoot);
    return bRet;
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 查询或校验 get 对应的数据。
 * @param [in] pObj 函数处理参数。
 * @param [in] item 函数处理参数。
 * @param [in] key 函数处理参数。
 * @param [in,out] nValue 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

bool Json::get(const char *pObj, std::string item, std::string key, int &nValue)
{
    if (!pObj || item.empty() || key.empty())
    {
        return false;
    }

    Object *pRoot = cJSON_Parse(pObj);
    if (!pRoot)
    {
        return false;
    }
    bool bRet = false;
    do
    {
        Object *pItems = cJSON_GetObjectItem(pRoot, item.c_str());
        if (!pItems)
        {
            break;
        }
        auto pItem = cJSON_GetObjectItem(pItems, key.c_str());
        if (!pItem)
        {
            break;
        }
        nValue = pItem->valueint;
        bRet = true;
    } while (0);

    deinit(pRoot);
    return bRet;
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 查询或校验 get 对应的数据。
 * @param [in] pObj 函数处理参数。
 * @param [in] item 函数处理参数。
 * @param [in] key 函数处理参数。
 * @param [in,out] dValue 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

bool Json::get(const char *pObj, std::string item, std::string key, double &dValue)
{
    if (!pObj || item.empty() || key.empty())
    {
        return false;
    }

    Object *pRoot = cJSON_Parse(pObj);
    if (!pRoot)
    {
        return false;
    }
    bool bRet = false;
    do
    {
        Object *pItems = cJSON_GetObjectItem(pRoot, item.c_str());
        if (!pItems)
        {
            break;
        }
        auto pItem = cJSON_GetObjectItem(pItems, key.c_str());
        if (!pItem)
        {
            break;
        }
        dValue = pItem->valuedouble;
        bRet = true;
    } while (0);

    deinit(pRoot);
    return bRet;
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 查询或校验 get 对应的数据。
 * @param [in] pObj 函数处理参数。
 * @param [in] item 函数处理参数。
 * @param [in] key 函数处理参数。
 * @param [in,out] value 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

bool Json::get(const char *pObj, std::string item, std::string key, std::string &value)
{
    if (!pObj || item.empty() || key.empty())
    {
        return false;
    }

    Object *pRoot = cJSON_Parse(pObj);
    if (!pRoot)
    {
        return false;
    }
    bool bRet = false;
    do
    {
        Object *pItems = cJSON_GetObjectItem(pRoot, item.c_str());
        if (!pItems)
        {
            break;
        }
        auto pItem = cJSON_GetObjectItem(pItems, key.c_str());
        if (!pItem)
        {
            break;
        }
        value = pItem->valuestring;
        bRet = true;
    } while (0);

    deinit(pRoot);
    return bRet;
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 查询或校验 get 对应的数据。
 * @param [in] pObj 函数处理参数。
 * @param [in] key 函数处理参数。
 * @param [in,out] value 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

bool Json::get(const char *pObj, std::string key, std::string &value)
{
    if (!pObj || key.empty())
    {
        return false;
    }
    Object *pRoot = cJSON_Parse(pObj);
    if (!pRoot)
    {
        return false;
    }
    bool bRet = get(pRoot, key, value);
    deinit(pRoot);
    return bRet;
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 查询或校验 get 对应的数据。
 * @param [in] pObj 函数处理参数。
 * @param [in] key 函数处理参数。
 * @param [in] nSize 函数处理参数。
 * @param [in,out] pValue 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

bool Json::get(const char *pObj, std::string key, size_t nSize, char *pValue)
{
    if (!pObj || key.empty() || !pValue)
    {
        return false;
    }
    std::string value;
    if (!get(pObj, key, value))
    {
        return false;
    }
    strncpy(pValue, value.c_str(), nSize);
    return true;
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 查询或校验 get 对应的数据。
 * @param [in] pObj 函数处理参数。
 * @param [in] item 函数处理参数。
 * @param [in] key 函数处理参数。
 * @param [in] nSize 函数处理参数。
 * @param [in,out] pValue 函数处理参数。
 * @return 返回该处理的状态或结果。
 */
bool Json::get(const char *pObj, std::string item, std::string key, size_t nSize, char *pValue)
{
    if (!pObj || item.empty() || key.empty() || !pValue)
    {
        return false;
    }
    std::string value;
    if (!get(pObj, item, key, value))
    {
        return false;
    }
    strncpy(pValue, value.c_str(), nSize);
    return true;
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 add 定义的内部处理。
 * @param [in,out] pObj 函数处理参数。
 * @param [in] key 函数处理参数。
 * @param [in,out] pValue 函数处理参数。
 * @return 无返回值。
 */
void Json::add(Object *pObj, std::string key, char *pValue)
{
    add(pObj, key, std::string(pValue));
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 add 定义的内部处理。
 * @param [in,out] pObj 函数处理参数。
 * @param [in] key 函数处理参数。
 * @param [in] pValue 函数处理参数。
 * @return 无返回值。
 */
void Json::add(Object *pObj, std::string key, const char *pValue)
{
    add(pObj, key, std::string(pValue));
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 add 定义的内部处理。
 * @param [in,out] pObj 函数处理参数。
 * @param [in] key 函数处理参数。
 * @param [in] value 函数处理参数。
 * @return 无返回值。
 */

void Json::add(Object *pObj, std::string key, std::string value)
{
    if (!pObj || key.empty())
    {
        return;
    }
    if (cJSON_AddStringToObject(pObj, key.c_str(), value.c_str()) == nullptr)
    {
        return;
    }
}

/*新增:在 JSON 对象中删除一个字段*/
void Json::remove(Object *pObj, std::string key)
{
    if (!pObj || key.empty())
    {
        return;
    }
    cJSON_DeleteItemFromObject(pObj, key.c_str());
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 update 对应的处理。
 * @param [in,out] pObj 函数处理参数。
 * @param [in] key 函数处理参数。
 * @param [in] nValue 函数处理参数。
 * @return 无返回值。
 */

void Json::update(Object *pObj, std::string key, int nValue)
{
    if (!pObj || key.empty())
    {
        return;
    }
    if (cJSON_ReplaceItemInObject(pObj, key.c_str(), cJSON_CreateNumber(nValue)) == false)
    {
        return;
    }
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 update 对应的处理。
 * @param [in,out] pObj 函数处理参数。
 * @param [in] key 函数处理参数。
 * @param [in] value 函数处理参数。
 * @return 无返回值。
 */

void Json::update(Object *pObj, std::string key, std::string value)
{
    if (!pObj || key.empty())
    {
        return;
    }
    if (cJSON_ReplaceItemInObject(pObj, key.c_str(), cJSON_CreateString(value.c_str())) == false)
    {
        return;
    }
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 to_string 定义的内部处理。
 * @param [in,out] pObj 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

std::string Json::to_string(Object *pObj)
{
    if (!pObj)
    {
        return std::string();
    }
    char *p = cJSON_Print(pObj);
    if (p)
    {
        std::string out(p);
        free(p);
        p = nullptr;
        return out;
    }
    else
    {
        return std::string();
    }
}

char *Json::print(Object *pObj)
{
    if (!pObj)
    {
        return nullptr;
    }
    return cJSON_Print(pObj);
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 release 定义的内部处理。
 * @param [in,out] p 函数处理参数。
 * @return 无返回值。
 */

void Json::release(char *&p)
{
    if (p)
    {
        free(p);
        p = nullptr;
    }
}

/* 数组 */
Object *Json::Array::init()
{
    return cJSON_CreateArray();
}
Object *Json::Array::init(const int *nums, int nCount)
{
    return cJSON_CreateIntArray(nums, nCount);
}

Object *Json::Array::init(const float *nums, int nCount)
{
    return cJSON_CreateFloatArray(nums, nCount);
}
Object *Json::Array::init(const double *nums, int nCount)
{
    return cJSON_CreateDoubleArray(nums, nCount);
}
Object *Json::Array::init(const char *const *strings, int nCount)
{
    return cJSON_CreateStringArray(strings, nCount);
}

int Json::Array::size(Object *pObj)
{
    return cJSON_GetArraySize(pObj);
}

Object *Json::Array::get(Object *pObj, int nIndex)
{
    return cJSON_GetArrayItem(pObj, nIndex);
}
bool Json::Array::get(Object *pObj, std::vector<int> &value)
{
    if (!pObj)
    {
        return false;
    }
    for (int i = 0; i < size(pObj); i++)
    {
        Object *pItem = get(pObj, i);
        if (pItem)
        {
            int nValue = 0;
            Json::Value::get(pItem, nValue);
            value.push_back(nValue);
        }
    }
    return true;
}
bool Json::Array::get(Object *pObj, std::vector<std::string> &value)
{
    if (!pObj)
    {
        return false;
    }
    for (int i = 0; i < size(pObj); i++)
    {
        Object *pItem = get(pObj, i);
        if (pItem)
        {
            std::string str;
            Json::Value::get(pItem, str);
            value.push_back(str);
        }
    }
    return true;
}

void Json::Array::add(Object *pArr, const char *string)
{
    cJSON_AddItemToArray(pArr, cJSON_CreateString(string));
}

void Json::Array::add(Object *pArr, const int nValue)
{
    cJSON_AddItemToArray(pArr, cJSON_CreateNumber(nValue));
}

void Json::Array::add(Object *pArr, const float fValue)
{
    cJSON_AddItemToArray(pArr, cJSON_CreateNumber(fValue));
}

void Json::Array::add(Object *pArr, Object *pItem)
{
    cJSON_AddItemToArray(pArr, pItem);
}

void Json::Array::add(Object *pArr, std::string value)
{
    add(pArr, value.c_str());
}
#if 0
#include <iostream>
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 运行当前 Demo 的主流程。
 * @return 返回该处理的状态或结果。
 */
int main()
{
    Json::Object *pObj = Json::init();
    Json::Object *pArr = Json::Array::init();
    int num = 3;
    do {
        if (!pObj || ! pArr) {
            break;
        }
        Json::add(pObj, "file_num", num);
        for (int i = 0; i < num; i++) {
            Json::Object *pItems = Json::init();
            if (!pItems) {
                continue;
            }
            Json::add(pItems, "filename", "aaa");

            Json::Array::add(pArr, pItems);
        }
        Json::add(pObj, "data", pArr);
    } while (0);

    auto res = Json::to_string(pObj);
    std::cout << res << std::endl;
    Json::deinit(pObj);
    Json::deinit(pArr);
    return 0;

}
/*
编译：g++ JsonInterfase.cpp cJSON.c  -o test
输出：
{
	"file_num":	3,
	"data":	[{
			"filename":	"aaa"
		}, {
			"filename":	"aaa"
		}, {
			"filename":	"aaa"
		}]
}


*/
#endif
