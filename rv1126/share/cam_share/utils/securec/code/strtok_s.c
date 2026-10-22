
#include "securec.h"

static char *SecFindBegin(char *strToken, const char *strDelimit)
{
    /* Find beginning of token (skip over leading delimiters). Note that
     * there is no token if this loop sets string to point to the terminal null.
     */
    char *token = strToken;
    while (*token != 0) {
        const char *ctl = strDelimit;
        /*遍历所有分隔符直到读到终止符，或者当前字符是分隔符则跳出循环*/
        while (*ctl != 0 && *ctl != *token) {
            ++ctl;
        }
         //ctl遍历完指向终止符了，说明不是分隔符，跳出循环
        if (*ctl == 0) {        /* don't find any delimiter in string header, break the loop */
            break;
        }
        //是分隔符，则继续匹配下一个字符串
        ++token;
    }
    //指向第一个非分隔符或字符串末尾
    return token;
}

static char *SecFindRest(char *strToken, const char *strDelimit)
{

    /* Find the rest of the token. If it is not the end of the string,
     * put a null there.
     */
    char *token = strToken;
    while (*token != 0) {
        const char *ctl = strDelimit;
        while (*ctl != 0 && *ctl != *token) {
            ++ctl;
        }
        if (*ctl != 0) {        /* find a delimiter */
            *token++ = 0;       /* set string termintor */
            break;
        }
        ++token;
    }
    return token;
}

/*******************************************************************************
 * <功能描述>
 *  函数 strtok_s 将字符串解析为令牌序列
 *  首次调用时，strToken 参数应指定待解析的字符串
 *  后续对同一字符串的解析调用中，strToken 应设为 NULL
 *
 * <输入参数>
 * strToken       包含令牌的字符串（首次调用传入）
 * strDelimit    分隔符字符集合
 * context       用于存储调用间位置信息的上下文指针
 *
 * <输出参数>
 *  context       更新后的解析状态
 *
 *  <返回值>
 *  成功时返回指向下一个令牌的指针
 *  当找不到更多令牌时返回 NULL
 *  每次调用会修改原字符串，将令牌后的第一个分隔符替换为 NULL 字符
 *
 *  返回 NULL 的条件：
 *  - context 为 NULL
 *  - strDelimit 为 NULL
 *  - strToken 为 NULL 且 (*context) 为 NULL
 *  - 未找到令牌
 *******************************************************************************
 */
char *strtok_s(char *strToken, const char *strDelimit, char **context)
{
    char *orgToken = strToken;
    /* validate delimiter and string context */
    if (context == NULL || strDelimit == NULL) {
        return NULL;
    }

    /* 当前没有传入新的字符串，且上下文指针没有保存之前的解析进度 */
    if (orgToken == NULL && (*context) == NULL) {
        return NULL;
    }

    /* 没有传入新的字符串，则用上一次保存的上下文指针继续解析 */
    if (orgToken == NULL) {
        orgToken = *context;
    }
    /*找到非分隔符的第一个字符*/
    orgToken = SecFindBegin(orgToken, strDelimit);

    {
        char *token = orgToken; /* point to updated position */
        /*定位字符串中下一个分隔符，将其替换为终止符 \0，并返回后续字符串的起始指针*/
        orgToken = SecFindRest(orgToken, strDelimit);

        /* record string position for next search in the context */
        *context = orgToken;

        /* Determine if a token has been found. */
        if (token == orgToken) {
            token = NULL;
        }
        return token;
    }
}


