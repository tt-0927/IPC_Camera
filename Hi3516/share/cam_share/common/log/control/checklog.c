/*
*  File Name        : checklog.c
*  Created on       : 2021-09-03
*  Author           : yanzehui
*  description      : 检查日志文件有效期
*  Modify date      : 2021-09-03
*  Modifier Author  : yanzehui
*  description      : 用于详细说明此程序文件完成的主要功能
*/
#include "checklog.h"

static int MonthDays[] = {31 , 28 , 31 , 30 , 31 , 30 , 31 , 31 , 30 , 31 , 30 , 31} ;

typedef struct
{
    int nYear;      /*年*/
    int nMonth;     /*月*/
    int nDay;       /*日*/
    time_t nTotalSec;  /*1970年以来的秒*/
}LogOperationDate_S;

/* 判断是否是闰年 */
static char IsLeapYear(int nYear)
{
    if((nYear % 4 == 0 && nYear % 100 != 0) || nYear % 400 == 0)
    {
        return 1 ;
    }
    else
    {
        return 0 ;
    }

}

static time_t datetime2sec(LogOperationDate_S stDate)
{
    struct tm stTime;
    memset(&stTime,0,sizeof(stTime));
    stTime.tm_year = stDate.nYear -1900;
    stTime.tm_mon = stDate.nMonth -1;
    stTime.tm_mday = stDate.nDay;
    stTime.tm_hour = 0;
    stTime.tm_min = 0;
    stTime.tm_sec = 0;
    return mktime(&stTime);
}


/* 获取两个月份之间的天数 例如 3-4间的天数是0,3-5间的天数是1个月的天数*/
static int GetMidMonthDays(int nMonth1 , int nMonth2, int nYear)
{
    int nDays = 0 ;
    int i = 0;

    for(i = nMonth1 + 1 ; i < nMonth2 ; i++)
    {
        nDays += MonthDays[i-1] ;
        if(i == 2)
        {
            if(IsLeapYear(nYear))
            {
                nDays++ ;
            }
        }
    }
    return nDays ;
}

/* 获取相同年份间的天数 */
static int GetSameYearDays(LogOperationDate_S stDate1 , LogOperationDate_S stDate2)
{
    int nDays ;
    /* 如果月份相同 */
    if(stDate1.nMonth == stDate2.nMonth)
    {
        nDays =abs(stDate2.nDay - stDate1.nDay)  ;
    }
    else
    {
        /* 计算第一个日期的月份剩下的天数 */
        nDays = abs(MonthDays[stDate1.nMonth - 1] - stDate1.nDay) ;
        if(stDate1.nMonth == 2)
        {
            if(IsLeapYear(stDate1.nYear))
            {
                nDays++ ;
            }
        }

        if((stDate1.nMonth + 1) != stDate2.nMonth)
        {
            /* 计算中间月份的天数 */
            nDays += GetMidMonthDays(stDate1.nMonth , stDate2.nMonth, stDate1.nYear);
        }
        nDays += stDate2.nDay ;
    }
    return nDays;
}

/* 获取两个年份之间的天数，例如 2020-2021间的天数是0,2020-2022间的天数是1年的天数 */
static int GetMidYearDays(int nYear1,int nYear2)
{
    int nDays=0;
    int i=0;
    for (i = nYear1 + 1 ; i < nYear2 ; i++)
    {
        if (IsLeapYear(i-1))
        {
            nDays+=366;
        }
        else
        {
            nDays+=365;
        }

    }
    return nDays;
}

/* 获取不同日期之间的天数 */
static int GetDifferentYearDays(LogOperationDate_S stDate1 , LogOperationDate_S stDate2)
{
    int nDays;
    /* 如果年份相等，调用相同年份函数 */
    if (stDate1.nYear == stDate2.nYear)
    {
        nDays = GetSameYearDays(stDate1, stDate2);
    }
    else
    {

        nDays = MonthDays[stDate1.nMonth + 1] - stDate1.nDay;

        /* 如果不止相差一年 */
        if((stDate1.nYear + 1 ) != stDate2.nYear)
        {
            nDays += GetMidYearDays(stDate1.nYear, stDate2.nYear);
        }

        nDays += GetMidMonthDays(stDate1.nMonth-1, 13, stDate1.nYear);

        if(stDate2.nMonth == 1)
        {
            nDays += stDate2.nDay;
        }
        else
        {
            nDays += GetMidMonthDays(1, stDate2.nMonth, stDate2.nYear);
            nDays += stDate2.nDay;
        }
    }
    return nDays;
}

/* 获取log文件名中的日期 */
static int getDate(char *pDateStr,LogOperationDate_S *pstDate)
{
    char *pTemp = strrchr(pDateStr,'_');
    if(pTemp == NULL)
    {
        return -1;
    }

    /*确保字符串中有两个‘-’*/
    char *pTmp1 = strrchr(pTemp,'-');
    if(pTmp1 == NULL)
    {
        return -1;
    }
    char *pTmp2 = strrchr(pTmp1,'-');
    if(pTmp2 == NULL)
    {
        return -1;
    }

    sscanf(pTemp,"_%d-%d-%d.log",&pstDate->nYear,&pstDate->nMonth,&pstDate->nDay);
    if(pstDate->nYear <= 0)
    {
        return -1;
    }
    if(pstDate->nMonth <= 0 || pstDate->nMonth > 12)
    {
        return -1;
    }
    if(pstDate->nDay <= 0 || pstDate->nDay > 31)
    {
        return -1;
    }

    /*获取总时间*/
    pstDate->nTotalSec = datetime2sec(*pstDate);

    return 0;
}

/* 删除过期日志 */
int removeLogFile(char* pLogPath, int nSaveDays)
{
	printf("removeLogFile\n");
    if(pLogPath == NULL)
    {
        return -1;
    }

    if(nSaveDays <= 0)
    {
        return -2;
    }

    int nLogTotal = 0;


    /* 获取当前日期 */
    struct tm *pTm = NULL;
    time_t timep;
    time(&timep);
    pTm = gmtime(&timep);
    LogOperationDate_S stCurrentDate;
    stCurrentDate.nYear  = pTm->tm_year + 1900;
    stCurrentDate.nMonth = pTm->tm_mon + 1;
    stCurrentDate.nDay   = pTm->tm_mday;
    stCurrentDate.nTotalSec = datetime2sec(stCurrentDate);

    /*目录操作*/
    DIR              *pDir = NULL ;
    struct dirent    *pDirEnt = NULL ;

    /*临时变量*/
    LogOperationDate_S stDate = {0};

    /*最新的时间*/
    LogOperationDate_S stNewestDate = {0};

    /*过期时间的总秒数*/
    time_t nSaveSec = nSaveDays * 24 * 60 * 60;


# if 0
    /* 打开目录句柄 */
    pDir=opendir(pLogPath);
    
    /* 遍历目录 ,找到最新的时间*/
    while((pDirEnt=readdir(pDir))!=NULL)
    {
    	/* 一般文件 */
        if(pDirEnt->d_type & DT_REG)
        {
            if (strstr(pDirEnt->d_name,".log"))
            {
                nLogTotal++;
                int nRet = getDate(pDirEnt->d_name,&stDate);
                if(nRet != 0)
                {
                    printf("该日志不用过期处理\n");
                    continue;
                }

                /*判断是否为最新*/
                if(stNewestDate.nTotalSec < stDate.nTotalSec)
                {
                    stNewestDate = stDate;
                }
            }
        }
    }
    closedir(pDir);
# else
    stNewestDate = stCurrentDate;
#endif

    /* 打开目录句柄 */
    pDir=opendir(pLogPath);
    
    /* 遍历目录 */
    while((pDirEnt=readdir(pDir))!=NULL)
    {
    	/* 一般文件 */
        if(pDirEnt->d_type & DT_REG)
        {
            if (strstr(pDirEnt->d_name,".log"))
            {
                int nRet = getDate(pDirEnt->d_name,&stDate);
                if(nRet != 0)
                {
                    printf("该日志不用过期处理\n");
                    continue;
                }

                #if 0
                /* 判断日志是否过期 */
                if(GetDifferentYearDays(stDate, stNewestDate) >= nSaveDays)
                {
                    /* 拼接路径 */
                    char filePath[1024] = {0};
                    sprintf(filePath,"%s/%s",pLogPath,pDirEnt->d_name);
                    printf("delete log file: %s\n", filePath);

                    /* 过期删除日志 */
                    remove(filePath);
                }
                #elif 0  /*全部不符合要求的都删除*/
                /* 判断日志是否过期 */
                if(stDate.nTotalSec > stNewestDate.nTotalSec)
                {
                    if(stDate.nTotalSec - stNewestDate.nTotalSec >= nSaveSec)
                    {
                        /* 拼接路径 */
                        char filePath[1024] = {0};
                        sprintf(filePath,"%s/%s",pLogPath,pDirEnt->d_name);
                        printf("delete log file: %s\n", filePath);

                        /* 过期删除日志 */
                        remove(filePath);
                    }
                }
                else if(stDate.nTotalSec < stNewestDate.nTotalSec)
                {
                    if(stNewestDate.nTotalSec - stDate.nTotalSec >= nSaveSec)
                    {
                        /* 拼接路径 */
                        char filePath[1024] = {0};
                        sprintf(filePath,"%s/%s",pLogPath,pDirEnt->d_name);
                        printf("delete log file: %s\n", filePath);

                        /* 过期删除日志 */
                        remove(filePath);
                    }
                }
                #else
                /*不删除未来时的*/
                if(stDate.nTotalSec < stNewestDate.nTotalSec)
                {
                    if(stNewestDate.nTotalSec - stDate.nTotalSec >= nSaveSec)
                    {
                        /* 拼接路径 */
                        char filePath[1024] = {0};
                        sprintf(filePath,"%s/%s",pLogPath,pDirEnt->d_name);
                        printf("delete log file: %s\n", filePath);

                        /* 过期删除日志 */
                        remove(filePath);
                    }
                }
                #endif
            }
        }
    }
    closedir(pDir);
    
    return 0;
}