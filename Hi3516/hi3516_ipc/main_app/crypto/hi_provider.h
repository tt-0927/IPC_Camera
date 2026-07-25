/**
 * @FilePath     : hi_provider.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-05-27 11:32:29
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-05-27 15:19:41
 * @Description  : HiSilicon 硬件加速 OpenSSL Provider 头文件
 */

#ifndef __HI_PROVIDER_H__
#define __HI_PROVIDER_H__

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief   : 注册 HiSilicon 硬件 Provider
 * @return  : 0 成功，非 0 失败
 */
int hi_provider_register();

/**
 * @brief   : 注销 HiSilicon 硬件 Provider
 * @return  : 0 成功，非 0 失败
 */
int hi_provider_unregister();

#ifdef __cplusplus
}
#endif
#endif
