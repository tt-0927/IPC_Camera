


#include "ae.h"

#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <sys/time.h>
#include <string.h>
#include <stdlib.h>

#include <pthread.h>
#define MAXFD 5

void file_cb(evEventBase *base, int fd, void *user, int events)
{
	static time_t preTime = 0;
	time_t now = time(NULL);
	printf("IO is timecal[%ld] events[%d]\n",(now - preTime),events);
	preTime = now;
	//printf("I'm IO_cb ,here [EventLoop: %p],[fd : %d],[data: %p],[mask: %d] \n",base,fd,user,events);
}

int time_cb(evEventBase *l,void *data)
{
	static time_t preTime = 0;
	time_t now = time(NULL);
	printf("time is timecal[%ld]\n",(now - preTime));
	preTime = now;
	//printf("I'm time_cb,here [EventLoop: %p],[data: %p] \n",l,data);
	return 1*1000;
}

int time_heart(evEventBase *l,void *data)
{
	static time_t preTime = 0;
	time_t now = time(NULL);
	printf("==========>heart time is timecal[%ld]\n",(now - preTime));
	preTime = now;
	//printf("I'm time_cb,here [EventLoop: %p],[data: %p] \n",l,data);
	return 1000;
}

void *event_loop_thr(void* argv)
{
	int res = 0;
	evEventBase *l = (evEventBase*)argv;
	struct timeval time;
	time.tv_sec = 1;
	time.tv_usec = 0;

	evTimeEvent* timeHandle = evEvent_addTime(l,&time,time_cb,NULL);
	printf("create time event is ok? [%d]\n",!res);

	sleep(5);
	printf("==================>>>>>>>>>>del event!!!\n");
	evEvent_delIO(l,STDIN_FILENO,EV_READABLE);

	sleep(5);
	printf("==================>>>>>>>>>>del time!!!\n");
	evEvent_delTime(l,timeHandle);

	sleep(3);
	printf("add time event start!!!\n");
	timeHandle = evEvent_addTime(l,&time,time_cb,NULL);
	printf("add time event end!!\n");

	while(1)
	{
		sleep(10);
	}

	return NULL;
}



int main(int argc,char *argv[])
{
	int res = 0;
	evEventBase *l = NULL;
	char *msg = "Here std say:";
	char *user_data = malloc(50*sizeof(char));
	if(! user_data)
	{
		assert( ("user_data malloc error",user_data) );
	}
	memset(user_data,'\0',50);
	memcpy(user_data,msg,sizeof(msg));

	l = evEvent_init(MAXFD);


	struct timeval time;
	time.tv_sec = 1;
	time.tv_usec = 0;
	evTimeEvent* timeHandle = evEvent_addTime(l,&time,time_heart,NULL);


#if 1
	pthread_t hndl;
	res = pthread_create(&(hndl), NULL, event_loop_thr, (void*)l);
	if	(res < 0)
	{
		printf("OS_thrCreate() - Could not create thread [%d]\n", res);
		return -1;
	}

	sleep(1);
#endif

	struct timeval tv;
	tv.tv_sec = 0;
	tv.tv_usec = 500*1000;
	res = evEvent_addIO(l,STDIN_FILENO,EV_READABLE | EV_TIMEOUT,&tv,file_cb,user_data);
	printf("create file event is ok? [%d]\n",res);


	printf("start event loop!!! name[%s]\n",evEvent_getApiName());
	evEvent_loop(l);

	printf("end========================\n\n\n\n\n\n");

	while(1)
	{
		sleep(60);
	}

	puts("Everything is ok !!!\n");
	return 0;
}


