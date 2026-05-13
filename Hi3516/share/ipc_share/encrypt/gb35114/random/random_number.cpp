/**
 * @FilePath     : random_number.cpp
 * @Author       : zhouzirui
 * @Date         : 2025-04-09 15:07:34
 * @LastEditors  : zhouzirui
 * @LastEditTime : 2025-04-09 15:28:01
 * @Description  : 随机数模块
 */

#include "random_number.h"
#include "dlog.h"

int randomNumber_get(uint8_t *buf, size_t buflen)
{
    if(buflen > RAND_BYTES_MAX_SIZE || buflen <= 0 || buf == nullptr)
    {
        dlog_error("参数错误");
        return ERR_PARAM;
    }
    rand_bytes(buf, buflen);

    return OK;
}