
#include "os_thr.h"
#include "stdio.h"
#define OS_SOK      0  ///< Status : OK
#define OS_EFAIL   -1  ///< Status : Generic error

int OS_thrCreate(OS_ThrHndl *hndl, OS_ThrEntryFunc entryFunc, uint32_t pri, uint32_t stackSize, void *prm)
{
	int status = OS_SOK;
	pthread_attr_t thread_attr;
	//struct sched_param schedprm;

	// initialize thread attributes structure
	status = pthread_attr_init(&thread_attr);

	if (status != OS_SOK)
	{
		printf("OS_thrCreate() - Could not initialize thread attributes\n");
		return status;
	}

	if (stackSize != OS_THR_STACK_SIZE_DEFAULT)
	{
		pthread_attr_setstacksize(&thread_attr, stackSize);
	}

	if (OS_DETACH == pri)
	{
		pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_DETACHED);
	}
	else
	{
		pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_JOINABLE);
	}
	/*
	status |= pthread_attr_setinheritsched(&thread_attr, PTHREAD_EXPLICIT_SCHED);
	status |= pthread_attr_setschedpolicy(&thread_attr, SCHED_FIFO);

	if (pri > OS_THR_PRI_MAX)
	{
		pri = OS_THR_PRI_MAX;
	}
	else if(pri < OS_THR_PRI_MIN)
	{
		pri = OS_THR_PRI_MIN;
	}

	schedprm.sched_priority = OS_THR_PRI_MAX;//pri;
	status |= pthread_attr_setschedparam(&thread_attr, &schedprm);
	*/
	if	(status	!=	OS_SOK)
	{
		printf("OS_thrCreate() - Could not initialize thread attributes\n");
		goto error_exit;
	}
	
	status = pthread_create(&(hndl->hndl), &thread_attr, entryFunc, prm);
	if	(status != OS_SOK)
	{
        hndl->hndl = 0;
		printf("OS_thrCreate() - Could not create thread [%d]\n", status);
	}

	error_exit:  
	pthread_attr_destroy(&thread_attr);

	return status;
}

int OS_thrJoin(OS_ThrHndl *hndl)
{
	int status = OS_SOK;
	void *returnVal;

	if (hndl != NULL)
	{
		if(hndl->hndl > 0)
		{
			status = pthread_join(hndl->hndl, &returnVal);
			hndl->hndl = 0;
		}
	}

	return status;    
}

int OS_thrDelete(OS_ThrHndl *hndl)
{
	int status = OS_SOK;

	if (hndl != NULL)
	{
	  	status = pthread_cancel(hndl->hndl); //会强制停止线程，但是用户在线程中申请的资源，不会自动释放
	  	status = OS_thrJoin(hndl);
	}

	return status;   	 
}

int OS_thrChangePri(OS_ThrHndl *hndl, uint32_t pri)
{
	int status = OS_SOK;
	struct sched_param schedprm;  
	if (hndl == NULL)
	{
		return status;
	}
	if(pri > (uint32_t)OS_THR_PRI_MAX)
	{
		pri = (uint32_t)OS_THR_PRI_MAX;
	}else
	{
		if(pri < (uint32_t)OS_THR_PRI_MIN)
		{
			pri = (uint32_t)OS_THR_PRI_MIN;
		}
	}

	schedprm.sched_priority = pri;  
	status |= pthread_setschedparam(hndl->hndl, SCHED_FIFO, &schedprm);
  
  	return status;
}

int OS_thrExit(void *returnVal)
{
	pthread_exit(returnVal);
	return OS_SOK;
}

