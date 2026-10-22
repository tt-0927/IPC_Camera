

#ifndef _CORE_SOURCE_HMACSHA1_INCLUDE_
#define _CORE_SOURCE_HMACSHA1_INCLUDE_


void hmac_sha
(
    char* k,    /* 秘钥 secret key */
    int lk,     /*  秘钥长度 length of the key in bytes */
    char* d,    /* 数据 data */
    int ld,     /*  数据长度 length of data in bytes */
    char* out,  /* 输出的字符串 output buffer, at least "t" bytes */
    int t
);



#endif //_CORE_SOURCE_HMACSHA1_INCLUDE_

