
#include <stdio.h>
#include <math.h>
#include <time.h>
#include "media_clock.h"

#ifdef WIN32
#include <winsock.h>
#include <time.h>
#endif

#define CLOK_NOSYNC_THRESHOLD 10.0


long long mdate()
{
    long long usec = 0;
#ifndef WIN32
    struct timespec ts;
    if (clock_gettime (CLOCK_MONOTONIC, &ts) != 0){
        abort ();
    }
	clock_gettime(CLOCK_MONOTONIC, &ts);
    usec = (1000000 * ts.tv_sec) + (ts.tv_nsec / 1000);
#else
    usec = GetTickCount()*1000;   //ms --> us
#endif
    return usec;
}

double media_get_clock(mediaClock_S *c)
{
    if (c->paused)
    {
        return c->pts;
    } else
    {
        double time = mdate() / 1000000.0;
        return c->pts_drift + time - (time - c->last_updated) * (1.0 - c->speed);
    }
}

static void media_set_clock_at(mediaClock_S *c, double pts, double time)
{
    c->pts = pts;			/* 有效帧对应的pts，以音频为例，此时对应的就是第一帧播放时，音频clock的时间值 */
    c->last_updated = time;	/* 有效帧播放时的系统时间 */
    /* 当前时刻，外部时钟的时间值（pts）与系统时间值之差
	 * 一定注意是当前时刻，后续时间点计算都是根据此值作为基准来计算。
	 * 每隔10秒才会更新这个基准时间。
	 */
    c->pts_drift = c->pts - time;
}

void media_set_clock(mediaClock_S *c, double pts)
{
    double time = mdate() / 1000000.0;
    media_set_clock_at(c, pts, time);
}

void media_set_clock_speed(mediaClock_S *c, double speed)
{
	media_set_clock(c, media_get_clock(c));
    c->speed = speed;
}

void media_set_clock_pause(mediaClock_S *c, int pause)
{
	c->paused = pause;
}

void media_init_clock(mediaClock_S *c)
{
    c->speed = 1.0;
    c->paused = 0;
    media_set_clock(c, NAN);
}

/*
 * c-->外部时钟的句柄，
 * slave--->外部时钟从属audio/video的时钟句柄
 * */
void media_sync_clock_to_slave(mediaClock_S *c, mediaClock_S *slave)
{
    double clock = media_get_clock(c);
    double slave_clock = media_get_clock(slave);
    /*
     *  更新外部时钟情况如下：
     * 	1>外部时钟pts非法，且从属时钟（音频/视频）的pts有效时更新，所以第一帧肯定同步视频/音频时钟。
        2>外部时钟pts与从属时钟的时间差值超过AV_NOSYNC_THRESHOLD（10秒），则对外部时钟进行更新。

        另外发现进行校正的必要条件之一是!isnan(diff)，也就是diff值是合法数值，
        这在第一帧的音频或视频显示前是不成立的，也就无需做同步校正。
        在第一帧视频或音频显示后，此时extclk得到对时，接下来就可以进入正常的同步“循环”了。
     */
    if (!isnan(slave_clock) && (isnan(clock) || fabs(clock - slave_clock) > CLOK_NOSYNC_THRESHOLD))
    {
        media_set_clock(c, slave_clock);
    }
}




