
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include "os.h"



#ifdef WIN32
#include <winsock.h>
#include <time.h>

#else
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <sys/time.h>   //gettimeofday()
#include <unistd.h>
#include <time.h>
#endif

Int32 os_msleep(Uint64 ms)
{
    Int32 reVal = 0;
#ifdef WIN32
    Sleep((unsigned long)ms);    //ms
#else
    usleep(ms*1000);        //us
#endif
    return reVal;
}


Uint32 OS_getCurTimeInMsec()
{
  static int isInit = FALSE;
  static Uint32 initTime=0;
  struct timeval tv;

  if(isInit==FALSE)
  {
      isInit = TRUE;

      if (gettimeofday(&tv, NULL) < 0)
        return 0;

      initTime = (Uint32)(tv.tv_sec * 1000u + tv.tv_usec/1000u);
  }

  if (gettimeofday(&tv, NULL) < 0)
    return 0;

  return (Uint32)(tv.tv_sec * 1000u + tv.tv_usec/1000u)-initTime;
}

Uint32 OS_getSysTimeInMsec()
{
    unsigned int msec = 0;
#ifndef WIN32
    struct timespec tp;
	clock_gettime(CLOCK_MONOTONIC, &tp);
	msec = tp.tv_sec;
	msec = msec * 1000 + tp.tv_nsec / 1000000;
#else
   msec = GetTickCount();   //ms
#endif
	return msec;
}

//获取从UTC1970-1-1 0:0:0到现在的毫秒数
Uint64 OS_getRealTimeInMsec()
{
    Uint64 msec = 0;
#ifndef WIN32
	struct timespec tp;
	clock_gettime(CLOCK_REALTIME, &tp);
	msec = tp.tv_sec;
	msec = msec * 1000 + tp.tv_nsec / 1000000;
#else

#endif
    return msec;
}




Int32 OS_getSys_DateTime(OS_DateTimeInfo_t *dtinfo)
{
	long ts;
	struct tm *ptm = NULL;

	ts = time(NULL);
	ptm = localtime((const time_t *)&ts);
	dtinfo->year = ptm->tm_year + 1900;
	dtinfo->month = ptm->tm_mon + 1;
	dtinfo->mday = ptm->tm_mday;
	dtinfo->hours = ptm->tm_hour;
	dtinfo->min = ptm->tm_min;
	dtinfo->sec = ptm->tm_sec;

	return 0;
}

#ifndef WIN32

void OS_waitMsecs(Uint32 msecs)
{
    struct timespec delayTime, remainTime;
    int ret;

    delayTime.tv_sec  = msecs/1000;
    delayTime.tv_nsec = (msecs%1000)*1000000;

    do
    {
        ret = nanosleep(&delayTime, &remainTime);
        if(ret < 0 && remainTime.tv_sec > 0 && remainTime.tv_nsec > 0)
        {
            /* restart for remaining time */
            delayTime = remainTime;
        }
        else
        {
            break;
        }
    } while(1);
}


int OS_attachSignalHandler(int sigId, void (*handler)(int ) )
{
  struct sigaction sigAction;

  /* insure a clean shutdown if user types ctrl-c */
  sigAction.sa_handler = handler;
  sigemptyset(&sigAction.sa_mask);
  sigAction.sa_flags = 0;
  sigaction(sigId, &sigAction, NULL);

  return OS_SOK;
}

#endif  //WIN32


//将十六进制数转为十进制数
static char xtod(char c)
{
  if (c>='0' && c<='9') return c-'0';
  if (c>='A' && c<='F') return c-'A'+10;
  if (c>='a' && c<='f') return c-'a'+10;
  return c=0;        // not Hex digit
}

//将十六进制转为整形
static int HextoDec(char *hex, int l)
{
  if (*hex==0)
    return(l);

  return HextoDec(hex+1, l*16+xtod(*hex)); // hex+1?
}

// hex string to integer
int os_xstrtoi(char *hex)
{
  return HextoDec(hex,0);
}


#ifdef WIN32
int gettimeofday(struct timeval *tp, void *tzp)
{
    time_t clock;
    struct tm tm;
    SYSTEMTIME wtm;
    GetLocalTime(&wtm);
    tm.tm_year   = wtm.wYear - 1900;
    tm.tm_mon   = wtm.wMonth - 1;
    tm.tm_mday   = wtm.wDay;
    tm.tm_hour   = wtm.wHour;
    tm.tm_min   = wtm.wMinute;
    tm.tm_sec   = wtm.wSecond;
    tm. tm_isdst  = -1;
    clock = mktime(&tm);
    tp->tv_sec = clock;
    tp->tv_usec = wtm.wMilliseconds * 1000;
    return (0);
}
#endif


///////////////////////////
#define Max 50
struct framerate{
    long framesec;
    char * str;
    int framecount;
};
static struct framerate FunRate[Max];
static int FunCount = 0;
static int modify = 0;
static  pthread_mutex_t modifyram_mutex = PTHREAD_MUTEX_INITIALIZER;
int frameRate(const char *Rate, long FlagTime)
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
                printf("\033[33m""%s %d\n""\033[0m",FunRate[nCmpare].str, retFrame);
                fflush(stdout);
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


int OS_forkApp(char* pCmd)
{
    if(pCmd == NULL)
    {
        return -1;
    }

    pid_t pid = fork();
    if (pid == -1) 
    {
        perror("fork");
        return -1;
    } 
    else if (pid == 0) 
    {
        // 子进程
        execl("/bin/sh", "sh", "-c", pCmd, NULL);
        perror("execl");
        return -1;
    } 
    // else 
    // {
    //     // 父进程
    //     int status;
    //     waitpid(pid, &status, 0);
    //     if (WIFEXITED(status)) 
    //     {
    //         dlog(LOG_DEBUG, "Command exited with status %d\n", WEXITSTATUS(status));
    //     } 
    //     else if (WIFSIGNALED(status))
    //      {
    //         printf("Command terminated by signal %d\n", WTERMSIG(status));
    //     }
    // }
    return 0;
}

/////////////////////////



