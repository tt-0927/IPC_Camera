/*
 * @Descripttion: 时间公共库
 * @version: 
 * @Author: fanghongshen
 * @Date: 2021-10-22 15:56:55
 * @LastEditors: huangjunda
 * @LastEditTime: 2024-08-13 14:02:40
 */

#include "get_time.h"

int get_time_char(char *pTime,int nLen)
{
    /* 获取当前日期时间 */
	struct tm *pTm = NULL;
	time_t timep;
	time(&timep);
	pTm = localtime(&timep);
	if(pTm == NULL)
	{
		return -1 ;
	}
	UpTime_S stCurrentDate;
	stCurrentDate.nYears  = pTm->tm_year + 1900;
	stCurrentDate.nMonths = pTm->tm_mon + 1;
	stCurrentDate.nDays   = pTm->tm_mday;
	stCurrentDate.nHours = pTm->tm_hour;
	stCurrentDate.nMinutes = pTm->tm_min;
	stCurrentDate.nSeconds = pTm->tm_sec;

	/*拼接时间字符串*/
	char aDateTime[64] = {0};
	sprintf(aDateTime,"%04d-%02d-%02d %02d:%02d:%02d",\
			stCurrentDate.nYears,stCurrentDate.nMonths,stCurrentDate.nDays,\
			stCurrentDate.nHours,stCurrentDate.nMinutes,stCurrentDate.nSeconds);


    snprintf(pTime,nLen,"%s",aDateTime);

    return 0;
}

int get_time_T_char(char *pTime,int nLen)
{
    /* 获取当前日期时间 */
	struct tm *pTm = NULL;
	time_t timep;
	time(&timep);
	pTm = localtime(&timep);
	if(pTm == NULL)
	{
		return -1 ;
	}
	UpTime_S stCurrentDate;
	stCurrentDate.nYears  = pTm->tm_year + 1900;
	stCurrentDate.nMonths = pTm->tm_mon + 1;
	stCurrentDate.nDays   = pTm->tm_mday;
	stCurrentDate.nHours = pTm->tm_hour;
	stCurrentDate.nMinutes = pTm->tm_min;
	stCurrentDate.nSeconds = pTm->tm_sec;

	/*拼接时间字符串*/
	char aDateTime[64] = {0};
	sprintf(aDateTime,"%04d-%02d-%02dT%02d:%02d:%02d",\
			stCurrentDate.nYears,stCurrentDate.nMonths,stCurrentDate.nDays,\
			stCurrentDate.nHours,stCurrentDate.nMinutes,stCurrentDate.nSeconds);

    snprintf(pTime,nLen,"%s",aDateTime);

    return 0;
}

double get_time_ms()
{
	struct timeval tv;
	double ret;
	int err;

	err = gettimeofday(&tv, 0);
	if (err == 0)
	{
		ret = tv.tv_sec * 1000.0 + tv.tv_usec * 1.0 / 1000;
		// ret maybe is NAN
		if (ret != ret)
		{
			printf("NAN(%.0f,sec=%.d,usec=%.d).\n", ret, (int)tv.tv_sec, (int)tv.tv_usec);
			ret = 0;
		}
	}
	else
	{
		perror(__FUNCTION__);
		ret = 0;
	}
	return ret;
}

/**
 * @description : 转换运行时间戳为字符串
 * @author      : huangjunda
 * @param        {long} m_sec
 * @param        {SysUptime_S} *stUptimeInfo
 * @return       {*}
 */
void sys_calculate_uptime(long m_sec, SysUptime_S *stUptimeInfo)
{
    // 总运行时间（秒）
    stUptimeInfo->lnUptime = m_sec;

    // 天
    int nDays = m_sec / (24 * 3600);
    stUptimeInfo->upTime.nDays = nDays;

    // 小时
    int nHours = (m_sec % (24 * 3600)) / 3600;
    stUptimeInfo->upTime.nHours = nHours;

    // 分钟
    int nMinutes = (m_sec % 3600) / 60;
    stUptimeInfo->upTime.nMinutes = nMinutes;

    // 秒
    int nSeconds = m_sec % 60;
    stUptimeInfo->upTime.nSeconds = nSeconds;

    // 构建字符串表示
    if (nDays >= 1)
	{
        snprintf(stUptimeInfo->achUptime, sizeof(stUptimeInfo->achUptime), "%d天 %02d:%02d:%02d",
                 nDays, nHours, nMinutes, nSeconds);
    }
	else
	{
        snprintf(stUptimeInfo->achUptime, sizeof(stUptimeInfo->achUptime), "%02d:%02d:%02d",
                 nHours, nMinutes, nSeconds);
    }
}