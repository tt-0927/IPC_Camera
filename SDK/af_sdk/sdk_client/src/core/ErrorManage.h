/**
 * @file ErrorManage.h
 * @author tianl (tianl@kfb.cn)
 * @date 2025-12-22
 * 
 * @brief 错误码处理
 */
#pragma once
#include <string>
#include <map>
#include <mutex>

#include "Singleton.h"

class CErrorManage : public CSingleton<CErrorManage>
{
    CErrorManage()
	{

	}
public:
	
	~CErrorManage()
	{

	}
	friend class CSingleton<CErrorManage>;

public:

    /**
     * @brief 设置最后错误码
     * @param [IN] code 错误码
     */
    void SetLastError(int code) 
	{
        lastErrorCode_ = code;
    }

    /**
     * @brief 获取最后错误码
     * @return 错误码
     */
    int GetLastError() 
	{
        return lastErrorCode_;
    }

    /**
     * @brief 获取错误描述（预留接口）
     * @return 错误描述字符串
     */
    const char* GetErrorMsg() 
	{
       
    }
private:
    
    static thread_local int lastErrorCode_;
};
