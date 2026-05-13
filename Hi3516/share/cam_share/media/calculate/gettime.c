#include "gettime.h"
#include <stdio.h>
#include<stdlib.h>
#define Change 1000000
#define False 0
#ifndef True
#define True 1
#endif
#include<stdio.h>
#include<string.h>
#include<sys/time.h>
#include <pthread.h>
#include "dlog.h"
#define TIME_MAX 32

typedef struct TimeGet
{
        struct timeval BeginTime;
        char *BeginStr;
        long sectime;
        int framecount;
        int display;
        int first;
}TimeGetFrameRate_t;
struct TimeGet Time[TIME_MAX];
static unsigned long g_TimeCount = 0;

#if 0
static int TurnValue(struct TimeGet *s1, struct TimeGet *s2)
{
	struct TimeGet s3;
	s3 = *s1;
	*s1 = *s2;
	*s2 = s3;
	return True;
}
#endif
int csTimeBegin(const char *str)
{
	return 0;
	struct TimeGet Getime;

	unsigned long nCmpare;




	gettimeofday (&Getime.BeginTime, NULL);


	for(nCmpare=0; nCmpare<g_TimeCount; nCmpare++)
	{

		//nslog(NS_DEBUG, "str: %s*******%s %d\n" ,str ,Time[nCmpare].BeginStr, nCmpare);
		if(strcmp(str, Time[nCmpare].BeginStr))
			continue;
		else
			break;
	}
	if(nCmpare == g_TimeCount)
	{
		if(TIME_MAX -1 <= g_TimeCount)
		{
			return -1;
		}
		Getime.BeginStr = (char *)malloc(strlen(str) + 1);
		if(Getime.BeginStr == NULL)
		{
			return -1;
		}
		strncpy(Getime.BeginStr, str, strlen(str) + 1);
		Time[nCmpare] = Getime;
		g_TimeCount++;
	}
	Time[nCmpare].BeginTime = Getime.BeginTime;

	return True;
}
unsigned long csTimeEnd(const char *str)
{
	return 0;
	struct TimeGet Begin;
	unsigned long nCmpare;
	//unsigned long nDelete;

	for(nCmpare=0; nCmpare<g_TimeCount; nCmpare++)
	{
		Begin = Time[nCmpare];

		if(strcmp(str, Begin.BeginStr))
			continue;
		else
			break;
	}

	if(nCmpare == g_TimeCount)
	{	
		printf("no match BeginTime\n");
		return False;
	}

	struct timeval EndTime;
	gettimeofday(&EndTime, NULL);
	unsigned long RunTime = (EndTime.tv_sec - Begin.BeginTime.tv_sec) * Change + (EndTime.tv_usec - Begin.BeginTime.tv_usec);

	//printf("%s : %ld usec\n", str, RunTime);
	return  RunTime;

}
int FunCallBackTime(const char *str)
{
	static int nChoose = 1;
	int RunTime = 0;
	if(nChoose != 1)
	   {
		RunTime = csTimeEnd(str);
	   }
	csTimeBegin(str);
	nChoose++;
	if(nChoose == 10000)
	nChoose = 2;
	return RunTime;
}

struct framerate{
	long framesec;
    char * str;
	int framecount; 

};
static struct framerate FunRate[Max];
static int FunCount = 0;

static int modify = 0;

static  pthread_mutex_t modifyram_mutex = PTHREAD_MUTEX_INITIALIZER;
int FrameRate(const char *Rate, long FlagTime)
{
	int retFrame = 1;
	int nCmpare = 0;
	struct timeval FrameTime = {0};

	pthread_mutex_lock(&modifyram_mutex);
	gettimeofday (&FrameTime, NULL);


	for(nCmpare=0; nCmpare<FunCount ; nCmpare++)
		{
			if(strcmp(Rate, FunRate[nCmpare].str))
			{
				continue;
			}
			else
			{
				if(FrameTime.tv_sec - FunRate[nCmpare].framesec < FlagTime)
					{
					
						(FunRate[nCmpare].framecount)++;
						pthread_mutex_unlock(&modifyram_mutex);
						return -1; 
					}
				else
					{	
						retFrame = FunRate[nCmpare].framecount;
						FunRate[nCmpare].framecount = 1;
						FunRate[nCmpare].framesec = FrameTime.tv_sec;
						// dlog(LOG_DEBUG, "\033[33m""%s %d\n""\033[0m",FunRate[nCmpare].str, retFrame);
						pthread_mutex_unlock(&modifyram_mutex);
						return retFrame;
					}
				  
			}
			
		}
	if(nCmpare == FunCount && FunCount < Max)
	{
		printf("FunCount:%d\n", FunCount);
		nCmpare = FunCount;

		modify = 1;
		FunRate[nCmpare].framecount = 1;
		FunRate[nCmpare].framesec = FrameTime.tv_sec;
		FunRate[nCmpare].str = (char *)malloc(strlen(Rate) + 1);
		if(FunRate[nCmpare].str == NULL)
		{
			pthread_mutex_unlock(&modifyram_mutex);
			return -1;
		}
		memcpy(FunRate[nCmpare].str, Rate, strlen(Rate) + 1);
		FunCount++;
		modify = 0;
		pthread_mutex_unlock(&modifyram_mutex);
	}
	return -1;
}

typedef void* COUNT_FRAME_HANDLE;
COUNT_FRAME_HANDLE count_framerate_init(const char *str,long sectime ,int display)
{

	TimeGetFrameRate_t* begin = (TimeGetFrameRate_t*)malloc(sizeof(TimeGetFrameRate_t));
	int nLen = strlen(str) + 1;

	memset(begin, 0, sizeof(TimeGetFrameRate_t));
	begin->BeginStr = (char*)malloc(nLen);
	memcpy(begin->BeginStr, str, nLen);
//	strncpy(begin->BeginStr, str, nLen);
	begin->sectime = sectime;
	begin->display = display;
	return begin;
}

int count_framerate_deal(COUNT_FRAME_HANDLE handle)
{
	struct timeval curTime;
	TimeGetFrameRate_t* beginhandle = (TimeGetFrameRate_t*)handle;
	long countTime = 0;
	int frameRateNum = 0;
	if(beginhandle == NULL)
	{
		printf("count_framerate_deal handle is NULL\n");
		return -1;
	}
	if(beginhandle->BeginTime.tv_sec == 0)
	{
		(beginhandle->framecount)++;
		gettimeofday (&beginhandle->BeginTime, NULL);
	}
	countTime = beginhandle->sectime;
	gettimeofday (&curTime, NULL);
	if(curTime.tv_sec - beginhandle->BeginTime.tv_sec < countTime)
	{

		(beginhandle->framecount)++;
	}
	else
	{
		frameRateNum = beginhandle->framecount;
		beginhandle->framecount = 1;
		 beginhandle->BeginTime.tv_sec = curTime.tv_sec;
		 if(beginhandle->display == 1)
		 {
			// printf("\033[33m""newcount %s %d\n""\033[0m",beginhandle->BeginStr, frameRateNum);
		 }
		 //第一次统计不算
		 if( beginhandle->first == 1)
		 {
				return frameRateNum;
		 }
		 else
		 {
			 beginhandle->first = 1;
			 return 0;
		 }

	}
	return 0;
}


int timeBegin(const char *str, void** handle)
{
	TimeGetFrameRate_t* begin = NULL;
	int nLen = 0;
	if(handle == NULL)
	{
		printf( "timeBegin param is error\n");
		return -1;
	}
	if(*handle == NULL)
	{
		begin = (TimeGetFrameRate_t*)malloc(sizeof(TimeGetFrameRate_t));
		nLen  = strlen(str) + 1;
		memset(begin, 0, sizeof(TimeGetFrameRate_t));
		begin->BeginStr = (char*)malloc(nLen);
		memcpy(begin->BeginStr, str, nLen);
//		strncpy(begin->BeginStr, str, nLen);
		gettimeofday (&begin->BeginTime, NULL);
		*handle = begin;
	}
	else
	{
		begin = *handle;
		gettimeofday (&begin->BeginTime, NULL);
	}

	return 0;
}

unsigned long timeEnd( void** handle)
{
	struct timeval EndTime;
	unsigned long RunTime = 0;
	TimeGetFrameRate_t* Begin = NULL;
	gettimeofday(&EndTime, NULL);
	if(handle == NULL)
	{
		printf("timeEnd param is error\n");
		return -1;
	}
	Begin = (TimeGetFrameRate_t* )(*handle);
	if(*handle == NULL)
	{
		return -1;
	}
	RunTime = (EndTime.tv_sec - Begin->BeginTime.tv_sec) * Change + (EndTime.tv_usec - Begin->BeginTime.tv_usec);
	return RunTime;
}
int free_timecountHandle(void** handle)
{
	TimeGetFrameRate_t* Begin = NULL;
	if(handle == NULL)
	{
		return -1;
	}
	Begin = (TimeGetFrameRate_t* )(*handle);
	if(*handle == NULL)
	{
		return -1;
	}
	free(Begin->BeginStr);
	free(Begin);
	*handle = NULL;
	return 0;
}



