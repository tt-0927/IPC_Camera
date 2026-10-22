#ifndef _SHARE_FTP_H_
#define _SHARE_FTP_H_
#include "curl.h"
typedef int (*UploadValueFunc) (const char* flag, double dltotal,double dlnow,double ultotal,double ulnow);

typedef struct __FTP_CURL_INFO__
{
	int port;
	char Ip[16];
	char Passwd[64];
	char User[64];
	char SourceFile[256];
	char DestFile[256];
	char Url[256];
}ftpCurlInfo_t;


typedef enum FTP_STATE
{
	FTP_UPLOAD_SUCCESS,
	FTP_UPLOAD_FAILED,
	FTP_DOWNLOAD_SUCCESS,
	FTP_DOWNLOAD_FAILED
}FTP_STATE;


/*FTP OPERATIONS OPTIONS*/
typedef struct FTP_OPT
{
	char url[1024];		/*url of ftp*/
	char user_key[128];		/*username:password*/
	char file[512];		/*filepath*/
	UploadValueFunc valueFun;

}FTP_OPT;


/*upload file to ftp server*/
FTP_STATE ftp_upload(const FTP_OPT ftp_option);

FTP_STATE ftp_upload_continue(const FTP_OPT ftp_option);

/*download file from ftp server*/
FTP_STATE ftp_download(const FTP_OPT ftp_option);


void curl_exit(CURL *curl);
#endif
