
/*
 * 毫秒定时器  采用多级时间轮方式  借鉴linux内核中的实现
 * 支持的范围为1 ~  2^32 毫秒(大约有49天)
 * 若设置的定时器超过最大值 则按最大值设置定时器
 *
 **/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/time.h>

#include "timeWheer.h"



static void internal_add_timer(struct tvec_base *base, struct timer_list *timer)
{
    struct list_head *vec = NULL;
    unsigned long expires = timer->expires;
    unsigned long idx = expires - base->current_index;

    if ( idx < TVR_SIZE )	//第一级时间轮
    {
        int i = expires & TVR_MASK;	//确定第一级时间轮的具体齿轮
        vec = base->tv1.vec + i;	//然后获取对应齿轮的链表头
    }
    else if( idx < (1 << (TVR_BITS + TVN_BITS)) )	//第二级时间轮
    {
        int i = (expires >> TVR_BITS) & TVN_MASK;
        vec = base->tv2.vec + i;
    }
    else if( idx < (1 << (TVR_BITS + 2 * TVN_BITS)) )	//第三级时间轮
    {
        int i = (expires >> (TVR_BITS + TVN_BITS)) & TVN_MASK;
        vec = base->tv3.vec + i;
    }
    else if( idx < (1 << (TVR_BITS + 3 * TVN_BITS)) )	//第四级时间轮
    {
        int i = (expires >> (TVR_BITS + 2 * TVN_BITS)) & TVN_MASK;
        vec = base->tv4.vec + i;
    }
    else if( (signed long)idx < 0 )		//超出溢出（时间走完一轮了），从第一级开始，
    {
        vec = base->tv1.vec + (base->current_index & TVR_MASK);
    }
    else
    {
    	//设置的定时器超过最大值 则按最大值设置定时器
        int i;
        if (idx > 0xffffffffUL)
        {
            idx = 0xffffffffUL;
            expires = idx + base->current_index;
        }
        i = (expires >> (TVR_BITS + 3 * TVN_BITS)) & TVN_MASK;
        vec = base->tv5.vec + i;
    }

    //插入链表中
    list_add_tail(&timer->entry, vec);
}

static inline void detach_timer(struct timer_list *timer)
{
    struct list_head *entry = &timer->entry;
    __list_del(entry->prev, entry->next);
    entry->next = NULL;
    entry->prev = NULL;
}

static int __mod_timer(struct timer_list *timer, unsigned long expires)
{
    if(NULL != timer->entry.next)
    {
        detach_timer(timer);
    }

    internal_add_timer(timer->base, timer);

    return 0;
}

//修改定时器的超时时间外部接口
int timewheel_mod_timer(timerInfoNode_t ptimer, unsigned long expires)
{
    struct timer_list *timer = (struct timer_list *)ptimer;
    struct tvec_base *base = NULL;

    if(timer == NULL)
    {
    	return -1;
    }

	base = timer->base;
    if(NULL == base)
    {
        return -1;
    }

    expires = expires + base->current_index;
    if((timer->entry.next != NULL)  && (timer->expires == expires))
    {
        return 0;
    }

    if( NULL == timer->function )
    {
        printf("timer's timeout function is null\n");
        return -1;
    }

	timer->expires = expires;
    return __mod_timer(timer,expires);
}

//添加一个定时器
static int __ti_add_timer(struct timer_list *timer)
{
    if( NULL != timer->entry.next )
    {
        printf("timer is already exist\n");
        return -1;
    }

    return timewheel_mod_timer(timer, timer->expires);
}

/*添加一个定时器
 *返回定时器
 */
timerInfoNode_t timewheel_add_timer(timerHandle_t ptimewheel, unsigned long expires,timeouthandle phandle, void *user)
{
	int ret = 0;
    struct timer_list *ptimer = NULL;

    ptimer = (struct timer_list *)malloc( sizeof(struct timer_list) );
    if(NULL == ptimer)
    {
    	printf("malloc error!!!\n");
        return NULL;
    }

    bzero( ptimer,sizeof(struct timer_list) );
    ptimer->entry.next = NULL;
    ptimer->base = (struct tvec_base *)ptimewheel;
    ptimer->expires = expires;
    ptimer->function  = phandle;
    ptimer->user = user;

    ret = __ti_add_timer(ptimer);
    if(ret < 0)
    {
    	printf("__ti_add_timer error!!\n");
    	free(ptimer);
    	ptimer = NULL;
    }

    return ptimer;
}

/*
 * 删除一个定时器
 * @param in:nodeTimer:定时器节点，添加一个定时器时返回的值
 * @param out：0-成功，-1-失败
 * */
int timewheel_del_timer(timerInfoNode_t nodeTimer)
{
    struct timer_list *ptimer = (struct timer_list*)nodeTimer;
    if(NULL == ptimer)
    {
        return -1;
    }

    if(NULL != ptimer->entry.next)
    {
        detach_timer(ptimer);
    }

    free(ptimer);
    ptimer = NULL;
    return 0;
}

/*
 * 降级处理
 * 即将高级的定时器往低级移动
 * 每个tick到来，都只会去检测最低级的tv1的时间轮，因为多级时间轮的设计决定了最低级的时间轮永远保存这最近要超时的定时器
 * */
static int cascade(struct tvec_base *base, struct tvec *tv, int index)
{
    struct list_head *pos = NULL,*tmp = NULL;
    struct timer_list *timer = NULL;
    struct list_head tv_list;

    list_replace_init(tv->vec + index, &tv_list);	//替换结点

    list_for_each_safe(pos, tmp, &tv_list)
    {
        timer = list_entry(pos,struct timer_list,entry);
        internal_add_timer(base, timer);
    }

    return index;
}

static void *deal_function_timeout(void *base)
{
    struct timer_list *timer = NULL;
    struct timeval tv;
    struct tvec_base *ba = (struct tvec_base *)base;

    for(;;)
    {
        gettimeofday(&tv, NULL);	//返回自1970-01-01 00:00:00到现在经历的秒数
        while( ba->current_index <= (unsigned long)(tv.tv_sec*1000 + tv.tv_usec/1000) )
        {
			struct list_head work_list;
			int index = ba->current_index & TVR_MASK;
			struct list_head *head = &work_list;
			if(!index && (!cascade(ba, &ba->tv2, INDEX(0))) &&( !cascade(ba, &ba->tv3, INDEX(1))) && (!cascade(ba, &ba->tv4, INDEX(2))) )
			{
			   cascade(ba, &ba->tv5, INDEX(3));
			}

			ba->current_index ++;//+= ba->tickTime;
			list_replace_init(ba->tv1.vec + index, &work_list);
			while(!list_empty(head))
			{
				/*执行这个时间点的注册的回调函数*/
				timer = list_first_entry(head, struct timer_list, entry);
				detach_timer(timer);	//删除该定时器
				//执行回调函数
				timer->function(timer->user);
			}
        }

        usleep((ba->tickTime)*1000);	//防止cpu过高
    }

    return NULL;
}

static void init_tvr_list(struct tvec_root * tvr)
{
    int i;

    for( i = 0; i < TVR_SIZE; i++ )
    {
        INIT_LIST_HEAD(&tvr->vec[i]);
    }
}


static void init_tvn_list(struct tvec * tvn)
{
    int i;

    for( i = 0; i < TVN_SIZE; i++ )
    {
        INIT_LIST_HEAD(&tvn->vec[i]);
    }
}

//创建时间轮  外部接口
timerHandle_t timewheel_create_init(int tickTime)
{
    struct tvec_base *base = NULL;
    int ret = 0;
    struct timeval tv;

    base = (struct tvec_base *) malloc( sizeof(struct tvec_base) );
    if( NULL == base )
    {
        return NULL;
    }

    bzero( base,sizeof(struct tvec_base) );

    init_tvr_list(&base->tv1);
    init_tvn_list(&base->tv2);
    init_tvn_list(&base->tv3);
    init_tvn_list(&base->tv4);
    init_tvn_list(&base->tv5);

    if(tickTime >= 10)
    {
    	base->tickTime = tickTime;
    }else
    {
    	base->tickTime = 10;	//防止cpu过高，所以默认10ms
    }

    gettimeofday(&tv, NULL);	//返回自1970-01-01 00:00:00到现在经历的秒数
    base->current_index = tv.tv_sec*1000 + tv.tv_usec/1000;

    ret = OS_thrCreate(&base->thdealfun,deal_function_timeout,OS_DETACH,OS_THR_STACK_SIZE_DEFAULT,(void*)base);
    if(ret < 0)
    {
        free(base);
        base = NULL;
    }

    return base;
}

static void ti_release_tvr(struct tvec_root *pvr)
{
    int i;
    struct list_head *pos,*tmp;
    struct timer_list *pen;

    for(i = 0; i < TVR_SIZE; i++)
    {
        list_for_each_safe(pos,tmp,&pvr->vec[i])
        {
            pen = list_entry(pos,struct timer_list, entry);
            list_del(pos);
            free(pen);
        }
    }
}

static void ti_release_tvn(struct tvec *pvn)
{
    int i;
    struct list_head *pos,*tmp;
    struct timer_list *pen;

    for(i = 0; i < TVN_SIZE; i++)
    {
        list_for_each_safe(pos,tmp,&pvn->vec[i])
        {
            pen = list_entry(pos,struct timer_list, entry);
            list_del(pos);
            free(pen);
        }
    }
}


/*
 *释放时间轮 外部接口
 * */
int timewheel_destory_unInit(timerHandle_t pwheel)
{
    struct tvec_base *base = (struct tvec_base *)pwheel;

    if(NULL == base)
    {
        return -1;
    }

    ti_release_tvr(&base->tv1);
    ti_release_tvn(&base->tv2);
    ti_release_tvn(&base->tv3);
    ti_release_tvn(&base->tv4);
    ti_release_tvn(&base->tv5);

    free(pwheel);
    pwheel = NULL;

    return 0;
}



//////////////////////////////////demo//////////////////////////////////////

struct starg
{
    void *timer;
    int d;
};

#define TEST_FPS_TIME	(3000) //ms

void mytimer(void *argv)
{
      //struct starg *par = (struct starg *)argv;

      //printf("hello  %d  %x\n",par->d,par->timer);
      //timewheel_mod_timer(par->timer,TEST_FPS_TIME/*1000*/);  //进行在次启动定时器,再次挂载这个定时器

      //若定时器不在使用则
      //timewheel_del_timer(par->timer);
}


int timewheel_timer_demo()
{
	void *pwheel = NULL;
	//void *timer = NULL;
	struct starg *par = NULL;

	par = (struct starg *)malloc( sizeof(struct starg) );
	if(NULL == par)
	{
		return 0;
	}
	bzero(par,sizeof(struct starg));

	//创建一个时间轮
	pwheel = timewheel_create_init(20);
	if(NULL == pwheel)
	{
		return -1;
	}

	//添加一个定时器
	par->d = 100;
	par->timer = timewheel_add_timer(pwheel, TEST_FPS_TIME/*1000*/, mytimer, (void *)par);

	//马上修改超时时间
	timewheel_mod_timer(par->timer,1000);

	while(1)
	{
		sleep(60);
	}

	//释放时间轮
	timewheel_destory_unInit(pwheel);

	return 0;
}




















