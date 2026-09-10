#ifndef MD5_H
#define MD5_H

#ifdef __cplusplus
extern "C" {
#endif

#include <string.h>
//#include "share_define.h"
 #define ACTIVATION_TIME_NUM 6 //总共激活时间枚举数量为六个
 typedef enum
 {
 	ONE_WEEK, ONE_MONTH, TWO_MONTH, THREE_MONTH, HALF_YEAR, FOREVER
 }ACTIVATION_TIME;//激活时间枚举变量：一周、一月、两月、三月、半年、永久

typedef struct
{
	unsigned int count[2];
	unsigned int state[4];
	unsigned char buffer[64];
}MD5_CTX;




#define F(x,y,z) ((x & y) | (~x & z))
#define G(x,y,z) ((x & z) | (y & ~z))
#define H(x,y,z) (x^y^z)
#define I(x,y,z) (y ^ (x | ~z))
#define ROTATE_LEFT(x,n) ((x << n) | (x >> (32-n)))

#define FF(a,b,c,d,x,s,ac){\
a += F(b, c, d) + x + ac;\
a = ROTATE_LEFT(a, s);\
a += b;\
	}

#define GG(a,b,c,d,x,s,ac){\
a += G(b, c, d) + x + ac; \
a = ROTATE_LEFT(a, s);\
a += b;\
	}

#define HH(a,b,c,d,x,s,ac){ \
a += H(b, c, d) + x + ac;\
a = ROTATE_LEFT(a, s);\
a += b; \
	}
#define II(a,b,c,d,x,s,ac){\
a += I(b, c, d) + x + ac;\
a = ROTATE_LEFT(a, s);\
a += b;\
	}

//MD5加密算法
void MD5Init(MD5_CTX *context);
void MD5Update(MD5_CTX *context, unsigned char *input, unsigned int inputlen);
void MD5Final(MD5_CTX *context, unsigned char digest[16]);
void MD5Transform(unsigned int state[4], unsigned char block[64]);
void MD5Encode(unsigned char *output, unsigned int *input, unsigned int len);
void MD5Decode(unsigned int *output, unsigned char *input, unsigned int len);
//自定义加密机器码,input参数为输入机器码，activation_time参数为输入激活时间
//digest参数必须是长度为33的字符串，其中前32位为加密位，最后一位是'\0'，作为字符串结尾标志
//返回值false，加密失败；true，加密成功
int EncodeMachineCode(unsigned char *input, ACTIVATION_TIME activation_time, unsigned char *digest);
//自定义解密算法，machine_code参数为输入机器码，activation_time参数为输出激活时间
//activation_code参数必须是长度为33的字符串，其中前32位为加密位，最后一位是'\0'，作为字符串结尾标志
//返回值false，激活失败，激活码无效；true，激活成功
int DecodeActivationCode(unsigned char *machine_code, unsigned char *activation_code, ACTIVATION_TIME *activation_time);
//16进制转unsigned char
unsigned char HexToChar(unsigned char temp);
#ifdef __cplusplus
}
#endif

#endif
