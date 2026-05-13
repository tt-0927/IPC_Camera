/* user_module.c */
#include "user_module.h"

#include <string.h>
#include <ctype.h>
#include <stdlib.h>

/* token 相关：默认密码 */
const char *defaultPassword = "zfrl@168";

static const char *WeakPasswordWordsSummary[] = {
    "itc", "bl", "baolun", "ids", "aidisi", "panyu", "jinshangu", "jsg", "yiku", "zhongcun",
    "admin", "administrator", "root", "login", "system", "logout", "pass", "password", "P@ssw0rd", "test", "testing",
    "t3st", "73st", "t3$t", "welcome", "letmein", "demo", "user", "guest", "computer", "pc", "mac", "home", "work",
    "example", "sample", "public", "server", "access", "enter","hi", "sun", "killer", "hey", "moon", "awesome",
    "hello", "star", "cool", "h3ll0", "sky", "nice","h3llo", "cloud", "good", "olleh", "rain", "bad",
    "love", "dog", "hot", "like", "cat", "cold","hate", "fish", "secret", "lover", "bird", "private",
    "god", "tiger", "hidden", "angel", "lion", "security","devil", "dragon", "safe", "heaven", "monkey", "number",
    "hell", "apple", "first", "car", "orange", "last","bike", "banana", "one", "boat", "fruit", "alpha",
    "run", "food", "beta", "swim", "pizza", "master","game", "beer", "fuck", "play", "coffee", "fucker",
    "winner", "dearbook", "fucking", "win", "michael", "fuckme","lose", "michelle", "hunter", "fuckyou", "soccer", "superman",
    "football", "charlie", "super", "baseball", "loverpool", "jessica","basketball", "starwars", "unknown", "golf", "jordan", 
};

static const size_t WeakPasswordWordsSummaryCount =
    sizeof(WeakPasswordWordsSummary) / sizeof(WeakPasswordWordsSummary[0]);

static const char *strength_names[] = {
    "弱", "中等", "强", "非常强"
};

const char *pass_check_msg(int code)
{
    switch (code)
    {
    case OK:                            return "密码符合规范";
    case ERR_PARAM:                     return "输入参数为空";
    case PASS_ERR_REPEAT_CHAR:          return "密码存在 3 位及以上重复字符";
    case PASS_ERR_SEQ_CHAR:             return "密码存在 3 位及以上连续字母";
    case PASS_ERR_REPEAT_BLK:           return "密码存在重复序列";
    case PASS_ERR_KEYBOARD:             return "密码存在 3 位及以上键盘连续键位";
    case PASS_ERR_USER_INFO:            return "密码包含用户名";
    case PASS_ERR_WEAK_WORD:            return "密码出现弱口令";
    case PASS_ERR_STRENGTH_LOW:         return "密码强度较弱";
    case ERR_REPEAT_LOGIN_IP:           return "同一IP已登录，禁止重复登录";
    default:                            return "未知错误";
    }
}

/* 判断 haystack 是否包含 needle（不区分大小写） */
static bool containsCaseInsensitive(const char *haystack,
                                    const char *needle) {
    size_t hlen = strlen(haystack);
    size_t nlen = strlen(needle);

    if (nlen == 0 || hlen < nlen) {
        return false;
    }

    if( nlen != hlen ){
        return false;
    }

    for (size_t i = 0; i <= hlen - nlen; ++i) {
        size_t j = 0;
        for (; j < nlen; ++j) {
            unsigned char c1 = (unsigned char)haystack[i + j];
            unsigned char c2 = (unsigned char)needle[j]; /* needle 本身是小写 */

            /* 判断是否为字母字符 */
            if (isalpha(c1) && isalpha(c2)) {
                /* 字母字符进行大小写不敏感比较 */
                if (tolower(c1) != c2) {
                    break;
                }
            } else {
                /* 特殊字符直接比较 */
                if (c1 != c2) {
                    break;
                }
            }
        }
        if (j == nlen) {
            return true;
        }
    }

    return false;
}


PasswordStrength evaluatePasswordStrength(const char *password) {
    if (password == NULL) {
        return PASSWORD_STRENGTH_WEAK;
    }

    int has_digit = 0;
    int has_upper = 0;
    int has_lower = 0;
    int has_symbol = 0;
    int only_digits = 1;

    size_t len = 0;

    for (len = 0; password[len] != '\0'; ++len) {
        unsigned char c = (unsigned char)password[len];

        if (c >= '0' && c <= '9') {
            has_digit = 1;
        } else if (c >= 'A' && c <= 'Z') {
            has_upper = 1;
            only_digits = 0;
        } else if (c >= 'a' && c <= 'z') {
            has_lower = 1;
            only_digits = 0;
        } else {
            // 其他都算符号
            has_symbol = 1;
            only_digits = 0;
        }
    }

    int type_count = has_digit + has_upper + has_lower + has_symbol;

    // 4. 极强：长度 >= 16 且 4 类字符全部包含
    if (len >= 16 && type_count == 4) {
        return PASSWORD_STRENGTH_VERY_STRONG;
    }

    // 3. 高：长度 >= 8 且 至少 3 类字符
    if (len >= 8 && type_count >= 3) {
        return PASSWORD_STRENGTH_STRONG;
    }

    // 2. 中：长度 >= 8 且 至少 2 类字符
    if (len >= 8 && type_count >= 2) {
        return PASSWORD_STRENGTH_MEDIUM;
    }

    // 1. 低：长度 >= 6 且全部为数字
    if (len >= 6 && only_digits) {
        return PASSWORD_STRENGTH_WEAK;
    }

    // 其它情况（长度太短等），也视为弱
    return PASSWORD_STRENGTH_WEAK;
}

/* 判断是否有相邻 3 位或以上相同字符 */
bool auxHasOverlappingChars(const char *password) {
    if (password == NULL) {
        return false;
    }

    size_t len = strlen(password);
    if (len < 3) {
        return false;
    }

    for (size_t i = 2; i < len; ++i) {
        if (password[i] == password[i - 1] &&
            password[i] == password[i - 2]) {
            return true;
        }
    }

    return false;
}

char *auxHasOverlappingCharsToStr(const char *password) {
    if (password == NULL) {
        return NULL;
    }

    size_t len = strlen(password);
    if (len < 3) {
        return NULL;
    }

    size_t start = 0;
    size_t count = 1;
    char current_char = password[0];

    for (size_t i = 1; i < len; ++i) {
        if (password[i] == current_char) {
            count++;
            if (count >= 3) {
                // 当连续字符达到 3 位时，记录结束位置
                size_t end = i + 1;
                size_t substring_len = end - start;
                char *result = (char *)malloc(substring_len + 1);
                if (result == NULL) {
                    return NULL;
                }
                // 截取子字符串
                strncpy(result, password + start, substring_len);
                result[substring_len] = '\0';
                return result;
            }
        } else {
            // 重置计数器和当前字符
            current_char = password[i];
            start = i;
            count = 1;
        }
    }

    // 如果没有找到符合条件的字符部分，返回空字符串
    return NULL;
}

/* 判断是否有相邻 3 位或以上连续递增/递减（包括数字、字母） */
bool auxHasConsecutiveChars(const char *password) {
    if (password == NULL) {
        return false;
    }

    size_t len = strlen(password);
    if (len < 3) {
        return false;
    }

    /* 将 password 统一转成大写后进行判断 */
    char *tmp = (char *)malloc(len + 1);
    if (tmp == NULL) {
        return false;
    }

    for (size_t i = 0; i < len; ++i) {
        tmp[i] = (char)toupper((unsigned char)password[i]);
    }
    tmp[len] = '\0';

    int maxCount = 0;
    int addCount = 0;
    int subCount = 0;
    size_t i = 0;

    while (i + 1 < len) {
        if ((unsigned char)tmp[i] + 1 == (unsigned char)tmp[i + 1]) {
            ++addCount;
        } else {
            if (addCount > maxCount) {
                maxCount = addCount;
            }
            addCount = 0;
        }

        if ((unsigned char)tmp[i] - 1 == (unsigned char)tmp[i + 1]) {
            ++subCount;
        } else {
            if (subCount > maxCount) {
                maxCount = subCount;
            }
            subCount = 0;
        }

        ++i;
    }

    if (addCount > maxCount) {
        maxCount = addCount;
    }
    if (subCount > maxCount) {
        maxCount = subCount;
    }

    free(tmp);

    /* 连续 3 位及以上 => 相邻差计数 >= 2 */
    return (maxCount >= 2);
}

//连续递增或递减的三位及以上数字序列（以字符串形式），如果不存在则返回空字符串
char *auxHasConsecutiveNumToStr(const char *str) {
    int len = strlen(str);
    if (len < 3) {
        return NULL; // 长度不足3位，返回空字符串
    }
    
    // 检查字符串是否仅包含数字字符
    for (int i = 0; i < len; i++) {
        if (str[i] < '0' || str[i] > '9') {
            return NULL; // 包含非数字字符，返回空字符串
        }
    }
    
    int *digits = (int *)malloc(len * sizeof(int));
    int *diffs = (int *)malloc((len - 1) * sizeof(int));
    
    if (digits == NULL || diffs == NULL) {
        // 内存分配失败，返回空字符串
        free(digits);
        free(diffs);
        return NULL;
    }
    
    // 将字符串转换为整数数组
    for (int i = 0; i < len; i++) {
        digits[i] = str[i] - '0';
    }
    
    // 计算相邻数字的差值数组
    for (int i = 0; i < len - 1; i++) {
        diffs[i] = digits[i + 1] - digits[i];
    }
    
    // 检查是否有连续递增或递减的三位及以上数字
    for (int i = 0; i < len - 2; i++) {
        // 检查连续递增（差值为+1）
        if (diffs[i] == 1 && diffs[i + 1] == 1) {
            // 返回第一次出现的连续递增三位数字
            char *result = (char *)malloc(4 * sizeof(char)); // 3位数字 + 结束符
            if (result == NULL) {
                free(digits);
                free(diffs);
                return NULL;
            }
            snprintf(result, 4, "%c%c%c", str[i], str[i + 1], str[i + 2]);
            free(digits);
            free(diffs);
            return result;
        }
        // 检查连续递减（差值为-1）
        if (diffs[i] == -1 && diffs[i + 1] == -1) {
            // 返回第一次出现的连续递减三位数字
            char *result = (char *)malloc(4 * sizeof(char)); // 3位数字 + 结束符
            if (result == NULL) {
                free(digits);
                free(diffs);
                return NULL;
            }
            snprintf(result, 4, "%c%c%c", str[i], str[i + 1], str[i + 2]);
            free(digits);
            free(diffs);
            return result;
        }
    }
    
    // 如果没有找到连续递增或递减的三位及以上数字，返回空字符串
    free(digits);
    free(diffs);
    return NULL;
}

char *auxHasConsecutiveLetterToStr(const char *str) {
    int len = strlen(str);
    if (len < 3) {
        return NULL; // 长度不足3位，返回空字符串
    }
    
    // 将字符串转换为全小写（统一处理大小写）
    char *lower_str = (char *)malloc(len * sizeof(char));
    for (int i = 0; i < len; i++) {
        lower_str[i] = tolower(str[i]);
    }
    
    // 检查是否存在连续递增或递减的三位及以上字母序列
    for (int i = 0; i < len - 2; i++) {
        // 检查连续递增（如 a->b->c）
        if ((lower_str[i] + 1 == lower_str[i + 1]) && 
            (lower_str[i + 1] + 1 == lower_str[i + 2])) {
            // 返回第一次出现的连续递增三位字母
            char *result = (char *)malloc(4 * sizeof(char)); // 3位字母 + 结束符
            snprintf(result, 4, "%c%c%c", str[i], str[i + 1], str[i + 2]);
            free(lower_str);
            return result;
        }
        // 检查连续递减（如 c->b->a）
        if ((lower_str[i] - 1 == lower_str[i + 1]) && 
            (lower_str[i + 1] - 1 == lower_str[i + 2])) {
            // 返回第一次出现的连续递减三位字母
            char *result = (char *)malloc(4 * sizeof(char)); // 3位字母 + 结束符
            snprintf(result, 4, "%c%c%c", str[i], str[i + 1], str[i + 2]);
            free(lower_str);
            return result;
        }
    }
    
    // 释放内存
    free(lower_str);
    return NULL;
}

/* 内部工具：检查 [start, end) 与其后同长度子串是否相同 */
static bool checkSame(const char *password, int start, int end) {
    int len        = (int)strlen(password);
    int frontRange = end - start;
    int tailRange  = len - end;

    if (tailRange - frontRange < 0) {
        return false;
    }

    for (int i = 0; i < frontRange; ++i) {
        if (password[start + i] != password[end + i]) {
            return false;
        }
    }

    return true;
}

/* 判断是否有重复序列 */
bool auxHasPeriodicSequence(const char *password) {
    if (password == NULL) {
        return false;
    }

    int len = (int)strlen(password);
    if (len < 4) {
        /* 长度太短不可能有长度 >=2 的重复序列 */
        return false;
    }

    int front = 0;
    int tail  = 2;

    while (front < len && tail < len) {
        if (password[front] != password[tail]) {
            ++tail;
            if (tail == len) {
                ++front;
                tail = front + 2;
            }
        } else {
            if (checkSame(password, front, tail)) {
                return true;
            }
            ++front;
            tail = front + 2;
        }
    }

    return false;
}

char *auxHasPeriodicSequenceToStr(const char *str) {
    int len = strlen(str);
    if (len < 4) {
        return NULL; // 长度不足4位，无法形成周期性重复序列
    }

    // 将字符串转换为全小写（统一处理大小写）
    char *lower_str = (char *)malloc(len * sizeof(char));
    for (int i = 0; i < len; i++) {
        lower_str[i] = tolower(str[i]);
    }

    // 检查所有可能的周期性序列
    for (int k = 2; k <= len / 2; k++) { // k是周期长度，从2开始
        for (int i = 0; i <= len - 2 * k; i++) { // 检查从i开始的2k长度字符串
            int match = 1;
            for (int j = 0; j < k; j++) {
                if (lower_str[i + j] != lower_str[i + j + k]) {
                    match = 0;
                    break;
                }
            }
            if (match) {
                // 返回第一次出现的周期性序列
                char *result = (char *)malloc((k + 1) * sizeof(char)); // k位字母 + 结束符
                strncpy(result, str + i, k);
                result[k] = '\0'; // 添加字符串结束符
                free(lower_str);
                return result;
            }
        }
    }

    // 释放内存
    free(lower_str);
    return NULL;
}

/* 检查是否有基于 QWERTY 键盘布局的连续 3 位及以上字母 */
bool auxHasQwertySequence(const char *password) {
    if (password == NULL) {
        return false;
    }

    size_t len = strlen(password);
    if (len < 3) {
        return false;
    }

    const char *qwertyChars =
        "qwertyuiopasdfghjklzxcvbnmnbvcxzlkjhgfdsapoiuytrewq";

    for (size_t i = 0; i <= len - 3; ++i) {
        char sub[4];
        sub[0] = (char)tolower((unsigned char)password[i]);
        sub[1] = (char)tolower((unsigned char)password[i + 1]);
        sub[2] = (char)tolower((unsigned char)password[i + 2]);
        sub[3] = '\0';

        if (strstr(qwertyChars, sub) != NULL) {
            return true;
        }
    }

    return false;
}

char *auxHasQwertySequenceToStr(const char *str) {
    int len = strlen(str);
    if (len < 3) {
        return NULL; // 长度不足3位，无法形成连续相邻序列
    }

    // 将字符串转换为全小写（统一处理大小写）
    char *lower_str = (char *)malloc(len * sizeof(char));
    for (int i = 0; i < len; i++) {
        lower_str[i] = tolower(str[i]);
    }
    lower_str[len] = '\0';

    // 定义键盘布局的三行及其反转
    const char *keyboard_patterns[] = {
        "qwertyuiop", // 第一行
        "poiuywertyq", // 第一行反转
        "asdfghjkl", // 第二行
        "lkjhgfdsa", // 第二行反转
        "zxcvbnm", // 第三行
        "mnbvcxz" // 第三行反转
    };
    int pattern_count = sizeof(keyboard_patterns) / sizeof(keyboard_patterns[0]);

    // 检查所有可能的连续序列
    for (int i = 0; i <= len - 3; i++) { // 起点i
        for (int l = 3; l <= len - i; l++) { // 子串长度l
            char *substr = (char *)malloc((l + 1) * sizeof(char));
            strncpy(substr, lower_str + i, l);
            substr[l] = '\0';

            // 检查子串是否存在于任何一个键盘布局模式中
            for (int p = 0; p < pattern_count; p++) {
                if (strstr(keyboard_patterns[p], substr) != NULL) {
                    // 返回第一次出现的连续序列
                    char *result = (char *)malloc((l + 1) * sizeof(char));
                    strncpy(result, str + i, l);
                    result[l] = '\0';
                    free(substr);
                    free(lower_str);
                    return result;
                }
            }

            free(substr);
        }
    }

    // 释放内存
    free(lower_str);
    return NULL;
}

/* 检查是否与个人信息强关联 */
bool auxIsAssociatedWithPersonalInfo(const char *password,
                                     const char *username,
                                     const char *phone,
                                     const char *email) {
    if (password == NULL) {
        return false;
    }

    size_t passLen  = strlen(password);
    size_t userLen  = (username != NULL) ? strlen(username) : 0u;
    size_t phoneLen = (phone    != NULL) ? strlen(phone)    : 0u;
    size_t emailLen = (email    != NULL) ? strlen(email)    : 0u;

    char *lowerPass  = (char *)malloc(passLen  + 1);
    char *lowerUser  = (char *)malloc(userLen  + 1);
    char *lowerPhone = (char *)malloc(phoneLen + 1);
    char *lowerEmail = (char *)malloc(emailLen + 1);

    if (!lowerPass || (!lowerUser && userLen > 0) || (!lowerPhone && phoneLen > 0) || (!lowerEmail && emailLen > 0)) {
        free(lowerPass);
        free(lowerUser);
        free(lowerPhone);
        free(lowerEmail);
        return false;
    }

    /* 转小写的密码 */
    for (size_t i = 0; i < passLen; ++i) {
        lowerPass[i] = (char)tolower((unsigned char)password[i]);
    }
    lowerPass[passLen] = '\0';

    /* 用户名转小写 */
    if (username != NULL) {
        for (size_t i = 0; i < userLen; ++i) {
            lowerUser[i] = (char)tolower((unsigned char)username[i]);
        }
        lowerUser[userLen] = '\0';
    } else if (lowerUser) {
        lowerUser[0] = '\0';
    }

    /* 手机号：原代码并没有转小写，这里也保持，只是直接拷贝 */
    if (phone != NULL) {
        for (size_t i = 0; i < phoneLen; ++i) {
            lowerPhone[i] = phone[i];
        }
        lowerPhone[phoneLen] = '\0';
    } else if (lowerPhone) {
        lowerPhone[0] = '\0';
    }

    /* 邮箱，这里也保持，只是直接拷贝 */
    if (email != NULL) {
        for (size_t i = 0; i < emailLen; ++i) {
            lowerEmail[i] = email[i];
        }
        lowerEmail[emailLen] = '\0';
    } else if (lowerEmail) {
        lowerEmail[0] = '\0';
    }

    bool associated = false;

    /* 检查用户名中任意连续 3 位及以上序列是否出现在密码中 */
    if (userLen >= 3) {
        for (size_t i = 0; i <= userLen - 3; ++i) {
            char sub[4];
            sub[0] = lowerUser[i];
            sub[1] = lowerUser[i + 1];
            sub[2] = lowerUser[i + 2];
            sub[3] = '\0';

            if (strstr(lowerPass, sub) != NULL) {
                associated = true;
                goto cleanup;
            }
        }
    }

    /* 检查手机号中任意连续 3 位及以上序列是否出现在密码中 */
    if (phoneLen >= 3) {
        for (size_t i = 0; i <= phoneLen - 3; ++i) {
            char sub[4];
            sub[0] = lowerPhone[i];
            sub[1] = lowerPhone[i + 1];
            sub[2] = lowerPhone[i + 2];
            sub[3] = '\0';

            if (strstr(lowerPass, sub) != NULL) {
                associated = true;
                goto cleanup;
            }
        }
    }

    if (emailLen >= 3) {
        for (size_t i = 0; i <= emailLen - 3; ++i) {
            char sub[4];
            sub[0] = lowerEmail[i];
            sub[1] = lowerEmail[i + 1];
            sub[2] = lowerEmail[i + 2];
            sub[3] = '\0';

            if (strstr(lowerPass, sub) != NULL) {
                associated = true;
                goto cleanup;
            }
        }
    }

cleanup:
    free(lowerPass);
    free(lowerUser);
    free(lowerPhone);
    free(lowerEmail);
    return associated;
}

char *auxIsAssociatedWithPersonalInfoToStr(const char *password, const char *name) {
    int len_name = strlen(name);
    int len_password = strlen(password);
    
    if (len_name < 3 || len_password < 3) {
        return NULL;
    }
    
    // 遍历 name 中所有可能的连续 3 位子串
    for (int i = 0; i <= len_name - 3; i++) {
        char sub[4];  // 用于存储 3 位子串和空字符
        strncpy(sub, name + i, 3);
        sub[3] = '\0';  // 确保子串以空字符结束
        
        // 检查 password 是否包含该子串
        const char *found = strstr(password, sub);
        if (found != NULL) {
            // 分配内存并返回该子串
            char *result = (char *)malloc(4);
            if (result == NULL) {
                return NULL; // 处理内存分配失败
            }
            strncpy(result, sub, 3);
            result[3] = '\0';
            return result;
        }
    }
    
    return NULL; // 未找到匹配的子串
}

const char *auxContainsWeakPasswordWordsToStr(const char *password) {
    if (password == NULL) {
        return NULL;
    }

    for (size_t i = 0; i < WeakPasswordWordsSummaryCount; ++i) {
        const char *word = WeakPasswordWordsSummary[i];
        if (containsCaseInsensitive(password, word)) {
            return word;
        }
    }

    return NULL;
}

int isWeakPassword(const char* password, const char *username, const char *phone, const char *email) {
    const char *pwd = password;
    if (pwd == NULL || *pwd == '\0') {
        return ERR_PARAM;
    }
    //判断重复字符（相邻 3 位及以上相同）
    if (auxHasOverlappingChars(pwd)) {
        return PASS_ERR_REPEAT_CHAR;
    }
    //判断连续数字——连续字母（相邻 3 位及以上递增/递减）
    if (auxHasConsecutiveChars(pwd)) {
        return PASS_ERR_SEQ_CHAR;
    }
    //判断重复序列
    if (auxHasPeriodicSequence(pwd)) {
        return PASS_ERR_REPEAT_BLK;
    }
    //判断键盘键位（QWERTY 键盘上的连续 3 位及以上字母）
    if (auxHasQwertySequence(pwd)) {
        return PASS_ERR_KEYBOARD;
    }
    //判断密码与用户信息强关联
    if (auxIsAssociatedWithPersonalInfo(pwd, username, phone, email)) {
        return PASS_ERR_USER_INFO;
    }

    return OK;
}


WeakPasswordResult isWeakPasswordToStr(const char* password, const char* username, const char* userphone, const char* useremail) {
    WeakPasswordResult result;
    char *temp_result = NULL;
    const char *pwd = password;
    result.conditionCode = 0;
    result.conditionDescription = NULL;

    if (password == NULL) {
        return result;
    }

    // 判断重复字符（相邻3位及以上相同）
    temp_result = auxHasOverlappingCharsToStr(pwd);
    if (temp_result != NULL && temp_result[0] != '\0') {
        // 复制字符串内容到新的内存块
        size_t desc_len = strlen(temp_result) + 1;
        result.conditionDescription = (char *)malloc(desc_len);
        if (result.conditionDescription == NULL) {
            free(temp_result);
            return result; // 内存分配失败，返回初始值
        }
        strncpy(result.conditionDescription, temp_result, desc_len);
        free(temp_result);
        result.conditionCode = 1;
        return result;
    }

    // 连续递增或递减的三位及以上数字序列
    temp_result = auxHasConsecutiveNumToStr(pwd);
    if (temp_result != NULL) {
        // 复制字符串内容到新的内存块
        size_t desc_len = strlen(temp_result) + 1;
        result.conditionDescription = (char *)malloc(desc_len);
        if (result.conditionDescription == NULL) {
            free(temp_result);
            return result; // 内存分配失败，返回初始值
        }
        strncpy(result.conditionDescription, temp_result, desc_len);
        free(temp_result);
        result.conditionCode = 2;
        return result;
    }

    // 不允许出现连续字母：相邻3位及以上连续递增/递减字母（不分大小写）
    temp_result = auxHasConsecutiveLetterToStr(pwd);
    if (temp_result != NULL) {
        // 复制字符串内容到新的内存块
        size_t desc_len = strlen(temp_result) + 1;
        result.conditionDescription = (char *)malloc(desc_len);
        if (result.conditionDescription == NULL) {
            free(temp_result);
            return result; // 内存分配失败，返回初始值
        }
        strncpy(result.conditionDescription, temp_result, desc_len);
        free(temp_result);
        result.conditionCode = 3;
        return result;
    }

    // 判断重复序列
    temp_result = auxHasPeriodicSequenceToStr(pwd);
    if (temp_result != NULL) {
        // 复制字符串内容到新的内存块
        size_t desc_len = strlen(temp_result) + 1;
        result.conditionDescription = (char *)malloc(desc_len);
        if (result.conditionDescription == NULL) {
            free(temp_result);
            return result; // 内存分配失败，返回初始值
        }
        strncpy(result.conditionDescription, temp_result, desc_len);
        free(temp_result);
        result.conditionCode = 4;
        return result;
    }

    // 存在基于QWERTY键盘布局的相邻3位及以上连续字母
    temp_result = auxHasQwertySequenceToStr(pwd);
    if (temp_result != NULL) {
        // 复制字符串内容到新的内存块
        size_t desc_len = strlen(temp_result) + 1;
        result.conditionDescription = (char *)malloc(desc_len);
        if (result.conditionDescription == NULL) {
            free(temp_result);
            return result; // 内存分配失败，返回初始值
        }
        strncpy(result.conditionDescription, temp_result, desc_len);
        free(temp_result);
        result.conditionCode = 5;
        return result;
    }

    if(strlen(username)  >= 3){
        temp_result = auxIsAssociatedWithPersonalInfoToStr(password, username);
            if (temp_result != NULL) {
            // 复制字符串内容到新的内存块
            size_t desc_len = strlen(temp_result) + 1;
            result.conditionDescription = (char *)malloc(desc_len);
            if (result.conditionDescription == NULL) {
                free(temp_result);
                return result; // 内存分配失败，返回初始值
            }
            strncpy(result.conditionDescription, temp_result, desc_len);
            free(temp_result);
            result.conditionCode = 6;
            return result;
        }

    }
    if(strlen(userphone)  >= 3){
        temp_result = auxIsAssociatedWithPersonalInfoToStr(password, userphone);
            if (temp_result != NULL) {
            // 复制字符串内容到新的内存块
            size_t desc_len = strlen(temp_result) + 1;
            result.conditionDescription = (char *)malloc(desc_len);
            if (result.conditionDescription == NULL) {
                free(temp_result);
                return result; // 内存分配失败，返回初始值
            }
            strncpy(result.conditionDescription, temp_result, desc_len);
            free(temp_result);
            result.conditionCode = 7;
            return result;
        }
    }
    if(strlen(useremail)  >= 3){
        temp_result = auxIsAssociatedWithPersonalInfoToStr(password, useremail);
            if (temp_result != NULL) {
            // 复制字符串内容到新的内存块
            size_t desc_len = strlen(temp_result) + 1;
            result.conditionDescription = (char *)malloc(desc_len);
            if (result.conditionDescription == NULL) {
                free(temp_result);
                return result; // 内存分配失败，返回初始值
            }
            strncpy(result.conditionDescription, temp_result, desc_len);
            free(temp_result);
            result.conditionCode = 8;
            return result;
        }
    }
    return result;
}


int check_password(const char *password,const char *username,const char *phone) {

    if (password == NULL)         
        return ERR_PARAM;

    /* 1. 弱密码类型检测 */
     int weak = isWeakPassword(password, username, phone,NULL);
     printf("弱密码类型: %s\n", weak ? "是" : "否");

    /* 2. 强度等级检测 */
    PasswordStrength level = evaluatePasswordStrength(password);
    printf("密码强度等级: %s\n", strength_names[level]);

    /* 3. 弱口令关键词检测 */
    const char *hit = auxContainsWeakPasswordWordsToStr(password);
    if (hit)
        printf("检测到弱口令关键词: '%s'\n", hit);
    else
        printf("未检测到常见弱口令\n");

    if(weak != OK )
        return weak;
    if (level <= PASSWORD_STRENGTH_WEAK )
        return PASS_ERR_STRENGTH_LOW;
    if (hit)
        return PASS_ERR_WEAK_WORD;

    return OK; 
}