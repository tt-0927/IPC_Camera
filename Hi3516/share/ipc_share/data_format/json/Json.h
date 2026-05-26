/*
 *  File Name: JsonInterfase.h
 *  Created on: 2022年12月02日
 *  Author: zjc
 *  description: json封装接口
 */

#ifndef JSON_INTERFASE_H_
#define JSON_INTERFASE_H_

#include "cJSON.h"
#include <string>
#include <vector>

namespace Json {

    /* 类 */
    /* （反）初始化项目 */
    using Object = cJSON;
    Object *init();
    Object *init(char *pObj);
    Object *init(const char *pObj);
    Object *init(std::string strObj);
    void deinit(Object *&pObj);

    /* 添加整形、字符串、项目 */
    void add(Object *pObj, std::string key, bool nValue);
    void add(Object *pObj, std::string key, int nValue);
    void add(Object *pObj, std::string key, unsigned int nValue);
    void add(Object *pObj, std::string key, long long lValue);
    void add(Object *pObj, std::string key, double dValue);
    void add(Object *pObj, std::string key, char* pValue);
    void add(Object *pObj, std::string key, const char* pValue);
    void add(Object *pObj, std::string key, std::string value);
    void add(Object *pObj, std::string key, Object *pItem);
    /*新增:在 JSON 对象中添加一个键值对，值为null*/
    void add(Object *pObj, std::string key);
    Object *get(Object *pObj, std::string item);

    /* 直接读取整形、字符串 */
    namespace Value {
        bool get(Object *pObj, bool &bValue);
        bool get(Object *pObj, int &nValue);
        bool get(Object *pObj, double &dValue);
        bool get(Object *pObj, std::string &value);
    }

    bool get(Object *pObj, std::string key, bool &bValue);
    bool get(Object *pObj, std::string key, int &nValue);
    bool get(Object *pObj, std::string key, int16_t &nValue);
    bool get(Object *pObj, std::string key, unsigned int &nValue);
    bool get(Object *pObj, std::string key, long long &lValue);
    bool get(Object *pObj, std::string key, double &dValue);
    bool get(Object *pObj, std::string key, float &fValue);
    bool get(Object *pObj, std::string key, std::string &value);
    bool get(Object *pObj, std::string key, size_t nSize, char* pValue);

    bool get(const char *pObj, std::string key, int &nValue);
    bool get(const char *pObj, std::string key, double &dValue);
    bool get(const char *pObj, std::string key, std::string &value);
    bool get(const char *pObj, std::string key, size_t nSize, char* pValue);

    bool get(const char *pObj, std::string item, std::string key, int &nValue);
    bool get(const char *pObj, std::string item, std::string key, double &dValue);
    bool get(const char *pObj, std::string item, std::string key, std::string &value);
    bool get(const char *pObj, std::string item, std::string key, size_t nSize, char* pValue);

    /*新增:在 JSON 对象中删除一个字段*/
    void remove(Object *pObj, std::string key);
    
    /*更改整型、字符串*/
    void update(Object *pObj, std::string key, int nValue);
    void update(Object *pObj, std::string key, std::string value);
    
    std::string to_string(Object *pObj);

    /* 转成字符串，需要release */
    char *print(Object *pObj);
    void release(char *&p);

    /* 数组相关操作 */
    namespace Array {
        Object* init();
        Object* init(const int *nums, int nCount);
        Object* init(const float *nums, int nCount);
        Object* init(const double *nums, int nCount);
        Object* init(const char *const *strings, int nCount);
        int size(Object *pObj);
        Object *get(Object *pObj, int nIndex);
        bool get(Object *pObj, std::vector<int> &value);
        bool get(Object *pObj, std::vector<std::string> &value);
        void add(Object *pArr, Object *pItem);
        void add(Object *pArr, const char *string);
        void add(Object *pArr, const int nValue);
        void add(Object *pArr, const float fValue);
        void add(Object *pArr, std::string value);
    }
}


#endif



