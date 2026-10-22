/*
 * @FilePath: code_actiontonum_returntomessege.h
 * @Author: yangwenyao
 * @Date: 2022-12-01 19:01:05
 * @LastEditors:
 * @LastEditTime: 2022-12-01 19:01:06
 * @Descripttion:
 */
#ifndef _CODE_ACTIONTONUM_RETURNTOMESSEGE_
#define _CODE_ACTIONTONUM_RETURNTOMESSEGE_

#include <stdio.h>
#include <string.h>
#include "public_define.h"
// #include "edukit_value.h"

typedef enum
{
    OPERATE_RETURN_STATUS = 200, // 成功
} RETURN_STATUS_E;

typedef struct _ENReturnAndMessege
{
    int nReturnCode;
    char *pReturnMessege;
} CodeReturnMessege_S;

typedef struct _ENActionAndCmdType
{
    int nCmdtype;
    char *pActionCode;
} CodeActionCmdType_S;

/*@ref 通过返回码，获取错误描述
 *@param[in] nReturnCode 返回码
 *@param[in] pMessege 错误描述存放指针(至少30个字节)
 *@return 成功返回RET_SUCCESS,失败见RetErr_t
 */
RetErr_t code_ToMessege_returncode(int nReturnCode, char *pMessege);

/*@ref 通过明文命令码，获取命令类型
 *@param[in] pActionCode 命令码存放指针
 *@return 命令类型码
 */
int code_ToCmdType_actioncode(char *pActionCode);

#endif