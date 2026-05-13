

#ifndef __MD5LIB__H__
#define __MD5LIB__H__

#ifdef __cplusplus
extern "C" {
#endif

/*带密钥的md5算法库md5lib.h共有下列函数：*/
char* MDString (char *, unsigned int);
char *MD5EncData(char *string, unsigned int ilen);
int MDToString(const char *pMd,unsigned int nMd5Len,\
			   char *pOutString,unsigned int nOutLen);
/*
* 输入任意一个字符串，经过md5算法处理后，返回结果：一个定长(32个字符)
* 字符串 
*/
char* MDFile (char *);
/* 
* 输入任意一个文件名，文件内容经过md5算法处理后，返回结果：一个定长
* (32个字符)字符串 
*/
char* hmac_md5(char* text, char* key);
/*
* 输入任意一个字符串text,和一个用做密钥的字符串key,经过hmac_md5算法处
* 理，返回处理结果：一个定长字符串(32个字符)
*/
char* MDFile_key (char *pFileName, char* pKey);
/*
* 输入任意一个文件名和一个用作密钥的字符串key, 经过hmac_md5算法处理
* 返回处理结果: 一个定长字符串(32个字节)
*/
char* MDFile_key_len(char* pFileName, char* pkey, unsigned int uiLen);
/*
* 输入任意一个文件名，一个用作密钥的字符串key和一个要校验的文件长度
* 经过hmac_md5算法处理
* 返回:一个定长的字符串(32个字节)
*/
#ifdef __cplusplus
}
#endif

/*
* MD5lib.h - md5 library
*/

/* 
* Copyright (C) 1990-2, RSA Data Security, Inc. Created 1990. All
* rights reserved.

* RSA Data Security, Inc. makes no representations concerning either
* the merchantability of this software or the suitability of this
* software for any particular purpose. It is provided "as is"
* without express or implied warranty of any kind.
  
* These notices must be retained in any copies of any part of this
* documentation and/or software.
*/

/* 
* The following makes MD default to MD5 if it has not already been
* defined with C compiler flags.
*/

#include <stdio.h>
#include <time.h>
#include <string.h>

#define MD 5

/* POINTER defines a generic pointer type */
typedef unsigned char *POINTER;

/* UINT2 defines a two byte word */
typedef unsigned short int UINT2;

/* UINT4 defines a four byte word */
typedef unsigned int UINT4;

/* Constants for MD5Transform routine. */
#define S11 7
#define S12 12
#define S13 17
#define S14 22
#define S21 5
#define S22 9
#define S23 14
#define S24 20
#define S31 4
#define S32 11
#define S33 16
#define S34 23
#define S41 6
#define S42 10
#define S43 15
#define S44 21

typedef struct 
{
	UINT4 state[4];                                   /* state (ABCD) */
	UINT4 count[2];									  /* number of bits, modulo 2^64 (lsb first) */
	unsigned char buffer[64];                         /* input buffer */
} MD5_CTX;

/* F, G, H and I are basic MD5 functions. */
#define F(x, y, z) (((x) & (y)) | ((~x) & (z)))
#define G(x, y, z) (((x) & (z)) | ((y) & (~z)))
#define H(x, y, z) ((x) ^ (y) ^ (z))
#define I(x, y, z) ((y) ^ ((x) | (~z)))

/* ROTATE_LEFT rotates x left n bits. */
#define ROTATE_LEFT(x, n) (((x) << (n)) | ((x) >> (32-(n))))

/* 
* FF, GG, HH, and II transformations for rounds 1, 2, 3, and 4.
* Rotation is separate from addition to prevent recomputation.
*/
#define FF(a, b, c, d, x, s, ac) { \
	(a) += F ((b), (c), (d)) + (x) + (UINT4)(ac); \
	(a) = ROTATE_LEFT ((a), (s)); \
	(a) += (b); \
	}
#define GG(a, b, c, d, x, s, ac) { \
	(a) += G ((b), (c), (d)) + (x) + (UINT4)(ac); \
	(a) = ROTATE_LEFT ((a), (s)); \
	(a) += (b); \
	}
#define HH(a, b, c, d, x, s, ac) { \
	(a) += H ((b), (c), (d)) + (x) + (UINT4)(ac); \
	(a) = ROTATE_LEFT ((a), (s)); \
	(a) += (b); \
	}
#define II(a, b, c, d, x, s, ac) { \
	(a) += I ((b), (c), (d)) + (x) + (UINT4)(ac); \
	(a) = ROTATE_LEFT ((a), (s)); \
	(a) += (b); \
	}

void MD5Init1 (MD5_CTX *context);
void MD5Update1(MD5_CTX *context, unsigned char *input,unsigned int inputLen);
void MD5Final1 (unsigned char digest[16], MD5_CTX *context);

#endif
