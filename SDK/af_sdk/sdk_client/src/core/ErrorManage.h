/**
 * @file ErrorManage.h
 * @author tianl (tianl@kfb.cn)
 * @date 2026-07-28
 * @LastEditors  : qinjt@kfb.cn
 * @LastEditTime : 2026-07-28
 *
 * @brief ErrorManage 模块接口与类型定义
 * 功能说明：
 * 1. 声明 ErrorManage 模块对外接口和数据类型
 * 2. 定义模块依赖的常量、回调或辅助类型
 * 3. 为调用方提供明确且稳定的编译期契约
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
     * @author tianl (tianl@kfb.cn)
     * @brief 设置最后错误码
     * @param [in] code 错误码
     * @return 无返回值。
     */
    void SetLastError(int code)
	{
        s_nLastErrorCode = code;
    }

    /**
 * @author tianl (tianl@kfb.cn)
     * @brief 获取最后错误码
     * @return 错误码
     */
    int GetLastError()
	{
        return s_nLastErrorCode;
    }

    /**
 * @author tianl (tianl@kfb.cn)
     * @brief 获取错误描述（预留接口）
     * @return 错误描述字符串
     */
    const char* GetErrorMsg()
	{
        return "";
    }
private:

    static thread_local int s_nLastErrorCode;
};
