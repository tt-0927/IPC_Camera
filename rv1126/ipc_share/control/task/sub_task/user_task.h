/***
 * @FilePath     : user_task.h
 * @Author       : zhangjc (zhangjc@kfb.cn)
 * @Date         : 2024-10-06 15:16:08
 * @LastEditors  : huangjunda
 * @LastEditTime : 2025-03-28 15:17:25
 * @Description  :
 */

#pragma once
#include "task_sub_class.h"

namespace Task
{
    namespace User
    {
        TaskSubClass(Login);
        TaskSubClass(Add);
        TaskSubClass(Del);
        TaskSubClass(Update);
        TaskSubClass(UpdateStatus);
        TaskSubClass(Find);
        TaskSubClass(GetLoginErrorInfo);
        TaskSubClass(GetAllUser);
        TaskSubClass(VerificationAdmin);
        TaskSubClass(GetOnlineUser);
        TaskSubClass(DeleteOnlinUser);
        TaskSubClass(UserPermissionAuth);
        TaskSubClass(DeleUserExit);
        TaskSubClass(UpdateUserExit);
        TaskSubClass(UpdateLocalOnlineUser);
        TaskSubClass(ResetPassword);
    } // namespace User
} // namespace Task
