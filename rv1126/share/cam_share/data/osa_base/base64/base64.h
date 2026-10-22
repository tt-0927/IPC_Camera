

/*base64.h*/
#ifndef _CORE_SOURCE_BASE64_H
#define _CORE_SOURCE_BASE64_H

#include <stdlib.h>
#include <string.h>

unsigned char *os_base64_encode(unsigned char *str,int strlen);

unsigned char *os_base64_decode(unsigned char *code,int codeLen);

#endif	//_CORE_SOURCE_BASE64_H

