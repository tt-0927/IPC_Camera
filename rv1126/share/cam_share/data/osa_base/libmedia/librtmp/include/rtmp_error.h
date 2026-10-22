
#ifndef _RTP_SOURCE_ERROR_LIST_INCLUDE_
#define _RTP_SOURCE_ERROR_LIST_INCLUDE_


#include <stdio.h>
#include <string.h>
#include <stdlib.h>



typedef struct _rtmp_error_list_
{
	int error;
	char reason[128];

}rtmp_error_t;


#define RTMP_SUCCESS	0
#define RTMP_HANDSHAKESIZE_ERROR	-2		//C1、C2的大小错误
#define RTMP_VERSIONERROR	-3		//版本错误
#define RTMP_CHUNKSTREAMID_ERROR	-4	//rtmp块流ID错误，==1 == 0
#define RTMP_CHUNKTYPE_ERROR	-5		//RTMP的chunk类型错误
#define RTMP_STREAMID_OVERRANGE	-6		//stream id 超出范围
#define RTMP_EINVAL			-7		/*参数不对*/
#define RTMP_BASICHEADERSIZE_ERROR	-8		/*basic header size error*/

#define RTMP_MESSAGEHEADERSIZE_ERROR			-9		/*message header size error*/
#define RTMP_EXTENDED_TIMESTAMP_ERROR			-10		/*extender timestamp error*/
#define RTMP_EXTENDED_TIMESTAMPSIZE_ERROR		-11		/*extender timestamp size error*/
#define RTMP_ENOMEM 			-12		/*分配内存失败，Out of memory*/
#define RTMP_CHUNKHEADERSIZE_ERROR	-13	//RTMP header size is not 4 or 5


char* rtmp_error_getReason(int error);

#endif //_RTP_SOURCE_ERROR_LIST_INCLUDE_



