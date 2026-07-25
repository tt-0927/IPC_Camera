/**
 * @FilePath     : cipher_context.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-05-27 08:56:36
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-05-27 15:01:30
 * @Description  : 安全子系统统一上下文封装头文件
 */
#ifndef __CIPHER_CONTEXT_H__
#define __CIPHER_CONTEXT_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "ot_mpi_cipher.h"
#include "mpi_common.h"

typedef struct _CipherContextNeedParam_S {
    /* 是否启用 HASH 子模块，用于 SM3 等摘要算法的硬件加速 */
    td_bool bEnableHash;
    /* 是否启用 SYMC（对称密码）子模块，用于 SM4 等对称加解密的硬件加速 */
    td_bool bEnableSymc;
    /* 是否启用 PKE（公钥引擎）子模块，用于 SM2 等非对称算法的硬件加速 */
    td_bool bEnablePke;
    /* 是否启用 TRNG（真随机数发生器）子模块，用于硬件随机数生成 */
    td_bool bEnableTrng;
} CipherContextNeedParam_S;

typedef struct _CipherContext_S CipherContext_S;

struct _CipherContext_S {
    /* 调用方声明本上下文需要启用的安全子模块，init 时按需初始化 */
    CipherContextNeedParam_S stNeedParam;
    /* HASH 子模块初始化状态，用于避免重复初始化和控制释放顺序 */
    td_bool bHashInited;
    /* SYMC 子模块初始化状态，用于后续 SM4-CBC 硬件路径收口 */
    td_bool bSymcInited;
    /* PKE 子模块初始化状态，用于后续 SM2 硬件桥接收口 */
    td_bool bPkeInited;

    /**
     * @brief   : 初始化安全子系统上下文中声明需要的硬件子模块
     * @param    {CipherContext_S*} pHandle：安全子系统上下文句柄
     * @return   {int} TD_SUCCESS：成功，非 TD_SUCCESS：失败
     */
    int (*cipherContext_init)(CipherContext_S *pHandle);
    /**
     * @brief   : 按反向顺序去初始化已启用的硬件子模块
     * @param    {CipherContext_S*} pHandle：安全子系统上下文句柄
     * @return   {int} TD_SUCCESS：成功，非 TD_SUCCESS：失败
     */
    int (*cipherContext_uninit)(CipherContext_S *pHandle);
    /**
     * @brief   : 使用硬件 HASH 子模块执行一次性 SM3 摘要计算
     * @param    {CipherContext_S*} pHandle：安全子系统上下文句柄
     * @param    {td_u8*} pData：输入数据指针，长度为 0 时允许为空
     * @param    {td_u32} u32Len：输入数据长度
     * @param    {td_u8*} pOut：输出摘要缓冲区
     * @param    {td_u32} u32OutSize：输出摘要缓冲区大小
     * @param    {td_u32*} pOutLen：实际输出摘要长度
     * @return   {int} TD_SUCCESS：成功，非 TD_SUCCESS：失败
     */
    int (*cipherContext_sm3_compute)(CipherContext_S *pHandle, const td_u8 *pData, td_u32 u32Len, td_u8 *pOut, td_u32 u32OutSize, td_u32 *pOutLen);
    /**
     * @brief   : 使用硬件 TRNG 获取随机字节
     * @param    {CipherContext_S*} pHandle：安全子系统上下文句柄
     * @param    {td_u8*} pOut：输出随机字节缓冲区
     * @param    {td_u32} u32Len：请求随机字节长度
     * @return   {int} TD_SUCCESS：成功，非 TD_SUCCESS：失败
     */
    int (*cipherContext_trng_get_bytes)(CipherContext_S *pHandle, td_u8 *pOut, td_u32 u32Len);
    /**
     * @brief   : 使用硬件 SYMC 子模块执行 SM4-CBC raw block 加解密
     * @param    {CipherContext_S*} pHandle：安全子系统上下文句柄
     * @param    {td_u8*} pKey：16 字节 SM4 明文密钥
     * @param    {td_u32} u32KeyLen：密钥长度，必须为 16
     * @param    {td_u8*} pIv：16 字节 CBC IV
     * @param    {td_u32} u32IvLen：IV 长度，必须为 16
     * @param    {td_u8*} pInput：输入数据，长度必须 16 字节对齐
     * @param    {td_u32} u32InputLen：输入数据长度
     * @param    {td_u8*} pOutput：输出缓冲区
     * @param    {td_u32} u32OutputSize：输出缓冲区大小
     * @param    {td_u32*} pOutputLen：实际输出长度
     * @param    {td_bool} bEncrypt：TD_TRUE 加密，TD_FALSE 解密
     * @return   {int} TD_SUCCESS：成功，非 TD_SUCCESS：失败
     * @note    : 该接口只提供硬件 raw block 能力，PKCS#7 padding 与 Base64 编码由上层 Provider 处理。
     */
    int (*cipherContext_sm4_cbc_crypt)(CipherContext_S *pHandle,
                                       const td_u8 *pKey,
                                       td_u32 u32KeyLen,
                                       const td_u8 *pIv,
                                       td_u32 u32IvLen,
                                       const td_u8 *pInput,
                                       td_u32 u32InputLen,
                                       td_u8 *pOutput,
                                       td_u32 u32OutputSize,
                                       td_u32 *pOutputLen,
                                       td_bool bEncrypt);
    /**
     * @brief   : 使用硬件 PKE 生成 SM2 raw 密钥对
     * @param    {CipherContext_S*} pHandle：安全子系统上下文句柄
     * @param    {td_u8*} pPrivKey：输出 32 字节私钥
     * @param    {td_u32} u32PrivKeyLen：私钥缓冲区长度，必须为 32
     * @param    {td_u8*} pPubX：输出 32 字节公钥 X
     * @param    {td_u32} u32PubXLen：公钥 X 缓冲区长度，必须为 32
     * @param    {td_u8*} pPubY：输出 32 字节公钥 Y
     * @param    {td_u32} u32PubYLen：公钥 Y 缓冲区长度，必须为 32
     * @return   {int} TD_SUCCESS：成功，非 TD_SUCCESS：失败
     */
    int (*cipherContext_sm2_keygen)(CipherContext_S *pHandle,
                                    td_u8 *pPrivKey,
                                    td_u32 u32PrivKeyLen,
                                    td_u8 *pPubX,
                                    td_u32 u32PubXLen,
                                    td_u8 *pPubY,
                                    td_u32 u32PubYLen);
    /**
     * @brief   : 使用硬件 PKE 计算 SM2 签名验签前的 ZA HASH
     * @param    {CipherContext_S*} pHandle：安全子系统上下文句柄
     * @param    {td_u8*} pId：SM2 ID
     * @param    {td_u32} u32IdLen：SM2 ID 长度
     * @param    {td_u8*} pPubX：32 字节公钥 X
     * @param    {td_u32} u32PubXLen：公钥 X 长度
     * @param    {td_u8*} pPubY：32 字节公钥 Y
     * @param    {td_u32} u32PubYLen：公钥 Y 长度
     * @param    {td_u8*} pMsg：待签名消息
     * @param    {td_u32} u32MsgLen：消息长度
     * @param    {td_u8*} pHash：输出 32 字节 hash
     * @param    {td_u32} u32HashLen：hash 缓冲区长度，必须为 32
     * @return   {int} TD_SUCCESS：成功，非 TD_SUCCESS：失败
     */
    int (*cipherContext_sm2_dsa_hash)(CipherContext_S *pHandle,
                                      const td_u8 *pId,
                                      td_u32 u32IdLen,
                                      const td_u8 *pPubX,
                                      td_u32 u32PubXLen,
                                      const td_u8 *pPubY,
                                      td_u32 u32PubYLen,
                                      const td_u8 *pMsg,
                                      td_u32 u32MsgLen,
                                      td_u8 *pHash,
                                      td_u32 u32HashLen);
    /**
     * @brief   : 使用硬件 PKE 对 SM2 ZA HASH 签名
     * @param    {CipherContext_S*} pHandle：安全子系统上下文句柄
     * @param    {td_u8*} pPrivKey：32 字节私钥
     * @param    {td_u32} u32PrivKeyLen：私钥长度
     * @param    {td_u8*} pHash：32 字节 ZA HASH
     * @param    {td_u32} u32HashLen：ZA HASH 长度
     * @param    {td_u8*} pSigR：输出 32 字节 r
     * @param    {td_u32} u32SigRLen：r 缓冲区长度
     * @param    {td_u8*} pSigS：输出 32 字节 s
     * @param    {td_u32} u32SigSLen：s 缓冲区长度
     * @return   {int} TD_SUCCESS：成功，非 TD_SUCCESS：失败
     */
    int (*cipherContext_sm2_sign)(CipherContext_S *pHandle,
                                  const td_u8 *pPrivKey,
                                  td_u32 u32PrivKeyLen,
                                  const td_u8 *pHash,
                                  td_u32 u32HashLen,
                                  td_u8 *pSigR,
                                  td_u32 u32SigRLen,
                                  td_u8 *pSigS,
                                  td_u32 u32SigSLen);
    /**
     * @brief   : 使用硬件 PKE 验证 SM2 ZA HASH 签名
     * @param    {CipherContext_S*} pHandle：安全子系统上下文句柄
     * @param    {td_u8*} pPubX：32 字节公钥 X
     * @param    {td_u32} u32PubXLen：公钥 X 长度
     * @param    {td_u8*} pPubY：32 字节公钥 Y
     * @param    {td_u32} u32PubYLen：公钥 Y 长度
     * @param    {td_u8*} pHash：32 字节 ZA HASH
     * @param    {td_u32} u32HashLen：ZA HASH 长度
     * @param    {td_u8*} pSigR：32 字节 r
     * @param    {td_u32} u32SigRLen：r 长度
     * @param    {td_u8*} pSigS：32 字节 s
     * @param    {td_u32} u32SigSLen：s 长度
     * @return   {int} TD_SUCCESS：成功，非 TD_SUCCESS：失败
     */
    int (*cipherContext_sm2_verify)(CipherContext_S *pHandle,
                                    const td_u8 *pPubX,
                                    td_u32 u32PubXLen,
                                    const td_u8 *pPubY,
                                    td_u32 u32PubYLen,
                                    const td_u8 *pHash,
                                    td_u32 u32HashLen,
                                    const td_u8 *pSigR,
                                    td_u32 u32SigRLen,
                                    const td_u8 *pSigS,
                                    td_u32 u32SigSLen);
    /**
     * @brief   : 使用硬件 PKE 执行 SM2 raw 公钥加密
     * @param    {CipherContext_S*} pHandle：安全子系统上下文句柄
     * @param    {td_u8*} pPubX：32 字节公钥 X
     * @param    {td_u32} u32PubXLen：公钥 X 长度
     * @param    {td_u8*} pPubY：32 字节公钥 Y
     * @param    {td_u32} u32PubYLen：公钥 Y 长度
     * @param    {td_u8*} pPlain：明文
     * @param    {td_u32} u32PlainLen：明文长度
     * @param    {td_u8*} pCipher：输出海思 raw 密文，长度为明文长度 + 97
     * @param    {td_u32} u32CipherSize：密文缓冲区大小
     * @param    {td_u32*} pCipherLen：实际密文长度
     * @return   {int} TD_SUCCESS：成功，非 TD_SUCCESS：失败
     */
    int (*cipherContext_sm2_encrypt)(CipherContext_S *pHandle,
                                     const td_u8 *pPubX,
                                     td_u32 u32PubXLen,
                                     const td_u8 *pPubY,
                                     td_u32 u32PubYLen,
                                     const td_u8 *pPlain,
                                     td_u32 u32PlainLen,
                                     td_u8 *pCipher,
                                     td_u32 u32CipherSize,
                                     td_u32 *pCipherLen);
    /**
     * @brief   : 使用硬件 PKE 执行 SM2 raw 私钥解密
     * @param    {CipherContext_S*} pHandle：安全子系统上下文句柄
     * @param    {td_u8*} pPrivKey：32 字节私钥
     * @param    {td_u32} u32PrivKeyLen：私钥长度
     * @param    {td_u8*} pCipher：海思 raw 密文，长度为明文长度 + 97
     * @param    {td_u32} u32CipherLen：密文长度
     * @param    {td_u8*} pPlain：输出明文
     * @param    {td_u32} u32PlainSize：明文缓冲区大小
     * @param    {td_u32*} pPlainLen：实际明文长度
     * @return   {int} TD_SUCCESS：成功，非 TD_SUCCESS：失败
     */
    int (*cipherContext_sm2_decrypt)(CipherContext_S *pHandle,
                                     const td_u8 *pPrivKey,
                                     td_u32 u32PrivKeyLen,
                                     const td_u8 *pCipher,
                                     td_u32 u32CipherLen,
                                     td_u8 *pPlain,
                                     td_u32 u32PlainSize,
                                     td_u32 *pPlainLen);
};

/**
 * @brief   : 分配安全子系统统一上下文并绑定能力函数表
 * @param    {CipherContextNeedParam_S} stNeedParam：调用方声明需要启用的子模块
 * @return   {CipherContext_S*} 成功返回上下文指针，失败返回 NULL
 */
CipherContext_S *cipherContext_alloc(CipherContextNeedParam_S stNeedParam);

/**
 * @brief   : 释放安全子系统统一上下文内存
 * @param    {CipherContext_S*} pHandle：安全子系统上下文句柄
 * @return   {void}
 */
void cipherContext_release(CipherContext_S *pHandle);

#ifdef __cplusplus
}
#endif

#endif
