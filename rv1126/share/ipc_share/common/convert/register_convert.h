/***
 * @FilePath     : register_convert.h
 * @Author       : huangjunda
 * @Date         : 2025-07-08 14:09:48
 * @LastEditors  : huangjunda
 * @LastEditTime : 2025-07-08 14:13:08
 * @Description  : 注册配置转换处理
 */

#pragma once
#include "Json.h"
#include "register_define.h"

namespace Convert
{
	void deal(Json::Object *pRootJson, Register::RegisterInfo_S &stRegisterInfo, bool bOutStruct);
	void deal(Json::Object *pRootJson, Register::ConfigRegisterEg_S &stConfigRegEg, bool bOutStruct);
	void deal(Json::Object *pRootJson, Register::ActivationPasswdInfo_S &stActivationInfo, bool bOutStruct);
	void deal(Json::Object *pRootJson, Register::RegisterIp_S &stInfo, bool bOutStruct);
	void deal(Json::Object *pRootJson, Register::NetWorkInfo_S &stNetWorkInfo, bool bOutStruct);

}
