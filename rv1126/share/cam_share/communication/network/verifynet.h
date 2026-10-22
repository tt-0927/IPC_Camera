

#ifndef _OSA_BASE_CORE_SOURCE_VERIFYNETWORK_INCLUDE_
#define _OSA_BASE_CORE_SOURCE_VERIFYNETWORK_INCLUDE_




#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#ifndef UINT32
#define UINT32 unsigned int
#endif


/*
 * 判断ip是否合法
 * param[in]:ip:ip地址
 * param[out]: TRUE-合法，FALSE-非法
 * */
int netIpIsValid(const char *ip);

/*
 * 判断子网掩码是否合法
 * param[in]:mask:掩码
 * param[out]: TRUE-合法，FALSE-非法
 * */
int netMaskIsValid(char *mask);

/*
 * 判断子网掩码跟ip组合校验是否有效
 * param[in]:str:ip地址
 * param[in]:maskstr:子网掩码
 * param[out]: TRUE-合法，FALSE-非法
 * */
int netMaskAndIpIsValid(char *str, char *maskstr);


/*
 * 判断两个ip是否在同一个网段
 * param[in]:str:主网ip
 * param[in]:substr:子网ip
 * param[in]:maskstr:掩码
 * param[out]: TRUE-合法，FALSE-非法
 * */
int netIPAndSubnetValid(char *str, char *substr, char *maskstr);


/*
 * 判断ipv6地址是否合法
 * param[in]:pIpv6Addr:主网ip
 * param[out]: TRUE-合法，FALSE-非法
 * */
int net_is_validipv6(char *pIpv6Addr);


#endif	//_OSA_BASE_CORE_SOURCE_VERIFYNETWORK_INCLUDE_


