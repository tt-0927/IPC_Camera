/*** 
 * @FilePath     : register_convert.cpp
 * @Author       : huangjunda
 * @Date         : 2025-07-08 14:09:48
 * @LastEditors  : huangjunda
 * @LastEditTime : 2025-07-17 20:21:39
 * @Description  : 注册配置转换处理
 */

#include "register_convert.h"
#include "convert.h" /* 这个要放在RegisterConvert.h的后面 */

void Convert::deal(Json::Object* pRootJson, Register::RegisterInfo_S &stRegisterInfo, bool bOutStruct)
{
	if (!pRootJson)
	{
		return;
	}

	Convert::CConvert convert(bOutStruct);
	convert.field(pRootJson, "MachinSn", stRegisterInfo.strMachinSn);
	convert.field(pRootJson, "StartTime", stRegisterInfo.strStartTime);
	convert.field(pRootJson, "LifeTimer", stRegisterInfo.lnLifeTimer);
	convert.field(pRootJson, "ActionTime", (int&)stRegisterInfo.enActionTime);

}

void Convert::deal(Json::Object* pRootJson, Register::ConfigRegisterEg_S &stConfigRegEg, bool bOutStruct)
{
	if (!pRootJson)
	{
		return;
	}

	Convert::CConvert convert(bOutStruct);
	convert.field(pRootJson, "RegisterEg", stConfigRegEg.strRegisterEg);
}

void Convert::deal(Json::Object* pRootJson, Register::ActivationPasswdInfo_S &stActivationInfo, bool bOutStruct)
{
	if (!pRootJson)
	{
		return;
	}

	Convert::CConvert convert(bOutStruct);
	convert.field(pRootJson, "EnActivated", stActivationInfo.bEnActivated);
	convert.field(pRootJson, "User", stActivationInfo.strUser);
	convert.field(pRootJson, "UserPwd", stActivationInfo.strUserPwd);
	convert.field(pRootJson, "Safety", stActivationInfo.nSafety);
	convert.field(pRootJson, "IpcPwd", stActivationInfo.strIpcPwd);
	convert.field(pRootJson, "SameDevice", stActivationInfo.bEnSameDevice);


}

void Convert::deal(Json::Object* pRootJson, Register::RegisterIp_S &stInfo, bool bOutStruct)
{
	if (!pRootJson)
	{
		return;
	}
	Convert::CConvert convert(bOutStruct);
	convert.field(pRootJson, "EnableDhcp", stInfo.bEnableDhcp);
	convert.field(pRootJson, "Ipv4Ip", stInfo.strIpv4Ip);
	convert.field(pRootJson, "Ipv4Mask", stInfo.strIpv4Mask);
	convert.field(pRootJson, "Ipv4Gateway", stInfo.strIpv4Gateway);
	
}

void Convert::deal(Json::Object* pRootJson, Register::NetWorkInfo_S &stNetWorkInfo, bool bOutStruct)
{
	if (!pRootJson)
	{
		return;
	}

	Convert::CConvert convert(bOutStruct);
	convert.structure(pRootJson,"NetInfos", stNetWorkInfo.stRegIp);
	convert.field(pRootJson, "AutoDns", stNetWorkInfo.bEnAutoDns);
	convert.field(pRootJson, "DnsMain", stNetWorkInfo.strDnsMain);
	convert.field(pRootJson, "DnsStandby", stNetWorkInfo.strDnsStandby);
}

