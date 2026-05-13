/* 
 * @FilePath     : user_manage.c
 * @Author       : zhouzirui
 * @Date         : 2024-08-26 11:10:51
 * @LastEditors  : zhouzirui
 * @LastEditTime : 2024-10-12 09:28:02
 * @Description  : 系统用户管理
 */
#include "user_manage.h"

/* 管理员配置 */
static UserManage_t gs_stAdminConfiguration;
/* 普通用户配置 */
static UserManage_t gs_stGuestConfiguration;


// 函数用于检测 UTF-8 编码的字符是否为语言符号
int isLanguageSymbol(const unsigned char *ch)
{
    // 如果第一个字节的最高位为0，它是ASCII字符
    if ((ch[0] & 0x80) == 0) {
        // ASCII 范围内的字符不是语言符号
        return FALSE;
    }
    // 如果第一个字节以110开头，它是两个字节的字符
    else if ((ch[0] & 0xE0) == 0xC0) {
        // 可以是拉丁文、希腊文、西里尔文等符号
        return TRUE;
    }
    // 如果第一个字节以1110开头，它是三个字节的字符
    else if ((ch[0] & 0xF0) == 0xE0) {
        // 可以是中文、日文、韩文等符号
        return TRUE;
    }
    // 如果第一个字节以11110开头，它是四个字节的字符
    else if ((ch[0] & 0xF8) == 0xF0) {
        // 通常是非常用的语言符号或特殊字符
        return FALSE;
    }
    // 其他情况可能是非法的 UTF-8 编码
    else {
        return FALSE;
    }
    
    // // 这里只是一个示例，实际上你需要根据具体的 Unicode 范围来判断
    // // 是否为特定语言的符号。以下是一个简单的判断逻辑：
    // if ((ch[0] & 0xF0) == 0xE0) {
    //     // 检查是否在某个特定语言符号的范围内
    //     // 例如，希腊字母的范围是 U+0370 到 U+03FF
    //     unsigned int codepoint = ((ch[0] & 0x0F) << 12) | ((ch[1] & 0x3F) << 6) | (ch[2] & 0x3F);
    //     if (codepoint >= 0x0370 && codepoint <= 0x03FF) {
    //         return TRUE; // 希腊字母符号
    //     }
    // }

    return FALSE; // 默认不是语言符号
}

int userManage_init()
{
    memset(&gs_stAdminConfiguration,0,sizeof(UserManage_t));
    gs_stAdminConfiguration.enRole = ADMIN_USER;
    memcpy(gs_stAdminConfiguration.strUserName,USER_ADMIN,sizeof(USER_ADMIN));
    memcpy(gs_stAdminConfiguration.strPassword,USER_ADMIN_PASSWD,sizeof(USER_ADMIN_PASSWD));
    gs_stAdminConfiguration.uLowLevelPasswordAllowed = FALSE;
    gs_stAdminConfiguration.enPasswordLevel = PASSWORD_STRENGTH_HIGH;
    gs_stAdminConfiguration.uFirstLogin = TRUE;

    memset(&gs_stGuestConfiguration,0,sizeof(UserManage_t));
    gs_stGuestConfiguration.enRole = GUEST_USER;
    memcpy(gs_stGuestConfiguration.strUserName,USER_GUEST,sizeof(USER_GUEST));
    memcpy(gs_stGuestConfiguration.strPassword,USER_GUEST_PASSWD,sizeof(USER_GUEST_PASSWD));
    gs_stGuestConfiguration.uLowLevelPasswordAllowed = FALSE;
    gs_stGuestConfiguration.enPasswordLevel = PASSWORD_STRENGTH_MEDIUM;
    gs_stGuestConfiguration.uFirstLogin = TRUE;

    return 0;
}

int userManage_deinit()
{
    memset(&gs_stAdminConfiguration,0,sizeof(UserManage_t));
    memset(&gs_stGuestConfiguration,0,sizeof(UserManage_t));
    return 0;
}

int userManage_set_config(UserManage_t stUserConfiguration)
{
    if (stUserConfiguration.strPassword == NULL || stUserConfiguration.strUserName == NULL)
    {
        dlog(LOG_ERROR, "userManage_set fail");
        return -1;
    }
    if (stUserConfiguration.enRole == ADMIN_USER)
    {
        memcpy(&gs_stAdminConfiguration, &stUserConfiguration, sizeof(UserManage_t));
    }
    else if (stUserConfiguration.enRole == GUEST_USER)
    {
        memcpy(&gs_stGuestConfiguration, &stUserConfiguration, sizeof(UserManage_t));
    }
    return 0;
}

int userManage_get_config(UserManage_t *pUserConfiguration, USER_NAME enRole)
{
    if (pUserConfiguration == NULL || pUserConfiguration->strPassword == NULL || pUserConfiguration->strUserName == NULL)
    {
        dlog(LOG_ERROR, "userManage_get fail");
        return -1;
    }
    if (enRole == ADMIN_USER)
    {
        memcpy(pUserConfiguration, &gs_stAdminConfiguration, sizeof(UserManage_t));
    }
    else if (enRole == GUEST_USER)
    {
        memcpy(pUserConfiguration, &gs_stGuestConfiguration, sizeof(UserManage_t));
    }
    return 0;
}

/* 判断密码是否符合规定的强度要求 */
int userManage_judge_passwordStrength(const char *pPassword, Password_Strength_E *pLevel)
{
    int length = strlen(pPassword);
    unsigned uHasLower; /*小写*/
    unsigned uHasUpper; /*大写*/
    unsigned uHasDigit; /*数字*/
    unsigned uHasSymbol;    /*符号*/
    unsigned uHasLangSymbol;    /* 语言符号 */
    uHasLower = uHasUpper = uHasDigit = uHasSymbol = uHasLangSymbol = FALSE;

    for (int i = 0; i < length; i++)
    {
        if (islower(pPassword[i]))  /*小写*/
        {
            uHasLower = TRUE;
        }
        else if (isupper(pPassword[i])) /*大写*/
        {
            uHasUpper = TRUE;
        }
        else if (isdigit(pPassword[i])) /*数字*/
        {
            uHasDigit = TRUE;
        }
        else if (ispunct(pPassword[i])) /*符号*/
        {
            uHasSymbol = TRUE;
        }
        else if(pPassword[i] == 0xEF && pPassword[i+1] == 0xBF && pPassword[i+2] == 0xA5)   /*判断人民币符号*/
        {
            uHasSymbol = TRUE;
            i += 2;
        }
        else if(isLanguageSymbol((unsigned char *)pPassword[i]))
        {
            uHasLangSymbol = TRUE; /* 语言符号 */
        }
        // else
        // {
        //     uHasLangSymbol = TRUE; /* 简单判断非ASCII字符为语言符号 */
        // }
    }

    /* 根据密码内容设置实际强度等级 */
    if (length >= 16 && uHasLower && uHasUpper && uHasDigit && (uHasSymbol || uHasLangSymbol))
    {
        *pLevel = PASSWORD_STRENGTH_EXTREME;
    }
    else if (length >= 8 && (uHasLower + uHasUpper + uHasDigit + uHasSymbol) >= 3)
    {
        *pLevel = PASSWORD_STRENGTH_HIGH;
    }
    else if (length >= 8 && ((uHasLower || uHasUpper) + uHasDigit) >= 2)
    {
        *pLevel = PASSWORD_STRENGTH_MEDIUM;
    }
    else if (length >= 6 && (uHasDigit))
    {
        *pLevel = PASSWORD_STRENGTH_LOW;
    }
    else
    {
        return -1; /* 密码不符合任何强度等级要求 */
    }
    return 0; /* 密码符合相应强度等级 */
}

int userManage_change_password(USER_NAME enRole, const char *pPassword, int nPasswordLen)
{
    if(pPassword == NULL)
    {
        dlog(LOG_ERROR,"userManage_change_password fail");
        return -1;
    }
    int nRet = -1;
    Password_Strength_E enLevel;
    nRet = userManage_judge_passwordStrength(pPassword, &enLevel);
    if(nRet == -1)
    {
        dlog(LOG_ERROR,"The password does not meet any of the strength levels");
        return -1;
    }
    if(enRole == ADMIN_USER)
    {
        if(enLevel < gs_stAdminConfiguration.enPasswordLevel)
        {
            /*密码强度过低*/
            dlog(LOG_INFO,"Password strength is too low");
            return -1;
        }
        memset(gs_stAdminConfiguration.strPassword, 0, sizeof(gs_stAdminConfiguration.strPassword));
        memcpy(gs_stAdminConfiguration.strPassword, pPassword, nPasswordLen);
    }else if(enRole == GUEST_USER)
    {
        if(enLevel > PASSWORD_STRENGTH_LOW)
        {
            if(enLevel < gs_stGuestConfiguration.enPasswordLevel)
            {
                /*密码强度过低*/
                dlog(LOG_INFO,"Password strength is too low");
                return -1;
            }
        }else if(enLevel == PASSWORD_STRENGTH_LOW)
        {
            if(gs_stAdminConfiguration.uLowLevelPasswordAllowed != TRUE)
            {
                /*密码强度过低*/
                dlog(LOG_INFO,"Password strength is too low");
                return -1;
            }
        }
        memset(gs_stGuestConfiguration.strPassword, 0, sizeof(gs_stGuestConfiguration.strPassword));
        memcpy(gs_stGuestConfiguration.strPassword, pPassword, nPasswordLen);
    }
    return 0;
}

int userManage_change_passwordStrength(USER_NAME enRole, Password_Strength_E enStrength)
{
    int nRet = -1;
    if(enRole == ADMIN_USER)
    {
        if(enStrength >= PASSWORD_STRENGTH_HIGH)
        {
            gs_stAdminConfiguration.enPasswordLevel = enStrength;
            nRet = 0;
        }
    }else if(enRole == GUEST_USER)
    {
        if(enStrength >= PASSWORD_STRENGTH_MEDIUM)
        {
            gs_stGuestConfiguration.enPasswordLevel = enStrength;
            nRet = 0;
        }
    }
    return nRet;
}

int userManage_change_normalLowLevelPassword(int nStatus)
{
    gs_stAdminConfiguration.uLowLevelPasswordAllowed = nStatus;
    if(nStatus)
    {
        gs_stGuestConfiguration.enPasswordLevel = PASSWORD_STRENGTH_LOW;
    }else{
        gs_stGuestConfiguration.enPasswordLevel = PASSWORD_STRENGTH_MEDIUM;
    }
    return 0;
}

int userManage_check_guestPasswordStrength()
{
    int nRet = -1;
    Password_Strength_E enLevel;
    nRet = userManage_judge_passwordStrength(gs_stGuestConfiguration.strPassword, &enLevel);
    if(nRet == -1)
    {
        dlog(LOG_ERROR,"The password does not meet any of the strength levels");
        return nRet;
    }
    if(enLevel <= PASSWORD_STRENGTH_LOW)
    {
        nRet = TRUE;
    }
    return nRet;
}