
#ifndef __REACH_OS__
#define __REACH_OS__

#include "stdint.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <malloc.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <signal.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/time.h>
#include <time.h>
#include <errno.h>
#include <dirent.h>


#define INVALID_MSGTYPE								(-1)


typedef struct inter_hands
{
	int32_t m_monitor_socket;
	void *m_retain;
	int32_t *m_flag;
}inter_hand_t;

typedef struct _msgque_{
	long msgtype;
	int8_t *msgbuf;
}msgque;


#define  ARR_SIZE(a) (sizeof(a)/sizeof(a[0]))

#define _XOPEN_SOURCE 600
typedef	void Sigfunc(int);
#define r_snprintf	snprintf
#define r_sprintf	sprintf
void *r_malloc(size_t size);
void r_free(void *ptr);
void *r_memset(void *s, int32_t c, size_t n);
int8_t *r_strcat(int8_t *dest, const int8_t *src);
int8_t *r_strncat(int8_t *dest, const int8_t *src, size_t n);
int8_t *r_strstr(const int8_t *haystack, const int8_t *needle);
int8_t *r_strrchr(const int8_t *s, int32_t c);
void r_bzero(void *s, size_t n);
void *r_memalign(size_t boundary, size_t size);

int32_t r_pthread_create(pthread_t * thread, pthread_attr_t * attr,
                         void * (*start_routine)(void *), void * arg);
ssize_t r_recv(int32_t socket, void *buffer, size_t len, int32_t flags);
ssize_t r_recvfrom(int32_t socket, void *buffer, size_t len, int32_t flags, struct sockaddr *from, socklen_t *from_len);
ssize_t r_send(int32_t socket, const void *buffer, size_t len, int32_t flags);
void *r_memcpy(void *dest, const void *src, size_t n);
void *r_cmemcpy(int8_t *dest, int8_t *src, size_t n, int32_t (*condition)(void *));
int8_t *r_strcpy(int8_t *dest, const int8_t *src);
int8_t * r_strncpy(int8_t *dest, int8_t *src, size_t n);

int32_t r_getsockname(int32_t socket, struct sockaddr *name, socklen_t *name_len);
uint16_t r_htons(uint16_t host_short);
int8_t r_inet_aton(const int8_t *cp, struct in_addr *inp);
ssize_t r_sendto(int32_t socket, const void *buffer, size_t len, int32_t flags,
                 const struct sockaddr *to, socklen_t tolen);
int32_t r_socket(int32_t domain, int32_t type, int32_t protocol);
int32_t r_setsockopt(int32_t socket, int32_t level, int32_t opt_name, const void *opt_val,
                     socklen_t opt_len);
size_t r_strlen(const int8_t *s);
int32_t r_connect(int32_t socket, const struct sockaddr *serv_addr, socklen_t addr_len);
int32_t r_close(int32_t fd);
uint16_t r_ntohs(uint16_t net_short);
int32_t r_pthread_detach(pthread_t th);
pthread_t r_pthread_self(void);
uint32_t r_sleep(int32_t second);
void r_usleep(int32_t u_second);
int32_t r_strcmp(const int8_t *s1, const int8_t *s2);
int32_t r_strncmp(const int8_t *s1, const int8_t *s2, size_t n);
int32_t r_memcmp(const void *s1, const void *s2, size_t n);
Sigfunc *Signal(int32_t signo, Sigfunc *func);
void ms_delay(int32_t ms);
int32_t str_is_num(int8_t *str);
uint32_t get_time(void);
uint32_t get_run_time(void);
int32_t r_system(const int8_t * string);
int32_t r_mkdir(const int8_t *pathname, mode_t mode);
int32_t r_pthread_mutex_lock(pthread_mutex_t *mutex);
int32_t r_pthread_mutex_unlock(pthread_mutex_t *mutex);
int32_t r_bind(int socket, const struct sockaddr *address, socklen_t address_len);
int32_t r_listen(int sockfd, int backlog);
int32_t r_connect(int32_t socket, const struct sockaddr *serv_addr, socklen_t addr_len);
int32_t r_accept(int32_t sockfd, struct sockaddr *addr, socklen_t *addrlen);
int32_t r_select(int32_t maxfdp1, fd_set *readset, fd_set *writeset, fd_set *exceptset, struct timeval *timeout);
int32_t r_pthread_join(pthread_t thread, void **retval);
int32_t r_strcmp(const int8_t *s1, const int8_t *s2);
int32_t r_strncmp(const int8_t *s1, const int8_t *s2, size_t n);
int32_t r_memcmp(const void *s1, const void *s2, size_t n);
FILE * r_fopen(const int8_t * path, const int8_t * mode);
uint32_t  r_htonl(uint32_t  hostlong);
int32_t r_getsockopt(int32_t sockfd, int32_t level, int32_t optname, void *optval, socklen_t *optlen);
int32_t r_msg_send(int32_t msqid, msgque *msgp, int32_t length, int32_t msgflg);
int32_t r_msg_recv(int32_t msqid, msgque *msgp, int32_t length, long msgtyp, int32_t msgflg);
int32_t r_pthread_attr_init(pthread_attr_t *attr);
int32_t r_setpriority(int32_t which, int32_t who, int32_t prio);
int32_t r_pthread_attr_destroy(pthread_attr_t *attr);
int32_t r_open(const int8_t *pathname, int32_t flags);
ssize_t r_write(int32_t fd, const void *buf, size_t count);
ssize_t r_read(int32_t fd, void *buf , size_t count);



typedef struct _localtime {
	int tm_sec;	//seconds
	int tm_min;	//minutes
	int tm_hour;	//hours
	int tm_mday;	//day of the month
	int tm_mon;	//month
	int tm_year;	//year
} localtime_t;

 void get_localtime(localtime_t *t);
int32_t r_msg_create_u(int32_t key);
int32_t r_msg_create_key(int8_t *path, int32_t num);
int32_t  get_msg_num(int32_t msgid);
int32_t filecpy(int8_t *dst_filename, int8_t *src_filename);
int64_t  get_file_size(char *filename);
int32_t dete_dir_exists(int8_t *dirname);

int32_t r_CopyDir(int8_t *SrcPath, int8_t *DstPath);
int32_t DelDir(int8_t *SrcPath);
int32_t CopyFile(int8_t* LocalFilePath, int8_t* DstFilePath);
int32_t DelFile(int8_t *FileName);




#endif //__REACH_OS__

