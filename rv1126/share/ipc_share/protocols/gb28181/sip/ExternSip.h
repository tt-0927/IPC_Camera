/*
 * @Author       : EasonLu
 * @Date         : 2025-02-12 15:55:59
 * @LastEditors  : EasonLu
 * @LastEditTime : 2025-02-12 17:20:18
 * @FilePath     : ExternSip.h
 * @Description  : 包含第三方的sip头文件
 */
#ifndef _EXTERN_SIP_H_
#define _EXTERN_SIP_H_

/* osip2/osip.h 使用 struct timeval 但自身未包含 sys/time.h；
 * uclibc 工具链（富瀚 gcc 6.5）不会经其他系统头间接带入，须显式前置 */
#include <sys/time.h>

#ifdef __cplusplus
extern "C" {
#endif

#include "eXosip2/eXosip.h"
#include "osip2/osip.h"

#ifdef __cplusplus
}
#endif
#endif
