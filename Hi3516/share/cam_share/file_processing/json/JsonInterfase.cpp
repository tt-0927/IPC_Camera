/*
 *  File Name: JsonInterfase.cpp
 *  Created on: 2022年12月02日
 *  Author: zjc
 *  description: json封装接口
 */

#include "JsonInterfase.h"
#include <cstring>

using namespace Json;

Object* Json::init()
{
    return cJSON_CreateObject();
}

Object* Json::init(char *pObj)
{
    return cJSON_Parse(pObj);
}

Object* Json::init(const char *pObj)
{
    return cJSON_Parse(pObj);
}

Object *Json::init(std::string strObj)
{
    return cJSON_Parse(strObj.c_str());
}

void Json::deinit(Object *&pObj)
{
    if (pObj) {
        cJSON_Delete(pObj);
        pObj = nullptr;
    }
}

void Json::add(Object *pObj, std::string key, int nValue)
{
	if(!pObj || key.empty()) {
        return;
	}
	if(cJSON_AddNumberToObject(pObj, key.c_str(), nValue) == nullptr) {
		return;
	}
}

void Json::add(Object *pObj, std::string key, long long lValue)
{
	if(!pObj || key.empty()) {
        return;
	}
	if(cJSON_AddNumberToObject(pObj, key.c_str(), lValue) == nullptr) {
		return;
	}
}

void Json::add(Object *pObj, std::string key, double dValue)
{
	if(!pObj || key.empty()) {
        return;
	}
	if(cJSON_AddNumberToObject(pObj, key.c_str(), dValue) == nullptr) {
		return;
	}
}

void Json::add(Object *pObj, std::string key, Object *&pItem)
{
	if(!pObj || key.empty()) {
        return;
	}
    cJSON_AddItemToObject(pObj, key.c_str(), pItem);
    pItem = nullptr;
}

/*新增:在 JSON 对象中添加一个键值对，值为null*/
void Json::add(Object *pObj, std::string key)
{
	if(!pObj || key.empty()) {
        return;
	}
    cJSON_AddNullToObject(pObj, key.c_str());
}

Object *Json::get(Object *pObj, std::string item)
{
	if(!pObj || item.empty()) {
        return nullptr;
	}
    return cJSON_GetObjectItem(pObj, item.c_str());
}

bool Json::get(Object *pObj, int &nValue)
{
    if(!pObj) {
        return false;
	}
    nValue = pObj->valueint;
    return true;
}

bool Json::get(Object *pObj, double &dValue)
{
    if(!pObj) {
        return false;
	}
    dValue = pObj->valuedouble;
    return true;
}

bool Json::get(Object *pObj, std::string &value)
{
    if(!pObj) {
        return false;
	}
    value = pObj->valuestring;
    return true;
}
bool Json::get(Object *pObj, std::string key, int &nValue)
{
    if(!pObj || key.empty()) {
        return false;
	}
    bool bRet = false;
    do {
        auto pItem = cJSON_GetObjectItem(pObj, key.c_str());
        if (!pItem) {
            break;
        }
	    nValue = pItem->valueint;
        bRet = true;
    } while (0);
    return bRet;
}

bool Json::get(Object* pObj, std::string key, long long &lValue)
{
    if(!pObj || key.empty()) {
        return false;
	}
    bool bRet = false;
    do {
        auto pItem = cJSON_GetObjectItem(pObj, key.c_str());
        if (!pItem) {
            break;
        }
	    lValue = pItem->valuedouble;
        bRet = true;
    } while (0);
    return bRet;
}

bool Json::get(Object *pObj, std::string key, double &dValue)
{
    if(!pObj || key.empty()) {
        return false;
	}
    bool bRet = false;
    do {
        auto pItem = cJSON_GetObjectItem(pObj, key.c_str());
        if (!pItem) {
            break;
        }
	    dValue = pItem->valuedouble;
        bRet = true;
    } while (0);
    return bRet;
}

bool Json::get(Object *pObj, std::string key, float &fValue)
{
    if(!pObj || key.empty()) {
        return false;
	}
    bool bRet = false;
    do {
        auto pItem = cJSON_GetObjectItem(pObj, key.c_str());
        if (!pItem) {
            break;
        }
	    fValue = pItem->valuedouble;
        bRet = true;
    } while (0);
    return bRet;
}

bool Json::get(Object *pObj, std::string key, std::string &value)
{
    if(!pObj || key.empty()) {
        return false;
	}
    bool bRet = false;
    do {
        auto pItem = cJSON_GetObjectItem(pObj, key.c_str());
        if (!pItem) {
            return false;
        }
        if (pItem->type == cJSON_NULL) {
            return false;
        }
        value = pItem->valuestring;
        bRet = true;
    } while (0);

    return bRet;
}

bool Json::get(Object *pObj, std::string key, size_t nSize, char* pValue)
{
    if(!pObj || key.empty()) {
        return false;
	}
    std::string value;
    if (!get(pObj, key, value)) {
        return false;
    }
    strncpy(pValue, value.c_str(), nSize);
    return true;
}

bool Json::get(const char *pObj, std::string key, int &nValue)
{
    if(!pObj || key.empty()) {
        return false;
	}
    Object *pRoot = cJSON_Parse(pObj);
	if(pRoot == nullptr) {
        return false;
	}
    bool bRet = get(pRoot, key, nValue);
    deinit(pRoot);
    return bRet;
}

bool Json::get(const char *pObj, std::string key, double &dValue)
{
    if(!pObj || key.empty()) {
        return false;
	}
    Object *pRoot = cJSON_Parse(pObj);
	if(pRoot == nullptr) {
        return false;
	}
    bool bRet = get(pRoot, key, dValue);
    deinit(pRoot);
    return bRet;
}

bool Json::get(const char *pObj, std::string item, std::string key, int &nValue)
{
    if (!pObj || item.empty() || key.empty()) {
        return false;
    }

    Object* pRoot = cJSON_Parse(pObj);
	if(!pRoot) {
        return false;
	}
    bool bRet = false;
    do {
        Object *pItems = cJSON_GetObjectItem(pRoot, item.c_str());
        if (!pItems) {
            break;
        }
        auto pItem = cJSON_GetObjectItem(pItems, key.c_str());
        if (!pItem) {
            break;
        }
	    nValue = pItem->valueint;
        bRet = true;
    } while (0);

    deinit(pRoot);
    return bRet;
}

bool Json::get(const char *pObj, std::string item, std::string key, double &dValue)
{
    if (!pObj || item.empty() || key.empty()) {
        return false;
    }

    Object* pRoot = cJSON_Parse(pObj);
	if(!pRoot) {
        return false;
	}
    bool bRet = false;
    do {
        Object *pItems = cJSON_GetObjectItem(pRoot, item.c_str());
        if (!pItems) {
            break;
        }
        auto pItem = cJSON_GetObjectItem(pItems, key.c_str());
        if (!pItem) {
            break;
        }
	    dValue = pItem->valuedouble;
        bRet = true;
    } while (0);

    deinit(pRoot);
    return bRet;
}

bool Json::get(const char *pObj, std::string item, std::string key, std::string &value)
{
    if (!pObj || item.empty() || key.empty()) {
        return false;
    }

    Object* pRoot = cJSON_Parse(pObj);
	if(!pRoot) {
        return false;
	}
    bool bRet = false;
    do {
        Object *pItems = cJSON_GetObjectItem(pRoot, item.c_str());
        if (!pItems) {
            break;
        }
        auto pItem = cJSON_GetObjectItem(pItems, key.c_str());
        if (!pItem) {
            break;
        }
	    value = pItem->valuestring;
        bRet = true;
    } while (0);

    deinit(pRoot);
    return bRet;
}

bool Json::get(const char *pObj, std::string key, std::string &value)
{
    if(!pObj || key.empty()) {
        return false;
	}
    Object* pRoot = cJSON_Parse(pObj);
    if(!pRoot) {
        return false;
    }
    bool bRet = get(pRoot, key, value);
    deinit(pRoot);
    return bRet;
}

bool Json::get(const char *pObj, std::string key, size_t nSize, char* pValue)
{
    if (!pObj || key.empty() || !pValue) {
        return false;
    }
    std::string value;
    if (!get(pObj, key, value)) {
        return false;
    }
    strncpy(pValue, value.c_str(), nSize);
    return true;
}
bool Json::get(const char *pObj, std::string item, std::string key, size_t nSize, char* pValue)
{
    if (!pObj || item.empty() || key.empty() || !pValue) {
        return false;
    }
    std::string value;
    if (!get(pObj, item, key, value)) {
        return false;
    }
    strncpy(pValue, value.c_str(), nSize);
    return true;
}
void Json::add(Object *pObj, std::string key, std::string value)
{
	if(!pObj || key.empty()) {
        return;
	}
	if(cJSON_AddStringToObject(pObj, key.c_str(), value.c_str()) == nullptr) {
		return;
	}
}

/*新增:在 JSON 对象中删除一个字段*/
void Json::remove(Object *pObj, std::string key)
{
	if(!pObj || key.empty()) {
        return;
	}
    cJSON_DeleteItemFromObject(pObj, key.c_str());
}


void Json::update(Object *pObj, std::string key, int nValue)
{
	if(!pObj || key.empty()) {
        return;
	}
	if(cJSON_ReplaceItemInObject(pObj, key.c_str(), cJSON_CreateNumber(nValue)) == false) {
		return;
	}
}

void Json::update(Object *pObj, std::string key, std::string value)
{
	if(!pObj || key.empty()) {
        return;
	}
	if(cJSON_ReplaceItemInObject(pObj, key.c_str(), cJSON_CreateString(value.c_str())) == false) {
		return;
	}
}

std::string Json::to_string(Object* pObj)
{
    if (!pObj) {
        return std::string();
    }
    char *p = cJSON_Print(pObj);
    if (p) {
        std::string out(p);
        free(p);
        p = nullptr;
        return out;
    } else {
        return std::string();
    }
}

char *Json::print(Object *pObj)
{
    if (!pObj) {
        return nullptr;
    }
    return cJSON_Print(pObj);
}

void Json::release(char *&p)
{
    if (p) {
        free(p);
        p = nullptr;
    }
}


/* 数组 */
Object* Json::Array::init()
{
    return cJSON_CreateArray();
}
Object* Json::Array::init(const int *nums, int nCount)
{
    return cJSON_CreateIntArray(nums, nCount);
}

Object* Json::Array::init(const float *nums, int nCount)
{
    return cJSON_CreateFloatArray(nums, nCount);
}
Object* Json::Array::init(const double *nums, int nCount)
{
    return cJSON_CreateDoubleArray(nums, nCount);
}
Object* Json::Array::init(const char *const *strings, int nCount)
{
    return cJSON_CreateStringArray(strings,nCount);
}

int Json::Array::size(Object *pObj)
{
    return cJSON_GetArraySize(pObj);
}

Object *Json::Array::get(Object *pObj, int nIndex)
{
    return cJSON_GetArrayItem(pObj, nIndex);
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

void Json::Array::add(Object *pArr, Object *&pItem)
{
    cJSON_AddItemToArray(pArr, pItem);
    pItem = nullptr;
}


#if 0
#include <iostream>
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
