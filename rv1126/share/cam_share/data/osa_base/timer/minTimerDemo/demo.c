

#include <stdio.h>
#include "minHeapTimer.h"


static void fun(void *arg)
{
   int *id = (int *) arg;
   printf("id:%d\n",*id);
}


static int s_time = 0;
static void fun_time(void *arg)
{
	int *id = (int *) arg;
	int time = 0;
	time = s_time;
	s_time = OS_getSysTimeInMsec();

	printf("id:%d time[%d]\n",*id,(s_time-time));
}

int main(int argc,char *argv[])
{
	int ret = 0;
	int timerID = 0;
	minTimer_t timerHandle;

	//init
	minHeap_timer_init(&timerHandle,1000);

#if 1

	int id1 = 101;
	timerID = minHeap_timer_add(&timerHandle,1000, fun, &id1,LIMIT_TIMER,1);//ms
	printf("id[%d]\n",timerID);
	int id2 = 102;
	minHeap_timer_add(&timerHandle,3000, fun, &id2,LIMIT_TIMER,1);
	int id3 = 103;
	timerID = minHeap_timer_add(&timerHandle,1000, fun, &id3,CYCLE_TIMER,0);

	sleep(5);
	minHeap_timer_remove(&timerHandle,timerID);

#endif


	int id4 = 101;
	s_time = OS_getSysTimeInMsec();
	timerID = minHeap_timer_add(&timerHandle,1*1000, fun_time, &id4,CYCLE_TIMER,1);//ms

	sleep(5);
	minHeap_timer_mod(&timerHandle,timerID,2000,CYCLE_TIMER,0);


	while(1)
	{
		sleep(60);
	}

	return 0;
}




