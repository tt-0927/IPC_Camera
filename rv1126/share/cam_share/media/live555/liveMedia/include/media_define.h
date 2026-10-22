/**
 * @FilePath     : media_define.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2025-12-22 11:41:35
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2025-12-22 14:58:31
 * @Description  : liveMedia相关工具宏定义
 */
#ifndef _MEDIA_DEFINE_H_
#define _MEDIA_DEFINE_H_

#ifdef __cplusplus
extern "C" {
#endif

/*封装printf*/
#define live_log(fmt...) \
    do { \
        printf("\033[1;34m[RtspServer][Func]:%s [Line]:%d ", __FUNCTION__, __LINE__); \
        printf(fmt); \
        printf("\033[0;39m\n"); \
        fflush(stdout); \
    } while (0)

#ifdef __cplusplus
}
#endif
#endif