#include "code_actiontonum_returntomessege.h"

#define ACTION_TYPE_SIZE 5

CodeReturnMessege_S g_stReturnCode[] = {
    /* 临时 */
    {100, "temporary_continue_request"}, /* 请继续请求 */
    {101, "temporary_cut_protocol"},     /* 确认切换协议 */
    /* 成功 */
    {200, "succeed"},                     /* 成功 */
    {201, "succeed_creater"},             /* 成功创建服务器资源 */
    {202, "succeed_accept_request"},      /* 服务器已接受请求 */
    {203, "succeed_no_impower_info"},     /* 成功处理，返回非授权信息 */
    {204, "succeed_no_return_info"},      /* 成功，无返回内容 */
    {205, "succeed_reset_info"},          /* 成功，无返回内容 */
    {206, "succeed_portion_info"},        /* 成功，处理部分请求 */
    /* 重定向 */
    {300, "redirect_variety_select"},     /* 可执行多项请求操作 */
    {301, "redirect_perpetual_move"},     /* 永久移动到新位置 */
    {302, "redirect_temporary_move"},     /* 临时移动到新位置 */
    {303, "redirect_look_other"},         /* 查看其他位置 */
    {304, "redirect_web_no_change"},      /* 网页未修改 */
    {305, "redirect_agency_visit"},       /* 使用代理 */
    {307, "redirect_temporary_redirect"}, /* 临时重定向 */
    /* 请求错误 */
    {400, "error_no_understand_request"}, /* 错误请求 */
    {401, "error_no_impower_visit"},      /* 未授权 */
    {403, "error_refuse_request"},        /* 拒绝请求 */
    {404, "error_no_find_web"},           /* 未找到网页 */
    {405, "error_ban_way"},               /* 禁止方法 */
    {406, "error_refuse_request_data"},   /* 不接受请求内容 */
    {407, "error_agency_no_impower"},     /* 需要代理授权 */
    {408, "error_request_overtime"},      /* 请求超时 */
    {409, "error_happen_conlict"},        /* 完成请求时发生冲突 */
    {410, "error_resource_delete"},       /* 请求资源已删除 */
    {411, "error_no_data_length"},        /* 缺失内容长度 */
    {412, "error_no_meet_conditions"},    /* 未满足前提条件 */
    {413, "error_request_oversize"},      /* 请求实体过大 */
    {414, "error_uri_overlength"},        /* 请求uri过长 */
    {415, "error_no_support_type"},       /* 不支持媒体类型 */
    {416, "error_request_out_of_range"},  /* 请求超出范围 */
    {417, "error_no_meet_expect"},        /* 未满足期望值 */
    {418, "File does not exist"},         /* 文件不存在 */
    {419, "Command code error"},          /* 命令码错误 */
    {420, "File modification failed"},    /* 修改文件失败 */
    {421, "File name null"},              /* 文件名为空 */
    /* 服务器错误 */
    {500, "server_happen_error"},         /* 服务器内部错误 */
    {501, "server_not_finish_request"},   /* 不具备完成请求功能 */
    {502, "server_error_gate"},           /* 错误网关 */
    {503, "server_not_use"},              /* 服务器无法使用 */
    {504, "server_gate_overtime"},        /* 网关超时 */
    {505, "server_no_support_request"}    /* 服务器不支持请求 */
};

CodeActionCmdType_S g_stActionType[ACTION_TYPE_SIZE] = {
    {30010, "set_record"},
    {30026, "register_platform"},
    {30032, "client_heart"},
    {30027, "get_rtmp_address"},
    {30040, "if_ai_recorder"}};

RetErr_t code_ToMessege_returncode(int nReturnCode, char *pMessege)
{
    if (nReturnCode == 0)
    {
        return RET_NULLRETURN;
    }
    if (strlen(pMessege) < 30)
    {
        // nslog(NS_ERROR,"return_Messege size :%d\n",strlen(pMessege));
        // return RET_PARAMER_ERR;
    }
    int i = 0;
    for (i = 0; i < 45; i++)
    {
        if (nReturnCode == g_stReturnCode[i].nReturnCode)
            memcpy(pMessege, g_stReturnCode[i].pReturnMessege, strlen(g_stReturnCode[i].pReturnMessege));
    }

    if (i == 45)
        return RET_PARAMER_ERR;

    return RET_SUCCESS;
}

int code_ToCmdType_actioncode(char *pActionCode)
{
    if (pActionCode == NULL)
    {
        return RET_NULLRETURN;
    }
    int i = 0;
    for (i = 0; i < ACTION_TYPE_SIZE; i++)
    {
        if (!strcmp(pActionCode, g_stActionType[i].pActionCode))
            return g_stActionType[i].nCmdtype;
    }
    return RET_PARAMER_ERR;
}
