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

    void SetLastError(int code) 
	{
        lastErrorCode_ = code;
    }

    int GetLastError() 
	{
        return lastErrorCode_;
    }

    // 获取错误描述
    const char* GetErrorMsg() 
	{
       
    }
private:
    
    static thread_local int lastErrorCode_;
};
