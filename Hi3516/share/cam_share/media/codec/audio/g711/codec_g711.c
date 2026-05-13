/**
 * @FilePath     : codec_g711.c
 * @Author       : zhouzirui
 * @Date         : 2025-05-14 11:19:31
 * @LastEditors  : zhouzirui
 * @LastEditTime : 2025-05-15 17:15:58
 * @Description  : g711编解码
 */

#include "codec_g711.h"

//* --------- G.711 编码 --------- */

/**
 * @brief G.711A编码
 * @param pInputBuf 输入的PCM数据
 * @param pOutputBuf 输出的G.711A数据
 * @param unLength 输入的PCM数据长度
 */
int codec_g711a_encode(uint8_t *pInputBuf, uint8_t *pOutputBuf, uint32_t unLength)
{
    return codec_g711_encode(CODEC_G711_TYPE_ALAW, pInputBuf, pOutputBuf, unLength);
}

/**
 * @brief G.711U编码
 * @param pInputBuf 输入的PCM数据
 * @param pOutputBuf 输出的G.711U数据
 * @param unLength 输入的PCM数据长度
 */
int codec_g711u_encode(uint8_t *pInputBuf, uint8_t *pOutputBuf, uint32_t unLength)
{
    return codec_g711_encode(CODEC_G711_TYPE_ULAW, pInputBuf, pOutputBuf, unLength);
}

/**
 * @brief G.711编码
 * @param type G.711编码类型
 * @param pInputBuf 输入的PCM数据
 * @param pOutputBuf 输出的G.711数据
 * @param unLength 输入的PCM数据长度
 */
int codec_g711_encode(CODEC_G711_TYPE_E type, uint8_t *pInputBuf, uint8_t *pOutputBuf, uint32_t unLength)
{
    int i = 0;
    int ret = 0;

    CODEC_RETVAL_NOMSG(CODEC_G711_TYPE_ALAW != type && CODEC_G711_TYPE_ULAW != type, CODEC_EINVAL);
    CODEC_RETVAL_NOMSG(NULL == pInputBuf, CODEC_EINVAL);
    CODEC_RETVAL_NOMSG(NULL == pOutputBuf, CODEC_EINVAL);
    CODEC_RETVAL_NOMSG(0 == unLength, CODEC_EINVAL);

    unLength = unLength / 2;

    for(i = 0; i < unLength; i++)
    {
#if (CODEC_G711_ENDIAN == CODEC_G711_ENDIAN_LITTLE)
        ret = codec_g711_encode_one(type, pInputBuf[2 * i + 1] << 8 | pInputBuf[2 * i], pOutputBuf + i);
#elif (CODEC_G711_ENDIAN == CODEC_G711_ENDIAN_BIG)
        ret = codec_g711_encode_one(type, pInputBuf[2 * i] << 8 | pInputBuf[2 * i + 1], pOutputBuf + i);
#endif
        CODEC_RETVAL_NOMSG(ret, ret);
    }

    return CODEC_OK;
}

/**
 * @brief G.711A编码单个采样点
 * @param pInputBuf 输入的PCM数据
 * @param pOutputBuf 输出的G.711A数据
 */
int codec_g711a_encode_one(int16_t pInputBuf, uint8_t *pOutputBuf)
{
    int16_t pcm13bit = 0;               // PCM 有效值
    int16_t pcm_no_s = 0;               // PCM 绝对值
    uint8_t s = 0;                      // G.711 符号位
    uint8_t eee = 0;                    // G.711 强度位
    uint8_t abcd = 0;                   // G.711 样本位

    CODEC_RETVAL_NOMSG(NULL == pOutputBuf, CODEC_EINVAL);

    /* 获取有效值 */
    pcm13bit = pInputBuf >> 3;

    /* 计算符号位 */
    s = 1 - ((pcm13bit & 0x1000) >> 12);
    pcm_no_s = (s == 0) ? ~pcm13bit : pcm13bit & 0xfff;

    /* 计算强度位 */
    if (pcm_no_s >= 0x800)
        eee = 7;
    else if (pcm_no_s >= 0x400)
        eee = 6;
    else if (pcm_no_s >= 0x200)
        eee = 5;
    else if (pcm_no_s >= 0x100)
        eee = 4;
    else if (pcm_no_s >= 0x080)
        eee = 3;
    else if (pcm_no_s >= 0x040)
        eee = 2;
    else if (pcm_no_s >= 0x020)
        eee = 1;
    else
        eee = 0;

    /* 计算样本位 */
    abcd = (pcm_no_s >> (eee ? eee : 1)) & 0xf;

    /* 组合为ALAW码字 */
    *pOutputBuf = ((s << 7) | (eee << 4) | abcd) ^ 0x55;

    return CODEC_OK;
}

/**
 * @brief G.711U编码单个采样点
 * @param pInputBuf 输入的PCM数据
 * @param pOutputBuf 输出的G.711U数据
 */
int codec_g711u_encode_one(int16_t pInputBuf, uint8_t *pOutputBuf)
{
    int16_t pcm14bit = 0;               // PCM 有效值
    int16_t pcm_no_s = 0;               // PCM 绝对值
    uint8_t s = 0;                      // G.711 符号位
    uint8_t eee = 0;                    // G.711 强度位
    uint8_t abcd = 0;                   // G.711 样本位

    CODEC_RETVAL_NOMSG(NULL == pOutputBuf, CODEC_EINVAL);

    /* 获取有效值 */
    pcm14bit = pInputBuf >> 2;

    /* 计算符号位 */
    s = (pcm14bit & 0x2000) >> 13;
    pcm_no_s = (s == 1) ? ~pcm14bit : pcm14bit & 0x1fff;

    /* 计算强度位 */
    if (pcm_no_s >= 0x1000)
        eee = 7;
    else if (pcm_no_s >= 0x800)
        eee = 6;
    else if (pcm_no_s >= 0x400)
        eee = 5;
    else if (pcm_no_s >= 0x200)
        eee = 4;
    else if (pcm_no_s >= 0x100)
        eee = 3;
    else if (pcm_no_s >= 0x080)
        eee = 2;
    else if (pcm_no_s >= 0x040)
        eee = 1;
    else if (pcm_no_s >= 0x020)
        eee = 0;
    else
        eee = 0;                // TODO

    /* 计算样本位 */
    abcd = (pcm_no_s >> (eee + 1)) & 0xf;

    /* 组合为ULAW码字 */
    *pOutputBuf = ((s << 7) | (eee << 4) | abcd) ^ 0xff;

    return CODEC_OK;
}

/**
 * @brief G.711编码单个采样点
 * @param type G.711编码类型
 * @param pInputBuf 输入的PCM数据
 * @param pOutputBuf 输出的G.711数据
 */
int codec_g711_encode_one(CODEC_G711_TYPE_E type, int16_t pInputBuf, uint8_t *pOutputBuf)
{
    CODEC_RETVAL_NOMSG(NULL == pOutputBuf, CODEC_EINVAL);

    if(CODEC_G711_TYPE_ALAW == type)
    {
        return codec_g711a_encode_one(pInputBuf, pOutputBuf);
    }
    else if(CODEC_G711_TYPE_ULAW == type)
    {
        return codec_g711u_encode_one(pInputBuf, pOutputBuf);
    }

    return CODEC_EINVAL;
}

//* --------- G.711 解码 --------- */

/**
 * @brief G.711A解码
 * @param pInputBuf 输入的G.711A数据
 * @param pOutputBuf 输出的PCM数据
 * @param unLength 输入的G.711A数据长度
 */
int codec_g711a_decode(uint8_t *pInputBuf, uint8_t *pOutputBuf, uint32_t unLength)
{
    return codec_g711_decode(CODEC_G711_TYPE_ALAW, pInputBuf, pOutputBuf, unLength);
}

/**
 * @brief G.711U解码
 * @param pInputBuf 输入的G.711U数据
 * @param pOutputBuf 输出的PCM数据
 * @param unLength 输入的G.711U数据长度
 */
int codec_g711u_decode(uint8_t *pInputBuf, uint8_t *pOutputBuf, uint32_t unLength)
{
    return codec_g711_decode(CODEC_G711_TYPE_ULAW, pInputBuf, pOutputBuf, unLength);
}

/**
 * @brief G.711解码
 * @param type G.711编码类型
 * @param pInputBuf 输入的G.711数据
 * @param pOutputBuf 输出的PCM数据
 * @param unLength 输入的G.711数据长度
 */
int codec_g711_decode(CODEC_G711_TYPE_E type, uint8_t *pInputBuf, uint8_t *pOutputBuf, uint32_t unLength)
{
    int i = 0;
    int ret = 0;

    CODEC_RETVAL_NOMSG(CODEC_G711_TYPE_ALAW != type && CODEC_G711_TYPE_ULAW != type, CODEC_EINVAL);
    CODEC_RETVAL_NOMSG(NULL == pInputBuf, CODEC_EINVAL);
    CODEC_RETVAL_NOMSG(NULL == pOutputBuf, CODEC_EINVAL);
    CODEC_RETVAL_NOMSG(0 == unLength, CODEC_EINVAL);

    for(i = 0; i < unLength; i++)
    {
        ret = codec_g711_decode_one(type, pInputBuf[i], pOutputBuf + i * 2);
        CODEC_RETVAL_NOMSG(ret, ret);
    }

    return CODEC_OK;
}

/**
 * @brief G.711A解码单个采样点
 * @param pInputBuf 输入的G.711A数据
 * @param pOutputBuf 输出的PCM数据
 */
int codec_g711a_decode_one(uint8_t pInputBuf, uint8_t *pOutputBuf)
{
    uint8_t s = 0;                      // G.711 符号位
    uint8_t eee = 0;                    // G.711 强度位
    uint8_t abcd = 0;                   // G.711 样本位
    int16_t pcm_no_s = 0;               // PCM 绝对值
    int16_t pcm13bit = 0;               // PCM 有效值
    int16_t pcm16bit = 0;               // PCM 值

    pInputBuf = pInputBuf ^ 0x55;

    s = 1 - ((pInputBuf >> 7) & 0x1);
    eee = (pInputBuf >> 4) & 0x7;
    abcd = pInputBuf & 0xf;

    if(0 == eee)
    {
        pcm_no_s = abcd << 1 | 0x1;
    }
    else
    {
        pcm_no_s = 1 << (eee + 4) | 1 << (eee - 1) | abcd << eee;
    }

    pcm13bit = (s == 0) ? pcm_no_s : ~pcm_no_s;
    pcm16bit = pcm13bit << 3;

#if (CODEC_G711_ENDIAN == CODEC_G711_ENDIAN_LITTLE)
    *pOutputBuf = pcm16bit & 0xff;
    *(pOutputBuf + 1) = (pcm16bit >> 8) & 0xff;
#elif (CODEC_G711_ENDIAN == CODEC_G711_ENDIAN_BIG)
    *(pOutputBuf + 1) = pcm16bit & 0xff;
    *pOutputBuf = (pcm16bit >> 8) & 0xff;
#endif

    return 0;
}

/**
 * @brief G.711U解码单个采样点
 * @param pInputBuf 输入的G.711U数据
 * @param pOutputBuf 输出的PCM数据
 */
int codec_g711u_decode_one(uint8_t pInputBuf, uint8_t *pOutputBuf)
{
    uint8_t s = 0;                      // G.711 符号位
    uint8_t eee = 0;                    // G.711 强度位
    uint8_t abcd = 0;                   // G.711 样本位
    int16_t pcm_no_s = 0;               // PCM 绝对值
    int16_t pcm14bit = 0;               // PCM 有效值
    int16_t pcm16bit = 0;               // PCM 值

    pInputBuf = pInputBuf ^ 0xff;

    s = (pInputBuf >> 7) & 0x1;
    eee = (pInputBuf >> 4) & 0x7;
    abcd = pInputBuf & 0xf;

    pcm_no_s = 1 << (eee + 5) | 1 << eee | abcd << (eee + 1);

    pcm14bit = (s == 0) ? pcm_no_s : ~pcm_no_s;
    pcm16bit = pcm14bit << 2;

#if (CODEC_G711_ENDIAN == CODEC_G711_ENDIAN_LITTLE)
    *pOutputBuf = pcm16bit & 0xff;
    *(pOutputBuf + 1) = (pcm16bit >> 8) & 0xff;
#elif (CODEC_G711_ENDIAN == CODEC_G711_ENDIAN_BIG)
    *(pOutputBuf + 1) = pcm16bit & 0xff;
    *pOutputBuf = (pcm16bit >> 8) & 0xff;
#endif

    return 0;
}

/**
 * @brief G.711解码单个采样点
 * @param type G.711编码类型
 * @param pInputBuf 输入的G.711数据
 * @param pOutputBuf 输出的PCM数据
 */
int codec_g711_decode_one(CODEC_G711_TYPE_E type, uint8_t pInputBuf, uint8_t *pOutputBuf)
{
    CODEC_RETVAL_NOMSG(NULL == pOutputBuf, CODEC_EINVAL);

    if(CODEC_G711_TYPE_ALAW == type)
    {
        return codec_g711a_decode_one(pInputBuf, pOutputBuf);
    }
    else if(CODEC_G711_TYPE_ULAW == type)
    {
        return codec_g711u_decode_one(pInputBuf, pOutputBuf);
    }

    return CODEC_EINVAL;
}


#ifdef __cplusplus
}
#endif

