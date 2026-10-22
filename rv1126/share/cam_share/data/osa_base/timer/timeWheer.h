
#ifndef _CODE_SOURCE_TIMER_WHEER_INCLUDE_
#define _CODE_SOURCE_TIMER_WHEER_INCLUDE_


#define TVN_BITS  6
#define TVR_BITS  8
#define TVN_SIZE (1 << TVN_BITS)	//2，3，4，5级时间轮每轮的大小（64个齿轮）
#define TVR_SIZE (1 << TVR_BITS)	//第一级时间轮的大小（256齿轮）

#define TVN_MASK (TVN_SIZE - 1)
#define TVR_MASK (TVR_SIZE - 1)

#include "os_thr.h"
#include "os_list.h"


typedef void* timerHandle_t;			//定时器服务器
typedef void* timerInfoNode_t;			//定时器信息节点

#define SEC_VALUE 0
#define USEC_VALUE 2000

struct tvec_base;
#define INDEX(N) ((ba->current_index >> (TVR_BITS + (N) * TVN_BITS)) & TVN_MASK)


typedef void (*timeouthandle)(void *user);


struct timer_list
{
    struct list_head entry;          //将时间连接成链表
    unsigned long expires;           //超时时间
    timeouthandle function; 		 //超时后的回调处理函数
    void *user;             		 //处理函数的参数，（上层应用传入的数据）
    struct tvec_base *base;          //指向时间轮句柄
};

struct tvec
{
    struct list_head vec[TVN_SIZE];		//链表头
};

struct tvec_root
{
    struct list_head vec[TVR_SIZE];		//链表头
};

//实现5级时间轮 范围为0~ (2^8 * 2^6 * 2^6 * 2^6 *2^6)=2^32
/*
 * 链表数组	idx范围
	tv1		0-255(2^8)
	tv2		256--16383(2^14)
	tv3		16384--1048575(2^20)
	tv4		1048576--67108863(2^26)
	tv5		67108864--4294967295(2^32)
 *
 * */
struct tvec_base
{
	//需要改为unsigned long long
    unsigned long current_index;	//记录自1970-01-01 00:00:00到现在经历的毫秒数，单位：ms
    OS_ThrHndl  thincrejiffies;
    OS_ThrHndl  thdealfun;
    struct tvec_root tv1;
    struct tvec      tv2;
    struct tvec      tv3;
    struct tvec      tv4;
    struct tvec      tv5;
    unsigned long tickTime;	//时间多久跳一次tick[10,~]ms

};


/*
 * 创建定时器
 * @param in:tickTime:时间多久跳一次，[10,~]ms，单位ms
 * @param return:定时器句柄
 * */
timerHandle_t timewheel_create_init(int tickTime);

/*
 * 销毁定时器
 * @param in:pwheel:定时器句柄
 * */
int timewheel_destory_unInit(timerHandle_t pwheel);

/*
 * 删除一个定时器
 * @param in:nodeTimer:定时器节点，添加一个定时器时返回的值
 * @param out：0-成功，-1-失败
 * */
int timewheel_del_timer(timerInfoNode_t nodeTimer);

/* 添加一个定时器
 * @param in : ptimewheel:定时器句柄
 * @param in expires:定时器超时时间，单位ms，延时多久后触发时间
 * @param in phandle:时间到达后，触发的事件，调用的回调函数
 * @param in user:回调函数带入的参数，用户自定义参数
 * @param out return:成功返回timerInfoNode_t节点信息，失败返回NULL
 */
timerInfoNode_t timewheel_add_timer(timerHandle_t ptimewheel, unsigned long expires,timeouthandle phandle, void *user);

/*
 * 修改定时器的超时时间，会重新注册定时器到内核，而不管定时器函数是否被运行过。
 * @param[in]: ptimer:定时器节点
 * @param[in]: expires:定时器超时时间，单位ms，延时多久后触发时间
 * @param[out]:return:成功返回timerInfoNode_t节点信息，失败返回NULL
 * */
int timewheel_mod_timer(timerInfoNode_t ptimer, unsigned long expires);

int timewheel_timer_demo();

#endif //_CODE_SOURCE_TIMER_WHEER_INCLUDE_
