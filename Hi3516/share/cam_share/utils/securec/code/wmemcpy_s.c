/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
/* [Standardize-exceptions] Use unsafe function: Portability
 * [reason] Use unsafe function to implement security function to maintain platform compatibility.
 *          And sufficient input validation is performed before calling
 */

#include "securecutil.h"
#include "memcpy_s.h"

/*******************************************************************************
<功能描述>
*memcpy_s 函数将 src 指向对象的 n 个字符复制到 dest 指向的对象中。memcpy_s的宽字符版，但是返回值有所不一样
*
<输入参数>
dest                             目标缓冲区
destMax                          目标缓冲区大小
src                              源缓冲区
count                            要复制的字符数

<输出参数>
dest                             目标缓冲区

<返回值>
EOK                              成功
EINVAL                           目标缓冲区 为 NULL 且 destMax != 0 且 destMax ≤ SECUREC_MEM_MAX_LEN（当destMax=0的时候，
                                 表示不拷贝到目标缓冲区，所以目标缓冲区可以为null）count <= destMax

EINVAL_AND_RESET                目标缓冲区不为null且缓冲区大小不超出限制，但源缓冲区为null，其他参数都合法
ESRANGE                         目标缓冲区大小超出限制，或为0，或count大于目标缓冲区且目标缓冲区为null，但目标缓冲区不为0且不超出限制
ERANGE_AND_RESET                其他参数都合法，但count大于目标缓冲区
EOVERLAP_AND_RESET              其他参数都合法，但目标缓冲区和源缓冲区有重叠部分

* 如果发生错误，dest 将被填充为 0。
* 若源和目标内存重叠，memcpy_s 的行为是未定义的。
* 处理重叠区域时应使用 memmove_s。
 *******************************************************************************
 */
errno_t wmemcpy_s(wchar_t *dest, size_t destMax, const wchar_t *src, size_t count)
{
    //目标缓冲区不合法
    if (destMax == 0 || destMax > SECUREC_WCHAR_MEM_MAX_LEN) {
        SECUREC_ERROR_INVALID_PARAMTER("wmemcpy_s");
        return ERANGE;
    }
    //复制数大于目标缓冲区大小
    if (count > destMax) {
        SECUREC_ERROR_INVALID_PARAMTER("wmemcpy_s");
        if (dest != NULL) {
        //如果目标缓冲区存在，清0目标缓冲区
            (void)memset(dest, 0, destMax * sizeof(wchar_t));
            return ERANGE_AND_RESET;
        }
        return ERANGE;
    }
    return memcpy_s(dest, destMax * sizeof(wchar_t), src, count * sizeof(wchar_t));
}


