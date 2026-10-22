
#ifndef _CORE_SOURCE_MEDIA_CLOCK_INCLUDE_
#define _CORE_SOURCE_MEDIA_CLOCK_INCLUDE_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>


enum {
    CLOCK_SYNC_AUDIO_MASTER, /* default choice */
	CLOCK_SYNC_VIDEO_MASTER,
    CLOCK_SYNC_EXTERNAL_CLOCK /* synchronize to an external clock */
};

typedef struct _MEDIA_CLOCK_INFO_S {
    double pts;           //最后一次更新的媒体包的显示时间,单位s，
    double pts_drift;     //最后一次更新，pts-time的时钟相对差值
    double last_updated;	//最后一次更新时钟的系统时间（时间轴走动的时间）
    double speed;			//时钟速度控制，用于控制播放速度，默认是1，正常速率，<1,时钟变慢，>1时钟变快
    int paused;				//时钟暂停，返回pts，最后一次更新的pts值,1-暂停，0-正常
} mediaClock_S;

/*
 * 从系统开机到现在的微妙数
 * */
long long mdate();

double media_get_clock(mediaClock_S *c);

/*
 * 设置时钟
 * pts-->对应的单位是s
 * */
void media_set_clock(mediaClock_S *c, double pts);

void media_set_clock_speed(mediaClock_S *c, double speed);

void media_set_clock_pause(mediaClock_S *c, int pause);

void media_init_clock(mediaClock_S *c);

void media_sync_clock_to_slave(mediaClock_S *c, mediaClock_S *slave);

#ifdef __cplusplus
}
#endif
#endif //_CORE_SOURCE_MEDIA_CLOCK_INCLUDE_

