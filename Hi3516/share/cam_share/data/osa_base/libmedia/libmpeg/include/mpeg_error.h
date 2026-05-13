
#ifndef _MPEG_SOURCE_ERROR_LIST_INCLUDE_
#define _MPEG_SOURCE_ERROR_LIST_INCLUDE_


#include <stdio.h>
#include <stdlib.h>


typedef struct _mpeg_error_list_
{
	int error;
	char reason[128];

}mpeg_error_t;

#define MPEG_SUCCESS	0
#define MPEG_TSSIZEERROR	-2		/*ts流的包大小不是188*/
#define MPEG_HEADERERROR	-3		/*ts包第一个字节不是0x47*/
#define MPEG_CRCERROR	-4			/*crc校验错误*/
#define MPEG_ENOMEM 		-9		/*分配内存失败，Out of memory*/
#define MPEG_PMT_COUNT_ERROR	-10		/*pmt cout != 1*/


char* mpeg_error_getReason(int error);


#endif // _MPEG_SOURCE_ERROR_LIST_INCLUDE_



