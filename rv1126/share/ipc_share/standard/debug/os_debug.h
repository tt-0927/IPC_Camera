
#ifndef _OS_DEBUG_H_
#define _OS_DEBUG_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include "os.h"

// printf wrapper that can be turned on and off by defining/undefining
#ifdef OS_DEBUG_MODE
           
  #define OS_printf(...)  do { printf("\n\r [host] " __VA_ARGS__); fflush(stdout); } while(0)

// #define __KLOCWORK__
#ifdef __KLOCWORK__
  #define OS_assert(x)  \
  { \
    if( (x) == 0) { \
      fprintf(stderr, " ASSERT (%s|%s|%d)\r\n", __FILE__, __func__, __LINE__); \
      abort();\
    } \
  }
#else
  #define OS_assert(x)  \
  { \
    if( (x) == 0) { \
      fprintf(stderr, " ASSERT (%s|%s|%d)\r\n", __FILE__, __func__, __LINE__); \
      while (getchar()!='q')  \
        ; \
    } \
  } 
#endif
    #define OS_UTILS_assert(x)   OS_assert(x)
               
#define OS_DEBUG \
  fprintf(stderr, " %s:%s:%d Press Any key to continue !!!", __FILE__, __func__, __LINE__); 


#define OS_DEBUG_WAIT \
  OS_DEBUG \
  getchar();

#define OS_COMPILETIME_ASSERT(condition)                                       \
                   do {                                                         \
                       typedef char ErrorCheck[((condition) == TRUE) ? 1 : -1]; \
                   } while(0)

#else
  
  #define OS_printf(...)
  #define OS_assert(x)
  #define UTILS_assert(x)  
  #define OS_DEBUG
  #define OS_DEBUG_WAIT
#endif

// printf wrapper that can be used to display errors. Prefixes all text with
// "ERROR" and inserts the file and line number of where in the code it was
// called
#define OS_ERROR(...) \
  do \
  { \
  fprintf(stderr, " ERROR  (%s|%s|%d): ", __FILE__, __func__, __LINE__); \
  fprintf(stderr, __VA_ARGS__); \
  } \
  while(0);

/*
 * 设置断言，assert;
 * 当ret != 0时，进入循环等待用户输入q，程序退出
 * 当ret == 0时，程序继续进行
 *
 * */
#define OS_assertSuccess(ret)  OS_assert(ret==OS_SOK)



typedef void (*OS_log)(const char *format, ...);//用于输出调试信息的函数指针

#ifdef WIN32
#define OS_printf_log(logFun,format,...) \
    if(logFun)\
    {\
        logFun("\033[31m[%s:%d]\033[0m" format "\r\n",__func__, __LINE__,__VA_ARGS__);\
    }else{\
        printf("\033[31m[%s:%d]\033[0m" format "\r\n",__func__, __LINE__,__VA_ARGS__);\
    }
#else
/*打印函数*/
#define OS_printf_log(logFun,format,args...)\
	if(logFun)\
	{\
		logFun("\033[31m[%s:%d] \033[0m" format "\r\n",__func__, __LINE__,##args);\
	}else{\
		printf("\033[31m[%s:%d] \033[0m" format "\r\n",__func__, __LINE__,##args);\
	}

#endif


#ifdef __cplusplus
}
#endif

#endif  //_OS_DEBUG_H_



