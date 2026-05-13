/*代码转换:从一种编码转为另一种编码*/
#include "image_iconv.h"
#include "iconv.h"
#include "nslog.h"
#include <string.h>
#include <stdlib.h>
#include <errno.h>
int code_convert(char *from_charset, char *to_charset, char *inbuf, int inlen, char *outbuf, int *outlen)
{
	iconv_t cd;
	char **pin = &inbuf;
	char **pout = &outbuf;
	cd = iconv_open(to_charset, from_charset);

	if((iconv_t) - 1 == cd) {
		nslog(NS_ERROR, "code_convert:open the char set failed,errno = %d,strerror(errno) = %s \n", errno, strerror(errno));
		return -1;
	}

	memset(outbuf, 0, *outlen);

	if(iconv(cd, pin, (size_t *)&inlen, pout, (size_t *)outlen) == (size_t) - 1) {
		nslog(NS_ERROR, "code_convert:conver char set failed,errno = %d,strerror(errno) = %s \n", errno, strerror(errno));
		return -1;
	}

	iconv_close(cd);
	return 0;
}
