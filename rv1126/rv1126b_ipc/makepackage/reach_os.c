#include <sys/resource.h>
#include "reach_os.h"
#include "stdint.h"

int32_t r_setpriority(int32_t which, int32_t who, int32_t prio)
{
	return setpriority(which, who, prio);
}

void *r_malloc(size_t size)
{
	return malloc(size);
}

void r_free(void *ptr)
{
	free(ptr);
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
	return memcpy(dest, src, n);
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

int32_t r_socket(int32_t domain, int32_t type, int32_t protocol)
{
	return socket(domain, type, protocol);
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
int32_t r_close(int32_t fd)
{
	return close(fd);
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

// 获取毫秒级别
uint32_t get_run_time(void)
{
	unsigned int msec;
	struct timespec tp;
	clock_gettime(CLOCK_MONOTONIC, &tp);
	msec = tp.tv_sec;
	msec = msec * 1000 + tp.tv_nsec / 1000000;
	return msec;
}

// 获取 1/10 毫秒级别
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
		//	nslog(NS_ERROR, "msgget error:[%d], errmsg = %s!\n", errno, strerror(errno));
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

	if (stat(filename, &f_stat) == -1)
	{
		printf("file is not exit\n");
		return -1;
	}
	printf("get_file_size is%ld\n", (int64_t)f_stat.st_size);
	return (int64_t)f_stat.st_size;
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

static char *basename(char *path)
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

				ret_val = unlink(LocalFilePath);

				if (ret_val == -1)
				{
					printf("DelDirectory unlink [ret_val = %d][unlink File = %s]\n", ret_val, LocalFilePath);
					ret = -1;
					break;
				}
			}
		}

	ret_val = rmdir(Pathh);

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

	return unlink(FileName);
}
