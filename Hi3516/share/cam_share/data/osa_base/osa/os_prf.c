

#include "os_prf.h"
#include "os_debug.h"

#ifdef OS_PRF_ENABLE

int OS_prfFps(OS_FrameHndl_t * hndl)
{
	if (NULL == hndl)
	{
		OS_printf("it is err OS_prfFps param=NULL\n");
		return OS_EFAIL;
	}
	
	if (hndl->flag != 1)
	{
		hndl->flag = 1;
		hndl->pre_time = OS_getCurTimeInMsec();
	}
	
	hndl->count++;
	hndl->cur_time = OS_getCurTimeInMsec();

	if(hndl->timePrintf == 0)
	{
	        hndl->timePrintf = 3000;    //默认是3s打印一次
	}

	if (hndl->cur_time - hndl->pre_time >= hndl->timePrintf)
	{
		hndl->pre_time = hndl->cur_time;
		OS_printf("%s=%d",hndl->msg, hndl->count);
		hndl->count = 0;
	}
	
	return OS_SOK;
}


int OS_prfBegin(OS_PrfHndl *hndl)
{
	hndl->curTime = OS_getCurTimeInMsec();
	return OS_SOK;
}

int OS_prfEnd(OS_PrfHndl *hndl, Uint32 value)
{
	hndl->curTime = OS_getCurTimeInMsec() - hndl->curTime;

	hndl->count++;

	hndl->totalTime += hndl->curTime;
		if(hndl->curTime > hndl->maxTime)
	hndl->maxTime = hndl->curTime;
		if(hndl->curTime < hndl->minTime)
	hndl->minTime = hndl->curTime;

	hndl->totalValue += value;
		if (value > hndl->maxValue)
	hndl->maxValue = value;
		if (value < hndl->minTime)
	hndl->minValue = value;

	return OS_SOK;
}

int OS_prfReset(OS_PrfHndl *hndl)
{
	hndl->count=0;
	hndl->totalTime=0;
	hndl->maxTime=0;
	hndl->maxValue=0;
	hndl->minTime=0;
	hndl->minValue=0;
	hndl->totalValue=0;
	  
	return OS_SOK;
}

int OS_prfPrint(OS_PrfHndl *hndl, char *name, Uint32 flags)
{
	OS_printf(" \n");
	OS_printf(" Profile Info  : %s \n", name);
	OS_printf(" ======================\n");

	if (flags == 0)
	{
		flags = OS_PRF_PRINT_DEFAULT;
	}
	
	if (flags & OS_PRF_PRINT_COUNT )
	{
		OS_printf(" Iterations    : %d \n", hndl->count);
	}

	if (flags & OS_PRF_PRINT_TIME )
	{
		OS_printf(" Avg Time (ms) : %9.2f \n", (float)hndl->totalTime/hndl->count);

		if (flags & OS_PRF_PRINT_MIN_MAX )
		{
	  	OS_printf(" Min Time (ms) : %d \n", hndl->minTime);
		}

		if (flags & OS_PRF_PRINT_MIN_MAX )
		{
	  		OS_printf(" Max Time (ms) : %d \n", hndl->maxTime);
		}
	}

	if (flags & OS_PRF_PRINT_VALUE )
	{

		OS_printf(" Avg Value     : %9.2f \n", (float)hndl->totalValue/hndl->count);
		OS_printf(" Avg Value/sec : %9.2f \n", (float)(hndl->totalValue*1000)/hndl->totalTime);

		if (flags & OS_PRF_PRINT_MIN_MAX)
		{
	  		OS_printf(" Min Value     : %d \n", hndl->minValue);
		}

		if (flags & OS_PRF_PRINT_MIN_MAX)
		{
	  		OS_printf(" Max Value     : %d \n", hndl->maxValue);
		}
	}
	OS_printf(" \n");

	return OS_SOK;
}




#endif
