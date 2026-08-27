/*
 * @Author: leiyy leiyy@kfb.cn
 * @Date: 2025-12-04 20:35:41
 * @LastEditors: leiyy leiyy@kfb.cn
 * @LastEditTime: 2025-12-22 19:58:39
 * @FilePath: /rv1126_8814T/share/ipc_share/control/business/system/user/user_module.h
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
/* user_module.h */
#ifndef USER_MODULE_H
#define USER_MODULE_H

#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include "IpcRet.h"

/* 密码强度 */
typedef enum PasswordStrength {
    PASSWORD_STRENGTH_WEAK = 0,
    PASSWORD_STRENGTH_MEDIUM,
    PASSWORD_STRENGTH_STRONG,
    PASSWORD_STRENGTH_VERY_STRONG
} PasswordStrength;

typedef struct {
    int conditionCode;
    char *conditionDescription;
} WeakPasswordResult;

/* token 相关：默认密码常量 */
extern const char *defaultPassword;

/**
* @description 根据错误码返回对应的密码合规检测提示文本
* @param  [int] code  : 错误码（0 表示成功，负值表示各类违规）
* @return [const char*]        : 静态字符串指针，描述性提示文本
*/
const char *pass_check_msg(int code);

/**
*@description 判断password密码强度
* 规则：
* 1. 低：长度 >= 6 且全部为数字
* 2. 中：长度 >= 8，字符类型（数字/大写/小写/符号）至少 4 选 2
* 3. 高：长度 >= 8，字符类型至少 4 选 3
* 4. 极：长度 >= 16，4 种字符类型必须全部包含
*
* 注意：如果既满足多个等级，返回“最高”等级。
* 对于不满足任何规则的密码，统一返回 PASSWORD_STRENGTH_WEAK

*@param  [const char *]     password         ： 检测的密码
*@return [PasswordStrength] PasswordStrength ： 密码强度枚举
**/
PasswordStrength evaluatePasswordStrength(const char *password);

/*
*@description 判断password是否为弱密码
*@param  [const char *] password  ：检测的密码
*@param  [const char *] username  ：检测的用户账户名
*@param  [const char *] userphone  ：检测的用户电话
*@param  [const char *] useremail  ：检测的用户邮箱
*@return [int]         ：
        IpcRet_E::OK   :成功，不为弱密码
        IpcRet_E::ERR_PARAM          = -2,  参数错误 
        IpcRet_E::PASS_ERR_REPEAT_CHAR          =  - 49,   密码出现重复字符（相邻 3 位及以上相同） 
        IpcRet_E::PASS_ERR_SEQ_CHAR             =  - 50,   密码出现连续字母（相邻 3 位及以上递增/递减） 
        IpcRet_E::PASS_ERR_REPEAT_BLK           =  - 51,   密码出现重复序列 
        IpcRet_E::PASS_ERR_KEYBOARD             =  - 52,   密码出现键盘键位（QWERTY 连续 3 位及以上） 
        IpcRet_E::PASS_ERR_USER_INFO            =  - 53,   密码出现用户信息强关联 
*/
int isWeakPassword(const char* password, const char *username, const char *phone, const char *email);

/*
*@description 判断password是否为弱密码
*@param  [const char *] password  ：检测的密码
*@param  [const char *] username  ：检测的用户账户名
*@param  [const char *] userphone  ：检测的用户电话
*@param  [const char *] useremail  ：检测的用户邮箱
*@return [WeakPasswordResult] WeakPasswordResult        ：符合弱密码条件返回对应的弱类型ID和对应出现弱条件的字符串

返回值 WeakPasswordResult.conditionCode为一下规则：
1：不允许出现重叠字符：相邻3位及以上字符相同，如AAA、aaaaa、111、2222、****等

2：不允许出现连续数字：相邻3位及以上连续递增/递减数字，如123、321、12345、54321等

3：不允许出现连续字母：相邻3位及以上连续递增/递减字母（不分大小写），如abc、ABcde、dcba等

4：序列长度2位及以上，连续周期性出现2次及以上，如ababab、1212、568568等

5：不得存在基于 QWERTY 键盘布局的相邻3位及以上连续字母，不区分大小写，包含颠倒拼写，如 qwer、rewq、Erty、asdf、rewQ等）

6：与用户名关联：密码中包含 “用户名”中任意连续3位及以上序列

7：与用户手机号关联：密码中包含手机号 任意连续3位及以上序列

8：与用户邮箱关联：密码中包含邮箱号 任意连续3位及以上序列

WeakPasswordResult.conditionDescription 为返回符合弱密码条件第一次出现的字符串，若都不符合则返回空字符串
*/
WeakPasswordResult isWeakPasswordToStr(const char* password, const char* username, const char* userphone, const char* useremail);


/*
*@description 判断password是否有相邻 3 位或以上相同字符
*@param  [const char *] password  ：检测的密码
*@return [bool] true\false        ：password有相邻 3 位或以上相同字符返回true，password没有相邻 3 位或以上相同字符返回true 
*/
bool auxHasOverlappingChars(const char *password);

/*
*@description 判断password是否有相邻 3 位或以上相同字符
*@param  [const char *] password  ：检测的密码
*@return [char *] str        ：password有相邻 3 位或以上相同字符返回对应第一次出现的str，password没有相邻 3 位或以上相同字符返回null
*/
char *auxHasOverlappingCharsToStr(const char *password);

/*
*@description 判断password是否有相邻 3 位或以上连续递增/递减（包括数字、字母）
*@param  [const char *] password  ：检测的密码
*@return [bool] true\false        ：password有相邻 3 位或以上连续递增/递减（包括数字、字母）返回true，password没有相邻 3 位或以上连续递增/递减（包括数字、字母）返回false
*/
bool auxHasConsecutiveChars(const char *password);

/*
*@description 判断password是否有相邻 3 位或以上连续递增/递减字母序列
*@param  [const char *] password  ：检测的密码
*@return [char *] str        ：password有相邻 3 位或以上连续递增/递减字母序列对应第一次出现的str，password没有相邻 3 位或以上连续递增/递减字母序列返回null
*/
char *auxHasConsecutiveLetterToStr(const char *str);

/*
*@description 判断password是否有相邻 3 位或以上连续递增/递减数字序列
*@param  [const char *] password  ：检测的密码
*@return [char *] str       ：password有相邻 3 位或以上连续递增/递减数字序列返回true，password没有相邻 3 位或以上连续递增/递减数字序列返回null
*/
char *auxHasConsecutiveNumToStr(const char *str);

/*
*@description 判断password是否有重复序列
*@param  [const char *] password  ：检测的密码
*@return [char *] true\false        ：password有重复序列返回true，password没有重复序列返回false
*/
bool auxHasPeriodicSequence(const char *password);

/*
*@description 判断password是否有重复序列
*@param  [const char *] password  ：检测的密码
*@return [char *] str        ：password有重复序列返回对应第一次出现的str，password没有重复序列返回null
*/
char *auxHasPeriodicSequenceToStr(const char *str);

/*
*@description 检查password是否有基于 QWERTY 键盘布局的连续 3 位及以上字母
*@param  [const char *] password  ：检测的密码
*@return [bool] true\false        ：password有基于 QWERTY 键盘布局的连续 3 位及以上字母返回true，password没有基于 QWERTY 键盘布局的连续 3 位及以上字母返回false
*/
bool auxHasQwertySequence(const char *password);

/*
*@description 检查password是否有基于 QWERTY 键盘布局的连续 3 位及以上字母
*@param  [const char *] password  ：检测的密码
*@return [char *] str        ：password有基于 QWERTY 键盘布局的连续 3 位及以上字母返回对应第一次出现的str，password没有重复序列返回null
*/
char *auxHasQwertySequenceToStr(const char *str);

/*
*@description 检查password是否与个人信息强关联
*@param  [const char *] password  ：检测的密码
*@param  [const char *] username  ：用户名
*@param  [const char *] phone     ：用户手机号码
*@param  [const char *] email     ：用户邮箱
*@return [bool] true\false        ：password有与个人信息强关联返回true，password没有与个人信息强关联返回false
*/
bool auxIsAssociatedWithPersonalInfo(const char *password, const char *username, const char *phone, const char *email);

/*
*@description 检查第一个字符串是否与第二个字符串强关联
*@param  [const char *] password  ：第一个字符串
*@param  [const char *] username  ：第二个字符串
*@return [char *] str        ：第一个字符串是与第二个字符串强关联返回对应第一次出现的str，password没有强关联返回null
*/
char *auxIsAssociatedWithPersonalInfoToStr(const char *password, const char *name);

/*
*@description 检查password包含高频常用名词 -- 高频常用名词为测试部维护词库
*@param  [const char *] password  ：检测的密码
*@return [char *] word            ：包含的高频常用名词
*/
const char *auxContainsWeakPasswordWordsToStr(const char *password);

/*
*@description 检查password是否符合规范
*@param  [const char *] password  ：检测的密码
*@param  [const char *] username  ：用户名
*@param  [const char *] phone     ：用户手机号码
*@return [int] 
   
        IpcRet_E::OK   :成功，不为弱密码
        IpcRet_E::ERR_PARAM          = -2,  参数错误 
        IpcRet_E::PASS_ERR_REPEAT_CHAR          =  - 49,   密码出现重复字符（相邻 3 位及以上相同） 
        IpcRet_E::PASS_ERR_SEQ_CHAR             =  - 50,   密码出现连续字母（相邻 3 位及以上递增/递减） 
        IpcRet_E::PASS_ERR_REPEAT_BLK           =  - 51,   密码出现重复序列 
        IpcRet_E::PASS_ERR_KEYBOARD             =  - 52,   密码出现键盘键位（QWERTY 连续 3 位及以上） 
        IpcRet_E::PASS_ERR_USER_INFO            =  - 53,   密码出现用户信息强关联 
        IpcRet_E::PASS_ERR_WEAK_WORD            =  - 54,   密码出现弱口令 
        IpcRet_E::PASS_ERR_STRENGTH_LOW         =  - 55,   密码强度较弱 
*/
int check_password(const char *password,const char *username,const char *phone);

#endif