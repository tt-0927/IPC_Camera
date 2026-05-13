#ifndef DONT_USE_SHARE_OS
#define _GNU_SOURCE
#include "share_os.h"
#include "dlog.h"
#include "stdint.h"
#include <arpa/inet.h>
#include <net/if.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/resource.h>
#include <sys/socket.h>

int32_t r_setpriority(int32_t which, int32_t who, int32_t prio)
{
	return setpriority(which, who, prio);
}

void *r_malloc(size_t size)
{
	char *p = NULL;
	p = malloc(size);

	if (p != NULL)
	{
		memset(p, 0, size);
	}
	else
	{
		// printf("malloc size[%d] failed\n", size);
	}

	return p;
}

// 2014-10-22 20:03:16 modify
void r_free(void *ptr)
{
	if (ptr)
	{
		free(ptr);
		ptr = NULL;
	}
}

void *debug_malloc(size_t size, const char *file, int line, const char *func)
{
	void *p = NULL;
	p = malloc(size);
	// printf("%s:%d:%s:malloc(%d): p=0x%lx\n", file, line, func, size, (unsigned long)p);
	return p;
}

void *r_memset(void *s, int32_t c, size_t n)
{
	return memset(s, c, n);
}

int8_t *r_strcat(int8_t *dest, const int8_t *src)
{
	return (int8_t *)strcat((char *)dest, (const char *)src);
}
int8_t *r_strncat(int8_t *dest, const int8_t *src, size_t n)
{
	return (int8_t *)strncat((char *)dest, (const char *)src, n);
}
int8_t *r_strstr(const int8_t *haystack, const int8_t *needle)
{
	return (int8_t *)strstr((const char *)haystack, (const char *)needle);
}

int8_t *r_strrchr(const int8_t *s, int32_t c)
{
	return (int8_t *)strrchr((const char *)s, (int)c);
}

void r_bzero(void *s, size_t n)
{
	bzero(s, n);
}

int32_t r_select(int32_t maxfdp1, fd_set *readset, fd_set *writeset, fd_set *exceptset, struct timeval *timeout)
{
	return select(maxfdp1, readset, writeset, exceptset, timeout);
}

void *r_memalign(size_t boundary, size_t size)
{
	return memalign(boundary, size);
}

int32_t r_pthread_attr_init(pthread_attr_t *attr)
{
	return pthread_attr_init(attr);
}

int32_t r_pthread_attr_destroy(pthread_attr_t *attr)
{
	return pthread_attr_destroy(attr);
}

int32_t r_pthread_create(pthread_t *thread, pthread_attr_t *attr,
						 void *(*start_routine)(void *), void *arg)
{
	printf("pthread_create------------------[%s:%s:%d]\n ", __FILE__, __FUNCTION__, __LINE__);

	return pthread_create(thread, attr, start_routine, arg);
}

int32_t r_pthread_join(pthread_t thread, void **retval)
{
	return pthread_join(thread, retval);
}

ssize_t r_write(int32_t fd, const void *buf, size_t count)
{
	return write(fd, buf, count);
}

ssize_t r_read(int32_t fd, void *buf, size_t count)
{
	return read(fd, buf, count);
}

ssize_t r_recv(int32_t socket, void *buffer, size_t len, int32_t iFlags)
{
	return recv(socket, buffer, len, iFlags);
}

ssize_t r_recvfrom(int32_t socket, void *buffer, size_t len, int32_t flags, struct sockaddr *from, socklen_t *from_len)
{
	return recvfrom(socket, buffer, len, flags, from, from_len);
}

ssize_t r_send(int32_t socket, const void *buffer, size_t len, int32_t flags)
{
	return send(socket, buffer, len, flags);
}

void *r_memcpy(void *dest, const void *src, size_t n)
{
#ifdef MYMEMCPY

	return memcpy(dest, src, n);
	// return memcpy_arm(dest, src, n);
#else
	return memcpy(dest, src, n);
#endif
}

void *r_cmemcpy(int8_t *dest, int8_t *src, size_t n, int32_t (*condition)(void *))
{
	int32_t i;
	int32_t j = 0;

	for (i = 0; i <= n; i++)
	{
		if (condition(src))
		{
			dest[j++] = src[i];
		}
	}

	dest[j++] = 0;
	return dest;
}

int8_t *r_strcpy(int8_t *dest, const int8_t *src)
{
	return (int8_t *)strcpy((char *)dest, (const char *)src);
}

int8_t *r_strncpy(int8_t *dest, int8_t *src, size_t n)
{
	return (int8_t *)strncpy((char *)dest, (const char *)src, n);
}

int32_t r_getsockname(int32_t socket, struct sockaddr *name, socklen_t *name_len)
{
	return getsockname(socket, name, name_len);
}

uint16_t r_htons(uint16_t hostshort)
{
	return htons(hostshort);
}

uint32_t r_htonl(uint32_t hostlong)
{
	return htonl(hostlong);
}

int8_t r_inet_aton(const int8_t *cp, struct in_addr *inp)
{
	return inet_aton((const char *)cp, inp);
}

ssize_t r_sendto(int32_t socket, const void *buffer, size_t len, int32_t flags,
				 const struct sockaddr *to, socklen_t tolen)
{
	return sendto(socket, buffer, len, flags, to, tolen);
}

int32_t r_accept(int32_t sockfd, struct sockaddr *addr, socklen_t *addrlen)
{
	return accept(sockfd, addr, addrlen);
}

int32_t r_setsockopt(int32_t socket, int32_t level, int32_t opt_name, const void *opt_val, socklen_t opt_len)
{
	return setsockopt(socket, level, opt_name, opt_val, opt_len);
}

int32_t r_getsockopt(int32_t sockfd, int32_t level, int32_t optname, void *optval, socklen_t *optlen)
{
	return getsockopt(sockfd, level, optname, optval, optlen);
}

int32_t r_bind(int32_t socket, const struct sockaddr *address, socklen_t address_len)
{
	return bind(socket, address, address_len);
}

int32_t r_listen(int32_t sockfd, int32_t backlog)
{
	return listen(sockfd, backlog);
}

size_t r_strlen(const int8_t *s)
{
	return strlen((const char *)s);
}

int32_t r_connect(int32_t socket, const struct sockaddr *serv_addr, socklen_t addr_len)
{
	return connect(socket, serv_addr, addr_len);
}

int32_t r_open(const int8_t *pathname, int32_t flags)
{
	return open((char *)pathname, flags);
}

uint16_t r_ntohs(uint16_t net_short)
{
	return ntohs(net_short);
}

int32_t r_pthread_detach(pthread_t th)
{
	return pthread_detach(th);
}

pthread_t r_pthread_self(void)
{
	return pthread_self();
}

uint32_t r_sleep(int32_t second)
{
	return sleep(second);
}

void r_usleep(int32_t u_second)
{
	usleep(u_second);
}

int32_t r_strcmp(const int8_t *s1, const int8_t *s2)
{
	return strcmp((const char *)s1, (const char *)s2);
}

int32_t r_strncmp(const int8_t *s1, const int8_t *s2, size_t n)
{
	return strncmp((const char *)s1, (const char *)s2, n);
}

int32_t r_memcmp(const void *s1, const void *s2, size_t n)
{
	return memcmp(s1, s2, n);
}

int32_t r_pthread_mutex_init(pthread_mutex_t *mutex, pthread_mutexattr_t *attr)
{
	return pthread_mutex_init(mutex, attr);
}

int32_t r_pthread_mutex_destroy(pthread_mutex_t *mutex)
{
	return pthread_mutex_destroy(mutex);
}

int32_t r_pthread_mutex_lock(pthread_mutex_t *mutex)
{
	return pthread_mutex_lock(mutex);
}

int32_t r_pthread_mutex_unlock(pthread_mutex_t *mutex)
{
	return pthread_mutex_unlock(mutex);
}

int32_t r_mkdir(const int8_t *pathname, mode_t mode)
{
	return (int32_t)mkdir((const char *)pathname, mode);
}

int32_t r_system(const int8_t *cmd_line)
{
	int ret = 0;
	Sigfunc *old_handler;

	old_handler = signal(SIGCHLD, SIG_DFL);
	ret = system((const char *)cmd_line);
	signal(SIGCHLD, old_handler);

	return ret;
}

FILE *r_fopen(const int8_t *path, const int8_t *mode)
{
	return fopen((const char *)path, (const char *)mode);
}

#if 1
/*捕捉信号量*/
Sigfunc *signal(int signo, Sigfunc *func)
{
	struct sigaction act, oact;

	act.sa_handler = func;
	sigemptyset(&act.sa_mask);
	act.sa_flags = 0;

	if (signo == SIGALRM)
	{
#ifdef SA_INTERRUPT
		act.sa_flags |= SA_INTERRUPT;
#endif
	}
	else
	{
#ifdef SA_RESTART
		act.sa_flags |= SA_RESTART;
#endif
	}

	if (sigaction(signo, &act, &oact) < 0)
	{
		return (SIG_ERR);
	}

	return (oact.sa_handler);
}
#endif
/*信号包裹函数*/
Sigfunc *Signal(int signo, Sigfunc *func)
{
	Sigfunc *sigfunc;

	if ((sigfunc = signal(signo, func)) == SIG_ERR)
	{
		printf("ERROR:  signal error \n");
	}

	return (sigfunc);
}

void ms_delay(int32_t ms)
{
	struct timeval tv;
	tv.tv_sec = 0;
	tv.tv_usec = ms * 1000;

	if (-1 == select(0, NULL, NULL, NULL, &tv))
	{
		printf("[ms_timer] select : %s", strerror(errno));
	}
}

int32_t str_is_num(int8_t *str)
{
	int32_t i = 0;

	for (i = 0; i < strlen((const char *)str); i++)
	{
		if (str[i] <= 48 || str[i] >= 57)
		{
			return 0;
		}
	}

	return 1;
}

uint32_t get_time(void)
{
	struct timeval tv;
	memset(&tv, 0, sizeof(tv));
	gettimeofday(&tv, NULL);
	return (tv.tv_sec * 1000 + tv.tv_usec / 1000);
}

//获取毫秒级别
uint32_t get_run_time(void)
{
	unsigned int msec;
	struct timespec tp;
	clock_gettime(CLOCK_MONOTONIC, &tp);
	msec = tp.tv_sec;
	msec = msec * 1000 + tp.tv_nsec / 1000000;
	return msec;
}

//获取 1/10 毫秒级别
uint32_t get_run_time_deci(void)
{
	unsigned int msec;
	struct timespec tp;
	clock_gettime(CLOCK_MONOTONIC, &tp);
	msec = tp.tv_sec;
	msec = msec * 10000 + tp.tv_nsec / 100000;
	return msec;
}

int64_t get_current_time_ms(void)
{
	struct timeval tv;
	memset(&tv, 0, sizeof(tv));
	gettimeofday(&tv, NULL);
	return (tv.tv_sec * 1000 + tv.tv_usec / 1000);
}

int64_t get_current_time_sec(void)
{
	struct timeval tv;
	memset(&tv, 0, sizeof(tv));
	gettimeofday(&tv, NULL);
	return (tv.tv_sec + tv.tv_usec / 1000000);
}

uint32_t get_localtime_sec(void)
{
	uint32_t current_time_sec = 0;
	time_t timer;
	struct tm *tblock;
	timer = time(NULL);
	tblock = localtime(&timer);
	current_time_sec = tblock->tm_hour * 3600 + tblock->tm_min * 60 + tblock->tm_sec;
	return current_time_sec;
}

int hi_getHwClock_time(int *year, int *month, int *mday, int *hours, int *min, int *sec, int *weekday)
{
	int ret = 0;
	char getbuff[512] = {0};

	ret = order_getInfo("hi_rtc_ctr -g time", getbuff, sizeof(getbuff));
	if (ret < 0)
	{
		printf("get hs clock time error!!\n");
		return -1;
	}

	/*
	 *[RTC_RD_TIME]
		Current time value:2019-6-5-8-23-54-3
	 * */
	char *p = strchr(getbuff, ':');
	printf("get--->%s\n", p + 1);

	sscanf(p + 1, "%d-%d-%d-%d-%d-%d-%d", year, month, mday, hours, min, sec, weekday);

	// printf("time--> %s\n", time);
	printf("%d %d %d %d %d %d %d\n", *year, *month, *mday, *hours, *min, *sec, *weekday);

	return 0;
}

int hi_setHwClock_time(char *time)
{
	if (time == NULL)
	{
		printf("this argument is NULL!!\n");
		return -1;
	}

	// 2019-06-04 20:37:34
	int ret = 0;
	int year = 0;
	int month = 0;
	int mday = 0;
	int hours = 0;
	int min = 0;
	int sec = 0;
	char cmd[128] = {0};

	sscanf(time, "%d-%d-%d %d:%d:%d", &year, &month, &mday, &hours, &min, &sec);
	sprintf(cmd, "hi_rtc_ctr -s time %d/%d/%d/%d/%d/%d", year, month, mday, hours, min, sec);
	printf("time[%s] time cmd:%s\n", time, cmd);
	system(cmd);

	return 0;
}

int set_localtime(int year, int month, int mday, int hours, int min, int sec) // zl
{
	if (year < 0)
	{
		return -1;
	}

	if (month > 12 || month < 1)
	{
		return -1;
	}

	if (mday > 31 || mday < 1)
	{
		return -1;
	}

	if (hours > 24 || hours < 0)
	{
		return -1;
	}

	if (min > 60 || min < 0)
	{
		return -1;
	}

	if (sec > 60 || sec < 0)
	{
		return -1;
	}

	char cmd[256] = {0};
	printf("set time:[%d-%d-%d %d:%d:%d]\n\n", year, month, mday, hours, min, sec);
	sprintf(cmd, "date -s \"%d-%d-%d %d:%d:%d\";hi_rtc_ctr -s time %d/%d/%d/%d/%d/%d",
			year, month, mday, hours, min, sec, year, month, mday, hours, min, sec);
	r_system((const int8_t *)cmd);
	r_system((const int8_t *)"/etc/init.d/save-rtc.sh");

	return 0;
}

/*
struct tm {
	int tm_sec;	//seconds
	int tm_min;	//minutes
	int tm_hour;	//hours
	int tm_mday;	//day of the month
	int tm_mon;	//month
	int tm_year;	//year
	int tm_wday;	//day of the week
	int tm_yday;	//day in the year
	int tm_isdst;	//daylight saving time
}
*/
void get_localtime(localtime_t *t)
{
	struct tm *tblock;
	time_t timer;
	timer = time(NULL);
	tblock = localtime(&timer);
	t->tm_year = tblock->tm_year + 1900;
	t->tm_mon = tblock->tm_mon + 1;
	t->tm_mday = tblock->tm_mday;
	t->tm_hour = tblock->tm_hour;
	t->tm_min = tblock->tm_min;
	t->tm_sec = tblock->tm_sec;
}
/*
 * 功能说明:  发送一包数据到消息队列
 * 参数说明:  ---- msqid	标识消息队列的ID
 *		   ---- msgp  	数据包首地址
 *		   ---- length 数据包的长度
 *		   ---- msgflag 发送标志位
 * 返回说明:  成功返回0，失败返回非0
 */
int32_t r_msg_send(int32_t msqid, msgque *msgp, int32_t length, int32_t msgflg)
{
	int32_t ret = 0;

	if (INVALID_MSGTYPE == msgp->msgtype)
	{
		printf("message send: invalid message type, please set the msgtype filed!\n");
		return -1;
	}

	ret = msgsnd(msqid, msgp, length, msgflg);

	return ret;
}

// add zl
int get_msgquen_info(int msgQid, int *msg_qbytes, int *msg_qnum)
{
	struct msqid_ds msg_info;

	if (msg_qbytes == NULL || msg_qnum == NULL)
	{
		return -1;
	}

	if (msgctl(msgQid, IPC_STAT, &msg_info) == -1)
	{
		return -1;
	}

	*msg_qbytes = msg_info.msg_qbytes;
	*msg_qnum = msg_info.msg_qnum;

	return 0;
}

int set_msgquen_info(int msgQid, int msg_qbytes)
{
	struct msqid_ds msg_info;

	if (msgctl(msgQid, IPC_STAT, &msg_info) == -1)
	{
		return -1;
	}

	msg_info.msg_qbytes = msg_qbytes;

	if (msgctl(msgQid, IPC_SET, &msg_info) == -1)
	{
		return -1;
	}

	return 0;
}

/*
 * 功能说明:  从消息队列里接收一帧数据
 * 参数说明:  ---- msqid	标识消息队列的ID
 *		   ---- msgp  	数据包首地址
 *		   ---- length 数据包的长度
 *		   ---- msgtype 数据包的类型
 *		   ---- msgflag 发送标志位
 * 返回说明:  成功返回0，失败返回非0
 */
int32_t r_msg_recv(int32_t msqid, msgque *msgp, int32_t length, long msgtyp, int32_t msgflg)
{
	int32_t recvlen = 0;

	recvlen = msgrcv(msqid, msgp, length, msgtyp, msgflg);
	return recvlen;
}

/**
 * @功能说明:
 * @参数说明:	----
 * @返回说明:
 */
int32_t r_msg_create(void)
{
	int32_t msgqid = 0;

	/*IPC_PRIVATE表示自动分配key*/
	msgqid = msgget(IPC_PRIVATE, IPC_CREAT | 0666);

	if (msgqid < 0)
	{
		printf("msgget error, errmsg = %s!\n", strerror(errno));
		return -1;
	}

	return msgqid;
}

int32_t r_msg_create_u(int32_t key)
{
	int32_t msgqid = 0;
	/*IPC_PRIVATE表示自动分配key*/
	msgqid = msgget((key_t)key, IPC_EXCL | IPC_CREAT | 0666);

	if (msgqid < 0)
	{
		msgqid = msgget((key_t)key, 0666);

		if (msgctl(msgqid, IPC_RMID, NULL) == -1)
		{
			return -1;
		}
	}

	msgqid = msgget((key_t)key, IPC_CREAT | 0666);

	printf("msgid=%d,key=%d\n", msgqid, key);

	if (msgqid < 0)
	{
		printf("msgget error, errmsg = %s!\n", strerror(errno));
		return -1;
	}

	return msgqid;
}

int32_t r_msg_create_key(int8_t *path, int32_t num)
{
	int32_t msgqid = -1;
	key_t key = ftok((const char *)path, num);
	// msgctl( msqid, IPC_RMID, NULL) ;
	/*IPC_PRIVATE表示自动分配key*/

	msgqid = msgget(key, IPC_EXCL | IPC_CREAT | 0666);

	if (msgqid < 0)
	{
		//	dlog(LOG_ERROR, "msgget error:[%d], errmsg = %s!\n", errno, strerror(errno));
		msgqid = msgget(key, 0666);

		if (msgctl(msgqid, IPC_RMID, NULL) == -1)
		{
			return -1;
		}
	}

	msgqid = msgget(key, IPC_CREAT | 0666);

	if (msgqid < 0)
	{
		printf("msgget error, errmsg = %s!\n", strerror(errno));
		return -1;
	}

	return msgqid;
}

int32_t get_msg_num(int32_t msgid)
{
	struct msqid_ds buf;

	if (msgctl(msgid, IPC_STAT, &buf) == -1)
	{
		return -1;
	}

	return buf.msg_qnum;
}

int32_t r_msg_del(int32_t msgid)
{
	if (msgctl(msgid, IPC_RMID, NULL) == -1)
	{
		return -1;
	}

	return 0;
}

int32_t filecpy(int8_t *dst_filename, int8_t *src_filename)
{
	int8_t cp_cmd[1024] = {0};
	sprintf((char *)cp_cmd, "cp -rf %s %s", src_filename, dst_filename);
	r_system((const int8_t *)cp_cmd);
	return 0;
}

int64_t get_file_size(char *filename)
{
	struct stat f_stat;
	printf("......%p %s....\n", filename, filename);
	if (stat(filename, &f_stat) == -1)
	{
		return -1;
	}
	// printf("%lld\n", (int64_t)f_stat.st_size);
	return (int64_t)f_stat.st_size;
}

char *get_file_buf(int *len, char *filename)
{
	int xml_len = 0;
	char *xml_buf = NULL;
	int read_len = 0;
	int fd = -1;
	xml_len = (int)get_file_size(filename);

	if (xml_len < 0)
	{
		dlog(LOG_ERROR, "file len is error\n");
		return NULL;
	}

	dlog(LOG_DEBUG, "the file len = %d\n", xml_len);

	if ((fd = open(filename, O_RDWR | O_DSYNC, 0777)) < 0)
	{
		dlog(LOG_DEBUG, "open[%s]  : %s", filename, strerror(errno));
		return NULL;
	}

	xml_buf = r_malloc(xml_len + 1);

	if (xml_buf == NULL)
	{
		dlog(LOG_ERROR, "malloc buff is failed\n");
		return NULL;
	}

	r_memset(xml_buf, 0, (xml_len + 1));

	read_len = read(fd, xml_buf, xml_len);

	if (read_len != xml_len)
	{
		dlog(LOG_ERROR, "read file failed, readlen=%d,xml_len=%d\n", read_len, xml_len);

		if (xml_buf != NULL)
		{
			free(xml_buf);
		}

		close(fd);
		return NULL;
	}

	*len = read_len;

	dlog(LOG_DEBUG, "read file[%s],[%s]\n", filename, xml_buf);

	return xml_buf;
}

int32_t data2file(int8_t *filename, int8_t *data, int32_t data_len)
{
	int32_t fd = -1;

	if ((fd = open((char *)filename, O_CREAT | O_WRONLY | O_TRUNC | O_DSYNC, 0666)) < 0)
	{
		dlog(LOG_ERROR, "open[%s]  : %s", filename, strerror(errno));
		return -1;
	}

	if (NULL != data || 0 < data_len)
	{
		if (0 > write(fd, data, data_len))
		{
			dlog(LOG_WARN, "write  : %s", strerror(errno));
			close(fd);
			return -1;
		}
	}

	close(fd);
	return 0;
}

int32_t dete_dir_exists(int8_t *dirname)
{
	struct stat filestat;

	if (stat((char *)dirname, &filestat) != 0)
	{
		return -1;
	}

	return S_ISDIR(filestat.st_mode) ? 1 : 0;
}

char *r_basename(char *path)
{
	/* Ignore all the details above for now and make a quick and simple
	   implementaion here */
	char *s1;
	char *s2;

	s1 = strrchr(path, '/');
	s2 = strrchr(path, '\\');

	if (s1 && s2)
	{
		path = (s1 > s2 ? s1 : s2) + 1;
	}
	else if (s1)
	{
		path = s1 + 1;
	}
	else if (s2)
	{
		path = s2 + 1;
	}

	return path;
}

static int32_t CopyDirectory(int8_t *SrcPath, int8_t *DstPath)
{
	if ((NULL == SrcPath) || (NULL == DstPath))
	{
		return -1;
	}

	DIR *pDir;
	struct dirent *ent;
	char Srcchildpath[1024];
	char Dstchildpath[1024];
	int32_t ret = 0;

	pDir = opendir((const char *)SrcPath);

	if (NULL == pDir)
	{
		printf("CopyDirectory Cannot open directory:[ %s ]\n", SrcPath);
		return -1;
	}

	while ((ent = readdir(pDir)) != NULL)
	{
		if (ent->d_type & DT_DIR)
		{

			if (r_strcmp((const int8_t *)ent->d_name, (const int8_t *)".") == 0 || r_strcmp((const int8_t *)ent->d_name, (const int8_t *)"..") == 0)
			{
				continue;
			}

			sprintf(Srcchildpath, "%s/%s", SrcPath, ent->d_name);
			sprintf(Dstchildpath, "%s/%s", DstPath, ent->d_name);
			mkdir(Dstchildpath, 0777);
			ret = CopyDirectory((int8_t *)Srcchildpath, (int8_t *)Dstchildpath);

			if (ret == -1)
			{
				break;
			}

			// printf("CopyDirectory ------> %s\n",Dstchildpath);
			// num = num + ret;
		}
		else
		{

			int8_t LocalFilePath[1024] = {0};
			int8_t DstFilePath[1024] = {0};
			FILE *SrcFile = NULL;
			FILE *DstFile = NULL;
			int8_t buffer[4096] = {0};
			int retrun = 0;

			sprintf((char *)LocalFilePath, "%s/", SrcPath);
			sprintf((char *)DstFilePath, "%s/", DstPath);
			r_strncat(LocalFilePath, (const int8_t *)ent->d_name, r_strlen((const int8_t *)ent->d_name));
			r_strncat(DstFilePath, (const int8_t *)ent->d_name, r_strlen((const int8_t *)ent->d_name));

			/* 隐藏文件过滤 */
			if (ent->d_name[0] == '.')
			{
				continue;
			}

			SrcFile = fopen((const char *)LocalFilePath, "r");

			if (SrcFile == NULL)
			{

				ret = -1;
				break;
			}

			DstFile = fopen((const char *)DstFilePath, "w");

			if (DstFile == NULL)
			{
				ret = -1;
				fclose(SrcFile);
				break;
			}

			// printf("CopyDirectory ------> %s\n",LocalFilePath);

			while (1)
			{
				int n, m = 0;
				n = fread(buffer, 1, 4096, SrcFile);

				if (n <= 0)
				{
					// printf("CopyDirectory -----------read fail---------\n");
					// retrun = -1;
					break;
				}

				m = fwrite(buffer, 1, n, DstFile);

				if (m != n)
				{
					retrun = -1;
					// printf("CopyDirectory -----------write fail---------\n");
					break;
				}

				// printf("CopyDirectory -----------write ok---------\n");
			}

			// printf("CopyDirectory ------> %s\n",DstFilePath);
			fclose(SrcFile);
			fclose(DstFile);

			if (retrun == -1)
			{
				ret = -1;
				break;
			}
		}
	}

	if (pDir != NULL)
	{
		closedir(pDir);
	}

	return ret;
}

static int32_t DelDirectory(int8_t *Pathh)
{
	if (NULL == Pathh)
	{
		return -1;
	}

	DIR *pDir;
	struct dirent *ent;
	char Srcchildpath[1024];
	int32_t ret = 1;
	int32_t ret_val = 0;

	pDir = opendir((const char *)Pathh);

	if (NULL == pDir)
	{
		printf("DelDirectory Cannot open directory:[ %s ]\n", Pathh);
		return -1;
	}

	if (pDir)

		while ((ent = readdir(pDir)) != NULL)
		{
			if (ent->d_type & DT_DIR)
			{

				if (r_strcmp((const int8_t *)ent->d_name, (const int8_t *)".") == 0 || r_strcmp((const int8_t *)ent->d_name, (const int8_t *)"..") == 0)
				{
					continue;
				}

				sprintf(Srcchildpath, "%s/%s", Pathh, ent->d_name);
				ret = DelDirectory((int8_t *)Srcchildpath);

				if (ret == -1)
				{
					break;
				}

				// num = num + ret;
			}
			else
			{
				int8_t LocalFilePath[1024] = {0};

				sprintf((char *)LocalFilePath, "%s/", Pathh);
				r_strncat(LocalFilePath, (const int8_t *)ent->d_name, r_strlen((const int8_t *)ent->d_name));

				/* 隐藏文件过滤 */
				if (ent->d_name[0] == '.')
				{
					continue;
				}

				ret_val = unlink((const char *)LocalFilePath);

				if (ret_val == -1)
				{
					printf("DelDirectory unlink [ret_val = %d][unlink File = %s]\n", ret_val, LocalFilePath);
					ret = -1;
					break;
				}
			}
		}

	ret_val = rmdir((const char *)Pathh);

	if (ret_val == -1)
	{
		printf("DelDirectory unlink [ret_val = %d][rmdir Dir = %s]\n", ret_val, Pathh);
		ret = -1;
	}

	if (pDir != NULL)
	{
		closedir(pDir);
	}

	return ret;
}

int32_t r_CopyDir(int8_t *SrcPath, int8_t *DstPath)
{
	if (SrcPath == NULL || DstPath == NULL)
	{
		return -1;
	}

	mkdir((char *)DstPath, 0777);

	return CopyDirectory(SrcPath, DstPath);
}

int32_t DelDir(int8_t *SrcPath)
{
	if (SrcPath == NULL)
	{
		return -1;
	}
	return DelDirectory(SrcPath);
}

int32_t CopyFile(int8_t *LocalFilePath, int8_t *DstFilePath)
{
	FILE *SrcFile = NULL;
	FILE *DstFile = NULL;
	int8_t buffer[4096] = {0};
	int retrun = 0;

	if (LocalFilePath == NULL || DstFilePath == NULL)
	{
		printf("CopyFile param ERROR\n");
		return -1;
	}

	SrcFile = fopen((const char *)LocalFilePath, "r");

	if (SrcFile == NULL)
	{
		printf("CopyFile fopen SrcFile ERROR\n");
		return -1;
	}

	DstFile = fopen((const char *)DstFilePath, "w");

	if (DstFile == NULL)
	{
		printf("CopyFile fopen SrcFile ERROR\n");
		fclose(SrcFile);
		return -1;
	}

	// printf("CopyDirectory ------> %s\n",LocalFilePath);

	while (1)
	{
		int n, m = 0;
		n = fread(buffer, 1, 4096, SrcFile);
		if (n <= 0)
		{
			// printf("CopyDirectory -----------read fail---------\n");
			// retrun = -1;
			break;
		}

		m = fwrite(buffer, 1, n, DstFile);

		if (m != n)
		{
			retrun = -1;
			printf("CopyFile -----------write fail---------\n");
			break;
		}

		// printf("CopyDirectory -----------write ok---------\n");
	}

	// printf("CopyDirectory ------> %s\n",DstFilePath);
	if (SrcFile)
	{
		fclose(SrcFile);
	}

	if (DstFile)
	{
		fclose(DstFile);
	}

	return retrun;
}

int32_t DelFile(int8_t *FileName)
{
	if (FileName == NULL)
	{
		return -1;
	}

	return unlink((const char *)FileName);
}

int32_t get_usb_dev_max_available_dir(int8_t *cdir)
{
	int end = 0;
	const char const Available[1024] = {"df |awk '/media/{print $4 \" \" $6}'|sort -rn|sed '2,$d'|awk '{print $2}'"};
	FILE *fp = NULL;
	fp = popen(Available, "r");

	if (NULL == fp)
	{
		printf("cmd:[%s] ERROR...\n", Available);
		return 0;
	}

	if (fgets((char *)cdir, 64, fp) == NULL)
	{
		printf("cdir:[%s] ERROR...\n", cdir);
		pclose(fp);
		return 0;
	}

	end = r_strlen(cdir) - 1;

	if (end >= 64)
	{
		printf("end:[%d] ERROR...\n", end);
		pclose(fp);
		return 0;
	}

	cdir[end] = 0;
	pclose(fp);
	fp = NULL;
	const char const mount_rw[1024] = {0};
	char rw[8] = {0};
	sprintf((char *)mount_rw, "mount|grep \"%s\"|awk -F\"[(,]\" '{print $2}' ", cdir);
	fp = popen(mount_rw, "r");

	if (NULL == fp)
	{
		printf("cmd:[%s] ERROR...\n", mount_rw);
		return 0;
	}

	if (fgets(rw, 8, fp) == NULL)
	{
		printf("cdir:[%s] ERROR...\n", rw);
		pclose(fp);
		return 0;
	}

	end = r_strlen((const int8_t *)rw) - 1;

	if (end >= 8)
	{
		printf("end:[%d] ERROR...\n", end);
		pclose(fp);
		return 0;
	}

	rw[end] = 0;
	pclose(fp);

	if (r_strcmp((const int8_t *)rw, (const int8_t *)"rw"))
	{
		printf("rw:[%s] ERROR...\n", rw);
		return 0;
	}

	return 1;
}

static uint64_t read_usb_dev_max_available(void)
{
	int8_t Available[128] = {0};
	int end = 0;
	const char const AvailableCmd[1024] = {"df |awk '/media/{print $4 \" \" $6}'|sort -rn|sed '2,$d'|awk '{print $1}'"};
	FILE *fp = popen(AvailableCmd, "r");

	if (NULL == fp)
	{
		printf("cmd:[%s] ERROR...\n", AvailableCmd);
		return 0;
	}

	if (fgets((char *)Available, 128, fp) == NULL)
	{
		printf("cdir:[%s] ERROR...\n", Available);
		pclose(fp);
		return 0;
	}

	end = r_strlen(Available) - 1;

	if (end >= 8)
	{
		printf("end:[%d] ERROR...\n", end);
		pclose(fp);
		return 0;
	}

	Available[end] = 0;
	pclose(fp);
	return atoll((const char *)Available);
}

static uint64_t g_max_usb_dev_available = 0;

uint64_t get_usb_dev_max_available(void)
{
	return g_max_usb_dev_available;
}

static void *update_usb_dev_max_available(void *arg)
{
	while (1)
	{
		g_max_usb_dev_available = read_usb_dev_max_available();

		if (0 == g_max_usb_dev_available)
		{
			; // printf("update_usb_dev_max_available zero failed!!!");
		}

		ms_delay(1000);
	}

	pthread_detach(pthread_self());
	pthread_exit(0);
	return NULL;
}

int32_t check_usb_dev_max_available(void)
{
	pthread_t p;
	//printf_pthread_create(__FILE__, ("update_usb_dev_max_available"));

	if (r_pthread_create(&p, NULL, update_usb_dev_max_available, NULL))
	{
		printf("Failed to create web listen thread\n");
		return -1;
	}

	return 0;
}

int32_t ReadCfgFile(const int8_t *Cfg, const int8_t *default_value, int8_t *buf, int32_t buf_len)
{
	FILE *CfgFile = NULL;

	int32_t nRet = 0;

	if (access((const char *)Cfg, F_OK) != 0)
	{
		nRet = -5;
		CfgFile = r_fopen((const int8_t *)Cfg, (const int8_t *)"w");

		if (NULL == CfgFile)
		{
			nRet = -1;
			goto ERR;
		}

		nRet = fwrite((const char *)default_value, 1, strlen((const char *)default_value), CfgFile);

		if (nRet <= 0)
		{
			nRet = -2;
		}
	}
	else
	{
		CfgFile = r_fopen((const int8_t *)Cfg, (const int8_t *)"r");

		if (NULL == CfgFile)
		{
			nRet = -3;
			goto ERR;
		}

		nRet = fread(buf, 1, buf_len, CfgFile);

		if (nRet <= 0)
		{
			nRet = -4;
		}
	}

	if (CfgFile)
	{
		fclose(CfgFile);
	}

ERR:

	if (nRet < 0)
	{
		r_strncpy(buf, (int8_t *)default_value, buf_len);
	}

	return nRet;
}

unsigned int r_ctoi_hex(unsigned char c)
{
	unsigned int i = 0;

	if (c > '0' - 1 && c < '9' + 1)
	{
		i = c - '0';
	}
	else if (c > 'a' - 1 && c < 'f' + 1)
	{
		i = 10 + c - 'a';
	}

	return i;
}

int order_getInfo(char *command, char *freadBuf, int len)
{
	if (freadBuf == NULL || command == NULL)
	{
		printf("order_getInfo \n");
		return -1;
	}
	FILE *fd = popen(command, "r");
	if (fd == NULL)
	{
		printf("%s is fail\n", command);
		return -1;
	}
	int ret = fread(freadBuf, 1, len, fd);
	//printf("freadBuf:%s\n", freadBuf);
	if (ret <= 0)
	{
		//printf("fread catforkInfo%d\n", ret);
		pclose(fd);
		return -1;
	}
	pclose(fd);
	return ret;
}

static int get_firstChar(char *src, char *ch, char *dest)
{
	if ((NULL == src) || (NULL == ch) || (NULL == dest))
	{
		dlog(LOG_ERROR, "get_firstChar param is err");
	}

	char *pt;
	pt = strtok(src, ch);
	// dlog(LOG_DEBUG,"get_firstChar is %s\n", pt);
	sprintf(dest, "%s", pt);
	return 0;
}
/*
int get_udev_info(char *udev_info)
{
	if (NULL == udev_info)
	{
		dlog(LOG_ERROR, "get_udev_info param is err!!!\n");
		return -1;
	}
	FILE *fp;
	char filename[128] = {0};
	char *buffer = NULL;
	size_t len = 0;
	ssize_t readlen;
	int ret = -1;
	char path[48] = {0};
	sprintf(filename, "%s", UDEV_INFO_TXT);
	fp = fopen(filename, "r");
	if (NULL == fp)
	{
		dlog(LOG_ERROR, "fopen is err!!\n");
		return -1;
	}
	while ((readlen = getline(&buffer, &len, fp)) != -1)
	{
		if (NULL != strstr(buffer, "hdd"))
		{
			get_firstChar(buffer, ":", path);
			ret = 0;
		}
	}

	if (-1 == ret)
	{
		fclose(fp);
		return ret;
	}

	fclose(fp);
	sprintf(udev_info, "/dev/%s", "sda");
	// dlog(LOG_DEBUG,"the udev_info is %s\n", udev_info);
	return ret;
}
*/
void share_get_cpuRawData(CpuRawData_S *pCpuInfo)
{
	FILE *fd;
	char achBuff[256];
	fd = fopen("/proc/stat", "r");
	if (fd == NULL)
	{
		perror("fopen:");
	}
	fgets(achBuff, sizeof(achBuff), fd);

	sscanf(achBuff, "%s %u %u %u %u %u %u %u",
		   pCpuInfo->name,
		   &pCpuInfo->user,
		   &pCpuInfo->nice,
		   &pCpuInfo->system,
		   &pCpuInfo->idle,
		   &pCpuInfo->iowait,
		   &pCpuInfo->irq,
		   &pCpuInfo->softirq);
	fclose(fd);
}

/* 获取Linux系统的CPU使用率 */
double share_get_linux_cpu()
{
	double dCpuUse = 0.0;
	/* 记录上次读取到的cpu信息 */
	static CpuRawData_S s_stLastCpuInfo = {0};
	/* 记录本次读取到的cpu信息 */
	static CpuRawData_S s_stCurrCpuInfo = {0};
	/* 函数首次调用标记位 */
	static int s_bFunctionFirstCall = 1;

	/* 获取当前CPU的数据 */
	share_get_cpuRawData(&s_stCurrCpuInfo);
	/* 第一次获取CPU数据时，无上次记录，需作记录 */
	if (s_bFunctionFirstCall)
	{
		s_bFunctionFirstCall = 0;
		/* 将本次读取的数据复制一遍 */
		memcpy(&s_stLastCpuInfo, &s_stCurrCpuInfo, sizeof(CpuRawData_S));
		/* 进行递归即可获取首次数据 */
		return share_get_linux_cpu();
	}

	/* 计算CPU使用率 */
	double dLastTime, dCurrTime;
	double id, sd;
	double cpu_use;

	/* 第一次(用户+优先级+系统+空闲)的时间再赋给od */
	dLastTime = (double)(s_stLastCpuInfo.user +
						 s_stLastCpuInfo.nice +
						 s_stLastCpuInfo.system +
						 s_stLastCpuInfo.idle +
						 s_stLastCpuInfo.softirq +
						 s_stLastCpuInfo.iowait +
						 s_stLastCpuInfo.irq);
	/* 第二次(用户+优先级+系统+空闲)的时间再赋给od */
	dCurrTime = (double)(s_stCurrCpuInfo.user +
						 s_stCurrCpuInfo.nice +
						 s_stCurrCpuInfo.system +
						 s_stCurrCpuInfo.idle +
						 s_stCurrCpuInfo.softirq +
						 s_stCurrCpuInfo.iowait +
						 s_stCurrCpuInfo.irq);

	id = (double)(s_stCurrCpuInfo.idle); /* 用户第一次和第二次的时间之差再赋给id */
	sd = (double)(s_stLastCpuInfo.idle); /* 系统第一次和第二次的时间之差再赋给sd */
	if ((dCurrTime - dLastTime) != 0)
	{	
		/* ((用户+系统)*100)/(第一次和第二次的时间差)再赋给g_cpu_used */
		dCpuUse = 100.0 - ((id - sd)) / (dCurrTime - dLastTime) * 100.00;
	}
	else
	{
		dCpuUse = 0.0;
	}

	/* 记录本次数据 */
	memcpy(&s_stLastCpuInfo, &s_stCurrCpuInfo, sizeof(CpuRawData_S));
	return dCpuUse;
}

/* 获取Linux系统的内存使用率 */
int share_get_linux_mem(double *pMemUse)
{
	/* 内存总量为固定的 */
	static long s_nMemTotal = 0;
	char achFreadBuf[32] = {0};
	int nRet = 0;
	if (0 == s_nMemTotal)
	{
		/* 内存总量只需获取一次即可 */
		nRet = order_getInfo("cat /proc/meminfo | grep MemTotal |sed -n 1p | awk '{print $2}'",
							 achFreadBuf, sizeof(achFreadBuf));
		if(nRet <= 0)
		{
			return 1;
		}
		/* 读取出来的是KB */
		s_nMemTotal = atol(achFreadBuf);
	}

	bzero(achFreadBuf, sizeof(achFreadBuf));
	nRet = order_getInfo("cat /proc/meminfo | grep MemAvailable |sed -n 1p | awk '{print $2}'",
						 achFreadBuf, sizeof(achFreadBuf));
	if (nRet <= 0)
	{
		return -2;
	}
	long nMemAvailable = atol(achFreadBuf);
	if(pMemUse)
	{
		/* 计算内存使用率并输出 */
		*pMemUse = (1.0 - ((double)nMemAvailable / (double)s_nMemTotal)) * 100;
	}
	else
	{
		return -3;
	}

	return 0;
}

int share_getDisk_info(Disk_Info_t *diskInfo, const char *diskname)
{
	if ((NULL == diskInfo) || (NULL == diskname))
	{
		dlog(LOG_ERROR, "command share_getDisk_info fork info is NULL\n");
		return -1;
	}
	char command[512] = {0};
	char freadBuf[64] = {0};
	int i = 0;
	int j = 0;
	int nCount = 0;
	int ret = 0;
	char infoBuf[32] = {0};
	char udev_info[48] = {0};
	ret = order_getInfo("df -h|grep /opt/course|awk '{print $1}'", freadBuf, sizeof(freadBuf));
	if (ret <= 0)
	{
		return -1;
	}
	sscanf(freadBuf, "%[^ ]", udev_info);
	char *node = strstr(udev_info, "\n");
	*node = 0;
	memset(diskInfo, 0, sizeof(Disk_Info_t));
	sprintf(command, "df -h|grep %s|awk  '{print $2\",\"$3\",\"$5}'", udev_info);
	ret = order_getInfo(command, freadBuf, sizeof(freadBuf));
	if (ret <= 0)
	{
		return -1;
	}
	// dlog(LOG_ERROR,"udev_info[%s] command[%s] get disk info:%s\n\n\n\n\n",udev_info,command,freadBuf);
	char *p = NULL;
	float total = 0;
	float use = 0;
	float remain = 0;

	/*total user presion*/
	for (i = 0; i < ret; i++)
	{
		if (freadBuf[i] == ',')
		{
			if (nCount == 0) // total
			{
				p = strstr(infoBuf, "T");
				if (NULL == p)
				{
					p = strstr(infoBuf, "G");
					if (p != NULL)
					{
						*p = '\0';
						total = atof(infoBuf);
					}
					else
					{
						p = strstr(infoBuf, "M");
						if (p != NULL)
						{
							*p = '\0';
							total = atof(infoBuf);
							total = total / 1024;
						}
					}
				}
				else
				{
					*p = '\0';
					total = atof(infoBuf);
					total = total * 1024;
				}
				// printf("the fuck_u %f\n", use);
				// memcpy(diskInfo->total, infoBuf, sizeof(diskInfo->total));
			}
			else
			{
				/*use*/
				p = strstr(infoBuf, "T");
				if (NULL == p)
				{
					p = strstr(infoBuf, "G");
					if (p != NULL)
					{
						*p = '\0';
						use = atof(infoBuf);
					}
				}
				else
				{
					*p = '\0';
					use = atof(infoBuf);
					use = use * 1024;
				}

				// printf("the fuck_y %f %f\n", use, remain);
				sprintf(diskInfo->Available, "%.01fG", (total - use)); //可用的
				// printf("the fuck_k %f\n", remain);
				sprintf(diskInfo->total, "%.01fG", total); //总的
														   // memcpy(diskInfo->Available, infoBuf, sizeof(diskInfo->Available));
			}
			memset(infoBuf, 0, sizeof(infoBuf));
			j = 0;
			nCount++;
			continue;
		}
		if (freadBuf[i] == '%')
		{
			freadBuf[i] = '\0';
		}
		if (freadBuf[i] == '\n' || freadBuf[i] == '\0')
		{
			memcpy(diskInfo->useperson, infoBuf, sizeof(diskInfo->useperson));
			break;
		}
		infoBuf[j++] = freadBuf[i];
	}

	if (strcmp(diskInfo->total, "16.0M") == 0)
	{
		memset(diskInfo, 0, sizeof(Disk_Info_t));
		strcpy(diskInfo->useperson, "100");
	}

	return 0;
}

int share_catfork_info(Fork_Info_t *pForkInfo)
{
	char command[512] = {0};
	char freadBuf[64] = {0};
	int i = 0;
	int j = 0;
	int ret = 0;
	char infoBuf[16] = {0};
	if (pForkInfo == NULL)
	{
		printf("command share_cat fork info is NULL\n");
		return -1;
	}
	memset((void *)pForkInfo, 0, sizeof(Fork_Info_t));
	sprintf(command, "ps aux|grep %s|sed -n 1p|awk  '{print $4\",\"$3}'", pForkInfo->forkName);

	ret = order_getInfo(command, freadBuf, sizeof(freadBuf));
	if (ret <= 0)
	{
		return -1;
	}
	for (i = 0; i < ret; i++)
	{
		if (freadBuf[i] == ',')
		{
			pForkInfo->mem = atof(infoBuf);
			memset(infoBuf, 0, sizeof(infoBuf));
			j = 0;
			continue;
		}
		if (freadBuf[i] == '\n' || freadBuf[i] == '\0')
		{
			pForkInfo->cpu = atof(infoBuf);
			break;
		}
		infoBuf[j++] = freadBuf[i];
	}
	return 0;
}
int getlocalip(char *outip)
{
	int i = 0;
	int sockfd;
	struct ifconf ifconf;
	char buf[512] = {0};
	struct ifreq *ifreq;
	char *ip = NULL;
	//初始化ifconf
	ifconf.ifc_len = 512;
	ifconf.ifc_buf = buf;
	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
	{
		return -1;
	}
	ioctl(sockfd, SIOCGIFCONF, &ifconf); //获取所有接口信息
	close(sockfd);
	//接下来一个一个的获取IP地址
	ifreq = (struct ifreq *)buf;
	for (i = (ifconf.ifc_len / sizeof(struct ifreq)); i > 0; i--)
	{
		ip = inet_ntoa(((struct sockaddr_in *)&(ifreq->ifr_addr))->sin_addr);
		if (strcmp(ip, "127.0.0.1") == 0) //排除127.0.0.1，继续下一个
		{
			ifreq++;
			continue;
		}
		strcpy(outip, ip);
		return 0;
	}
	return -1;
}
/*
int share_get_seat(int *seat)
{
	if (NULL == seat)
	{
		dlog(LOG_ERROR, "enc_switch_seat param is err!\n");
		return -1;
	}

	if (access(SEAT_CONFIG, F_OK) == -1)
	{
		FILE *fp = fopen(SEAT_CONFIG, "w+");
		if (fp == NULL)
		{
			dlog(LOG_DEBUG, "enc_switch_seat fopen xml file failed\n");
			return -1;
		}
		else
		{
			dlog(LOG_DEBUG, "fuck share_get_seat\n");
			fprintf(fp, "5\n");
			*seat = FIV_SEAT;
			fclose(fp);
			return 0;
		}
	}
#if 1
	FILE *fpread = NULL;
	char tmp[10];
	int dlogStatus = 0;
	int ret = 0;

	fpread = fopen(SEAT_CONFIG, "r");
	if (NULL == fpread)
	{
		printf("fopen  SEAT_CONFIG config error!: %s\n", strerror(errno));
		ret = -1;
		*seat = FIV_SEAT;
		goto EXIT_FLAG;
	}

	memset(tmp, 0, 10);

	ret = fread(tmp, 1, 1, fpread);
	if (ret <= 0)
	{
		ret = -1;
		*seat = FIV_SEAT;
	}
	else
	{
		dlogStatus = atoi(tmp);
		*seat = dlogStatus;
	}

EXIT_FLAG:
	if (NULL != fpread)
		fclose(fpread);

	return ret;
#endif
}
*/

/*
int share_get_shoutInfo(int *status)
{
	if (NULL == status)
	{
		dlog(LOG_ERROR, "share_get_shoutInfo param is err!\n");
		return -1;
	}

	if (access(SHOUTDOWN_CONFIG, F_OK) == -1)
	{
		FILE *fp = fopen(SHOUTDOWN_CONFIG, "w+");
		if (fp == NULL)
		{
			dlog(LOG_DEBUG, "share_get_shoutInfo fopen xml file failed\n");
			return -1;
		}
		else
		{
			dlog(LOG_DEBUG, "fuck share_get_shoutInfo\n");
			fprintf(fp, "0\n");
			*status = 0;
			fclose(fp);
			return 0;
		}
	}
#if 1
	FILE *fpread = NULL;
	char tmp[10];
	int dlogStatus = 0;
	int ret = 0;

	fpread = fopen(SHOUTDOWN_CONFIG, "r");
	if (NULL == fpread)
	{
		printf("fopen  SHOUTDOWN_CONFIG config error!: %s\n", strerror(errno));
		ret = -1;
		*status = 0;
		goto EXIT_FLAG;
	}

	memset(tmp, 0, 10);

	ret = fread(tmp, 1, 1, fpread);
	if (ret <= 0)
	{
		ret = -1;
		*status = 0;
	}
	else
	{
		dlogStatus = atoi(tmp);
		*status = dlogStatus;
	}

EXIT_FLAG:
	if (NULL != fpread)
		fclose(fpread);

	return ret;
#endif
}
*/
/*
int share_set_shoutInfo(int status)
{
	if ((0 != status) && (1 != status))
	{
		dlog(LOG_ERROR, "share_set_shoutInfo param is err!\n");
		return -1;
	}

	if (access(SHOUTDOWN_CONFIG, F_OK) == -1)
	{
		FILE *fp = fopen(SHOUTDOWN_CONFIG, "w+");
		if (fp == NULL)
		{
			dlog(LOG_DEBUG, "share_get_shoutInfo fopen xml file failed\n");
			return -1;
		}
		else
		{
			dlog(LOG_DEBUG, "fuck share_get_shoutInfo\n");
			fprintf(fp, "0\n");
			fclose(fp);
			return 0;
		}
	}
#if 1
	FILE *fpread = NULL;
	char tmp[10];
	// int dlogStatus = 0;
	int ret = 0;

	fpread = fopen(SHOUTDOWN_CONFIG, "w+");
	if (NULL == fpread)
	{
		printf("fopen  SHOUTDOWN_CONFIG config error!: %s\n", strerror(errno));
		ret = -1;
		goto EXIT_FLAG;
	}

	memset(tmp, 0, 10);
	sprintf(tmp, "%d", status);

	ret = fwrite(tmp, 1, 1, fpread);
	if (ret <= 0)
	{
		ret = -1;
		printf("fwrite is err!!!!\n");
	}

EXIT_FLAG:
	if (NULL != fpread)
		fclose(fpread);

	return ret;
#endif
}

*/
/*
int share_set_rebootInfo(int status)
{
	if ((0 != status) && (1 != status))
	{
		dlog(LOG_ERROR, "share_set_rebootInfo param is err!\n");
		return -1;
	}

	if (access(REBOOT_CONFIG, F_OK) == -1)
	{
		FILE *fp = fopen(REBOOT_CONFIG, "w+");
		if (fp == NULL)
		{
			dlog(LOG_DEBUG, "share_set_rebootInfo fopen xml file failed\n");
			return -1;
		}
		else
		{
			dlog(LOG_DEBUG, "fuck share_set_rebootInfo\n");
			fprintf(fp, "1\n");
			fclose(fp);
			return 0;
		}
	}
#if 1
	FILE *fpread = NULL;
	char tmp[10];
	// int dlogStatus = 0;
	int ret = 0;

	fpread = fopen(REBOOT_CONFIG, "w+");
	if (NULL == fpread)
	{
		printf("fopen  REBOOT_CONFIG config error!: %s\n", strerror(errno));
		ret = -1;
		goto EXIT_FLAG;
	}

	memset(tmp, 0, 10);
	sprintf(tmp, "%d", status);

	ret = fwrite(tmp, 1, 1, fpread);
	if (ret <= 0)
	{
		ret = -1;
		printf("fwrite is err!!!!\n");
	}

EXIT_FLAG:
	if (NULL != fpread)
		fclose(fpread);

	return ret;
#endif
}
*/

int share_get_fileType(char *dir_file, char *file_name, char *file_suffix)
{
	if (NULL == dir_file || NULL == file_name || NULL == file_suffix)
	{
		dlog(LOG_ERROR, "the dir_file is err!!!!\n");
		return -1;
	}

	struct dirent *dir_info;
	DIR *dir_fd;

	if ((dir_fd = opendir(dir_file)) == NULL)
	{
		dlog(LOG_ERROR, "open dir failed! dir: %s", dir_file);
		return -1;
	}

	for (dir_info = readdir(dir_fd); NULL != dir_info; dir_info = readdir(dir_fd))
	{
		if (strstr(dir_info->d_name, "mp4") != NULL && strstr(dir_info->d_name, file_name) != NULL)
		{
			sprintf(file_suffix, "mp4");
			goto EXIT;
		}

		if (strstr(dir_info->d_name, "mov") != NULL && strstr(dir_info->d_name, file_name) != NULL)
		{
			sprintf(file_suffix, "mov");
			goto EXIT;
		}

		if (strstr(dir_info->d_name, "flv") != NULL && strstr(dir_info->d_name, file_name) != NULL)
		{
			sprintf(file_suffix, "flv");
			goto EXIT;
		}

		if (strstr(dir_info->d_name, "avi") != NULL && strstr(dir_info->d_name, file_name) != NULL)
		{
			sprintf(file_suffix, "avi");
			goto EXIT;
		}

		if (strstr(dir_info->d_name, "mkv") != NULL && strstr(dir_info->d_name, file_name) != NULL)
		{
			sprintf(file_suffix, "mkv");
			goto EXIT;
		}
	}

EXIT:
	closedir(dir_fd);
	return 0;
}

int get_system_time(System_Time_t *systemTime, char *timeBuf)
{
	struct tm *ptm;
	long ts;
	ts = time(NULL);
	ptm = localtime(&ts);

	if (systemTime != NULL)
	{
		memset(systemTime, 0, sizeof(System_Time_t));
		systemTime->year = ptm->tm_year + 1900; //年
		systemTime->month = ptm->tm_mon + 1;	//月
		systemTime->day = ptm->tm_mday;			//日
		systemTime->hour = ptm->tm_hour;		//时
		systemTime->minute = ptm->tm_min;		//分
		systemTime->second = ptm->tm_sec;		//秒
	}

	if (timeBuf != NULL)
	{
		memset(timeBuf, 0, 64);
		sprintf(timeBuf, "%d-%d-%d %d:%d:%d ", ptm->tm_year + 1900, ptm->tm_mon + 1, ptm->tm_mday, ptm->tm_hour, ptm->tm_min, ptm->tm_sec);
		// dlog(LOG_DEBUG, "Time = %s", timeBuf);
	}
	return 1;
}

int share_dirSize_info(int *dirInfo, const char *dirName)
{
	int ret = 0;
	char command[512] = {0};
	char freadBuf[64] = {0};
	if ((NULL == dirInfo) || (NULL == dirName))
	{
		dlog(LOG_ERROR, "command share_getDisk_info fork info is NULL\n");
		return -1;
	}
	sprintf(command, "df -h|du -s '%s'|awk '{print $1}'", dirName);
	ret = order_getInfo(command, freadBuf, sizeof(freadBuf));
	if (ret <= 0)
	{
		return -1;
	}
	// sscanf(freadBuf,"%[^ ]",udev_info);
	*dirInfo = atoi(freadBuf);
	return 0;
}

inline char *CharArrayToHexString(char *pOut, const int nMaxLen, const char *pInput, const int nInLen)
{
	const char *chHexList = "0123456789ABCDEF";
	int nIndex = 0;
	int i = 0, j = 0;
	for (i = 0, j = 0; i < nInLen; i++, j += 2)
	{
		nIndex = (pInput[i] & 0xf);
		pOut[i * 2 + 1] = chHexList[nIndex];
		nIndex = ((pInput[i] >> 4) & 0xf);
		pOut[i * 2] = chHexList[nIndex];
	}
	return pOut;
}

int check_hardDisk()
{
	int ret = -1;
	Disk_Info_t disk_info;
	memset(&disk_info, 0, sizeof(Disk_Info_t));
	ret = share_getDisk_info(&disk_info, "/dev/sda");

	if (strcmp(disk_info.total, "") == 0)
	{

		return -1;
	}

	if (ret == 0)
	{
		dlog(LOG_DEBUG, "check_hardDisk......%s %s %s\n", disk_info.total, disk_info.Available, disk_info.useperson);
		return atoi(disk_info.useperson);
	}

	return -1;
}

char *extract_charToInt(char *str, const char *separator, char *outPut)
{
	char *result = NULL;
	result = strtok(str, separator);
	while (result != NULL)
	{
		strcat(outPut, result);
		result = strtok(NULL, separator);
	}

	return outPut;
}
shareSpecialCharacter _Special_character_[] = // mysystme 转译
	{
		{"&", "\\&"}, //注意\要转义

};
char *share_strreplace(char **dest, char *src, const char *oldstr, const char *newstr, size_t len)
{
	int flags = 0;
	int lenPtr = 0;
	if (strcmp(oldstr, newstr) == 0)
	{
		return src;
	}
	int NewLen = strlen(newstr);
	char *needle, *tmp;
	*dest = src;
	tmp = (char *)malloc(len);
	while ((needle = strstr(*dest + lenPtr + flags * NewLen, oldstr)) && ((lenPtr = needle - *dest) <= len))
	{
		memset(tmp, 0, len);
		strncpy(tmp, *dest, needle - *dest);
		tmp[needle - *dest] = '\0';
		strcat(tmp, newstr);
		strcat(tmp, needle + strlen(oldstr));
		if (*dest != src && *dest != NULL)
		{
			free(*dest);
		}
		*dest = strdup(tmp);
		flags = 1;
	}
	free(tmp);
	return *dest;
}
static char *share_convert(char *cszText, size_t len)
{
	char *new = NULL;
	int nCirCul = 0;
	int nLen = sizeof(_Special_character_) / sizeof(_Special_character_[0]);
	new = share_strreplace(&new, cszText, _Special_character_[nCirCul].old, _Special_character_[nCirCul].new_, len);
	for (nCirCul = 1; nCirCul < nLen; nCirCul++)
	{
		if (strcmp(_Special_character_[nCirCul].old, _Special_character_[nCirCul].new_) == 0)
			continue;
		printf("new %s\n", share_strreplace(&new, new, _Special_character_[nCirCul].old, _Special_character_[nCirCul].new_, len));
	}
	return new;
}
typedef void (*sighandler_t)(int);
int mySystem(char *cmd_line)
{

	int cmdEndLen = -1;
	int ret = 0;
	char *new = NULL;
	if (cmd_line == NULL)
	{
		return -1;
	}
	char *cmdPtr = NULL;
	char *cmd = NULL;
	char *ptr = strrchr(cmd_line, '&');
	if (ptr != NULL)
	{
		if (strstr(ptr - 1, " &") != NULL)
		{
			cmdEndLen = ptr - cmd_line;
			cmd = malloc(strlen(cmd_line) + 1);
			memset(cmd, 0, strlen(cmd_line) + 1);
			memcpy(cmd, cmd_line, cmdEndLen); //去掉&
			// cmd[cmdEndLen] = '0';
			cmdPtr = cmd;
		}
		else
		{
			cmdEndLen = -1;
			cmdPtr = cmd_line;
		}
	}
	else
	{
		cmdEndLen = -1;
		cmdPtr = cmd_line;
	}
	new = share_convert(cmdPtr, (strlen(cmd_line) + 1) * 4);

	if (cmdEndLen != -1 && new != NULL)
	{
		strcat(new, " &");
	}
	sighandler_t old_handler;
	old_handler = signal(SIGCHLD, SIG_DFL);
	ret = system(new);
	signal(SIGCHLD, old_handler);
	printf("cmd_line:%s\n,new:%s\n\n", cmd_line, new);
	if (new != cmd_line)
	{
		r_free(new);
	}
	if (cmdPtr != cmd_line && new != cmdPtr)
	{
		r_free(cmdPtr);
	}
	return ret;
}

/*
*@description 获取文件大小
*@Author: wxz
**参数：pFileName ：文件名，在/opt/course路径下的
*@return lFileSize：文件大小
	   -1： 获取失败
*/
long long int get_movie_file_size(const char *pFileName)
{
	long long int lFileSize;
	if ((NULL == pFileName))
	{
		dlog(LOG_ERROR, "command pFileName fork info is NULL\n");
		return -1;
	}
	char strCommand[512] = {0};
	char strFreadBuf[64] = {0};
	char strFilePath[256] = {0};
	int nCount = 0;
	int nRet = 0;

	sprintf(strFilePath, "/opt/course/%s", pFileName);
	int nFd = access(strFilePath, F_OK); //判断是否存在设备
	if (nFd < 0)
	{
		dlog(LOG_ERROR, "%s do not exist NULL\n", strFilePath);
		return -1;
		;
	}

	sprintf(strCommand, "du -s %s|awk '{print $1}'", strFilePath);
	nRet = order_getInfo(strCommand, strFreadBuf, sizeof(strFreadBuf));
	if (nRet <= 0)
	{
		return -1;
	}

	lFileSize = atoll(strFreadBuf);
	// 	lFileSize *= 1024; //byte

	// 	printf("%lld\n",lFileSize);

	return lFileSize;
}

void get_sys_time(Date_Time_Info_t *dtinfo)
{
	long ts;
	struct tm *ptm = NULL;
#if 1
	ts = time(NULL);
	ptm = localtime((const time_t *)&ts);
	dtinfo->year = ptm->tm_year + 1900;
	dtinfo->month = ptm->tm_mon + 1;
	dtinfo->mday = ptm->tm_mday;
	dtinfo->hours = ptm->tm_hour;
	dtinfo->min = ptm->tm_min;
	dtinfo->sec = ptm->tm_sec;
#else
	struct rtc_time rtc_tm;
	get_rtc_clock(&rtc_tm);
	dtinfo->year = rtc_tm.tm_year + 1900;
	dtinfo->month = rtc_tm.tm_mon + 1;
	dtinfo->mday = rtc_tm.tm_mday;
	dtinfo->hours = rtc_tm.tm_hour;
	dtinfo->min = rtc_tm.tm_min;
	dtinfo->sec = rtc_tm.tm_sec;
#endif
}

double time_get_ms()
{
    struct timeval tv;
    double ret;
    int err;

    err=gettimeofday(&tv,0);
    if(err==0)
    {
        ret=tv.tv_sec*1000.0+tv.tv_usec*1.0/1000;
        //ret maybe is NAN
        if(ret!=ret)
        {
            dlog(LOG_DEBUG,"NAN(%.0f,sec=%.d,usec=%.d).\n",ret,(int)tv.tv_sec,(int)tv.tv_usec);
            ret=0;
        }
    }else
    {
        perror(__FUNCTION__);
        ret=0;
    }
    return ret;
}

#endif