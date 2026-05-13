/**
 * @FilePath     : codec_g711.h
 * @Author       : zhouzirui
 * @Date         : 2025-05-14 11:19:31
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2025-06-09 14:21:37
 * @Description  : g711编解码
 */

#ifndef _CODEC_G711_H_
#define _CODEC_G711_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>
#include <stdio.h>

#define CODEC_OK 0
#define CODEC_NG -1
#define CODEC_EINVAL -2 /* 无效参数 */

/* S16 PCM 数据是大端序还是小端序 */
#define CODEC_G711_ENDIAN_LITTLE      (0) // 小端序
#define CODEC_G711_ENDIAN_BIG         (1) // 大端序
#define CODEC_G711_ENDIAN             CODEC_G711_ENDIAN_LITTLE  // 默认小端序

#define CODEC_RETVAL(condition, ret, fmt, msg...)                   \
    if (condition)                                                      \
    {                                                                   \
        printf(fmt "\r\n", ##msg);                                      \
        return ret;                                                     \
    }

#define CODEC_RETVAL_NOMSG(condition, ret)                          \
    if (condition)                                                      \
    {                                                                   \
        return ret;                                                     \
    }

/**
 * @brief G.711 编码类型
 */
typedef enum CODEC_G711_TYPE
{
    CODEC_G711_TYPE_ALAW = 0,
    CODEC_G711_TYPE_ULAW,
} CODEC_G711_TYPE_E;

#define CODEC_G711_TYPE_STR(en)     \
    (en == CODEC_G711_TYPE_ALAW) ? "ALAW" : \
    (en == CODEC_G711_TYPE_ULAW) ? "ULAW" : "UNKNOW"

/**
 * @brief G.711A编码
 * @param pInputBuf 输入的PCM数据
 * @param pOutputBuf 输出的G.711A数据
 * @param unLength 输入的PCM数据长度
 */
int codec_g711a_encode(uint8_t *pInputBuf, uint8_t *pOutputBuf, uint32_t unLength);

/**
 * @brief G.711U编码
 * @param pInputBuf 输入的PCM数据
 * @param pOutputBuf 输出的G.711U数据
 * @param unLength 输入的PCM数据长度
 */
int codec_g711u_encode(uint8_t *pInputBuf, uint8_t *pOutputBuf, uint32_t unLength);

/**
 * @brief G.711编码
 * @param type G.711编码类型
 * @param pInputBuf 输入的PCM数据
 * @param pOutputBuf 输出的G.711数据
 * @param unLength 输入的PCM数据长度
 */
int codec_g711_encode(CODEC_G711_TYPE_E type, uint8_t *pInputBuf, uint8_t *pOutputBuf, uint32_t unLength);

/**
 * @brief G.711A编码单个采样点
 * @param pInputBuf 输入的PCM数据
 * @param pOutputBuf 输出的G.711A数据
 */
int codec_g711a_encode_one(int16_t pInputBuf, uint8_t *pOutputBuf);

/**
 * @brief G.711U编码单个采样点
 * @param pInputBuf 输入的PCM数据
 * @param pOutputBuf 输出的G.711U数据
 */
int codec_g711u_encode_one(int16_t pInputBuf, uint8_t *pOutputBuf);

/**
 * @brief G.711编码单个采样点
 * @param type G.711编码类型
 * @param pInputBuf 输入的PCM数据
 * @param pOutputBuf 输出的G.711数据
 */
int codec_g711_encode_one(CODEC_G711_TYPE_E type, int16_t pInputBuf, uint8_t *pOutputBuf);

/**
 * @brief G.711A解码
 * @param pInputBuf 输入的G.711A数据
 * @param pOutputBuf 输出的PCM数据
 * @param unLength 输入的G.711A数据长度
 */
int codec_g711a_decode(uint8_t *pInputBuf, uint8_t *pOutputBuf, uint32_t unLength);

/**
 * @brief G.711U解码
 * @param pInputBuf 输入的G.711U数据
 * @param pOutputBuf 输出的PCM数据
 * @param unLength 输入的G.711U数据长度
 */
int codec_g711u_decode(uint8_t *pInputBuf, uint8_t *pOutputBuf, uint32_t unLength);

/**
 * @brief G.711解码
 * @param type G.711编码类型
 * @param pInputBuf 输入的G.711数据
 * @param pOutputBuf 输出的PCM数据
 * @param unLength 输入的G.711数据长度
 */
int codec_g711_decode(CODEC_G711_TYPE_E type, uint8_t *pInputBuf, uint8_t *pOutputBuf, uint32_t unLength);

/**
 * @brief G.711A解码单个采样点
 * @param pInputBuf 输入的G.711A数据
 * @param pOutputBuf 输出的PCM数据
 */
int codec_g711a_decode_one(uint8_t pInputBuf, uint8_t *pOutputBuf);

/**
 * @brief G.711U解码单个采样点
 * @param pInputBuf 输入的G.711U数据
 * @param pOutputBuf 输出的PCM数据
 */
int codec_g711u_decode_one(uint8_t pInputBuf, uint8_t *pOutputBuf);

/**
 * @brief G.711解码单个采样点
 * @param type G.711编码类型
 * @param pInputBuf 输入的G.711数据
 * @param pOutputBuf 输出的PCM数据
 */
int codec_g711_decode_one(CODEC_G711_TYPE_E type, uint8_t pInputBuf, uint8_t *pOutputBuf);

#ifdef __cplusplus
}
#endif
#endif /* _CODEC_G711_H_ */