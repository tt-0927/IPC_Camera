
#ifndef _RTP_SOURCE_ERROR_LIST_INCLUDE_
#define _RTP_SOURCE_ERROR_LIST_INCLUDE_


#include <stdio.h>
#include <string.h>
#include <stdlib.h>



typedef struct _rtp_error_list_
{
	int error;
	char reason[128];

}rtp_error_t;


#define RTP_SUCCESS	0
#define RTP_HEADSIZEERROR	-2
#define RTP_VERSIONERROR	-3
#define RTP_EXTERNLENERROR	-4
#define RTP_PADDINGERROR	-5
#define RTP_NONSUPPORTPAYLOAD	-6
#define RTP_EINVAL			-7		/*参数不对*/
#define RTP_NALUNDEFINED	-8		/*nal undefefined*/
#define RTP_ENOMEM 			-9		/*分配内存失败，Out of memory*/
#define RTP_E2BIG 			-10		/*invalid packet*/
#define RTP_LOSTPACK		-11		/*抛弃该数据包*/

char* rtp_error_getReason(int error);

#endif //_RTP_SOURCE_ERROR_LIST_INCLUDE_



