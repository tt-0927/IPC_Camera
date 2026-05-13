

#include "mpeg_error.h"



static mpeg_error_t g_mpeg_error_list[] =
{

		{MPEG_SUCCESS,"option success."},
		{MPEG_TSSIZEERROR,"ts size is error."},
		{MPEG_HEADERERROR,"ts header is not 0x47."},
		{MPEG_CRCERROR,"ts crc error."},
		{MPEG_ENOMEM,"malloc memory error."},

};


//返回具体的错误原因
char* mpeg_error_getReason(int error)
{
	char *reason = NULL;
	int index = 0;
	int total = 0;
	total = sizeof(g_mpeg_error_list[0])/sizeof(g_mpeg_error_list);
	for(index = 0;index < total;index++)
	{
		if(error == g_mpeg_error_list[index].error)
		{
			reason = g_mpeg_error_list[index].reason;
			break;
		}
	}

	return reason;
}








