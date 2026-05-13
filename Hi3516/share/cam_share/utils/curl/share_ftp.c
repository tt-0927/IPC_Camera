#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include "curl.h"
#include "share_ftp.h"


/*****************util api******************/

//改方法获得的文件超过4G会出错
static int ftp_get_file_size(FILE *file)
{
	int size = 0;
	fseek(file, 0L, SEEK_END);
	size = ftell(file);
	fseek(file, 0L, SEEK_SET);
	return size;
}

static long long ftp_getFile_size2(char* filename)
{
    struct stat statbuf;
    stat(filename,&statbuf);
    long long size = statbuf.st_size;

    return size;
}



/******************curl api****************/
CURL *curl_init()
{
	curl_global_init(CURL_GLOBAL_DEFAULT);
	CURL *curl = curl_easy_init();
	if(NULL == curl)
	{
		fprintf(stderr, "Init curl failed.\n");
		exit(1);
	}
	return curl;
}

static size_t read_callback(void *ptr, size_t size, size_t nmemb, void *stream)
{
  curl_off_t nread;
  size_t retcode = fread(ptr, size, nmemb,(FILE*)stream);
   nread = (curl_off_t)retcode;

  fprintf(stderr, "*** We read %"CURL_FORMAT_CURL_OFF_T"bytes from file\n",nread);
  return retcode;
}

size_t OnReadHeader(char *ptr, size_t size, size_t nmemb, void *stream)
{
	int r;
    long len = 0;

    r = sscanf((const char*)ptr, "Content-Length: %ld\n", &len);
    if (r) /* Microsoft: we don't read the specs */
        *((long *) stream) = len;
	printf("len : %ld\n",len);
    return size * nmemb;
}


void curl_set_upload_opt(CURL *ptrCurl, const char *pchUrl, const char *pchUserKey, FILE *File,char *pchFileName,char *pchErrBuf,UploadValueFunc pValueFun)
{
	curl_easy_setopt(ptrCurl, CURLOPT_URL, pchUrl);
	curl_easy_setopt(ptrCurl, CURLOPT_USERPWD, pchUserKey);
	curl_easy_setopt(ptrCurl, CURLOPT_READDATA, File);

	curl_easy_setopt(ptrCurl, CURLOPT_UPLOAD, 1L);
//	curl_easy_setopt(ptrCurl, CURLOPT_RESUME_FROM, -1); // -1 偏移到文件尾，打开无法上传文件
	curl_easy_setopt(ptrCurl, CURLOPT_INFILESIZE_LARGE, ftp_getFile_size2(pchFileName));
	curl_easy_setopt(ptrCurl, CURLOPT_FTP_CREATE_MISSING_DIRS, 2);
	//设置错误缓冲区
	curl_easy_setopt(ptrCurl,CURLOPT_ERRORBUFFER,pchErrBuf);
	curl_easy_setopt(ptrCurl,CURLOPT_NOPROGRESS, 0);
	curl_easy_setopt(ptrCurl,CURLOPT_PROGRESSFUNCTION,pValueFun);
	curl_easy_setopt(ptrCurl, CURLOPT_PROGRESSDATA, "flag");

	// curl_easy_setopt(ptrCurl, CURLOPT_NOBODY, 1L);
	// curl_easy_setopt(ptrCurl, CURLOPT_HEADERFUNCTION, OnReadHeader);
	//curl_easy_setopt(curl, CURLOPT_READFUNCTION, read_callback);
	//curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);
}

long curl_set_upload_opt_continue(CURL *ptrCurl, const char *pchUrl, const char *pchUserKey, FILE *File,char *pchFileName,char *pchErrBuf,UploadValueFunc pValueFun)
{
	long uploaded_len = 0;
	curl_easy_setopt(ptrCurl, CURLOPT_URL, pchUrl);
	curl_easy_setopt(ptrCurl, CURLOPT_USERPWD, pchUserKey);
	curl_easy_setopt(ptrCurl, CURLOPT_READDATA, File);
	curl_easy_setopt(ptrCurl, CURLOPT_UPLOAD, 1);
	curl_easy_setopt(ptrCurl, CURLOPT_RESUME_FROM, 0);
	curl_easy_setopt(ptrCurl, CURLOPT_INFILESIZE_LARGE, ftp_getFile_size2(pchFileName));
	curl_easy_setopt(ptrCurl, CURLOPT_FTP_CREATE_MISSING_DIRS, 2);
	//设置错误缓冲区
	curl_easy_setopt(ptrCurl,CURLOPT_ERRORBUFFER,pchErrBuf);
	curl_easy_setopt(ptrCurl,CURLOPT_NOPROGRESS, 0);
	curl_easy_setopt(ptrCurl,CURLOPT_PROGRESSFUNCTION,pValueFun);
	curl_easy_setopt(ptrCurl, CURLOPT_PROGRESSDATA, "flag");

	curl_easy_setopt(ptrCurl, CURLOPT_NOBODY, 1L);
	curl_easy_setopt(ptrCurl, CURLOPT_HEADERFUNCTION, OnReadHeader);
	curl_easy_setopt(ptrCurl, CURLOPT_HEADERDATA, &uploaded_len);
	//curl_easy_setopt(curl, CURLOPT_READFUNCTION, read_callback);
	//curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);

	return uploaded_len;
}

void curl_set_download_opt(CURL *curl, const char *url, const char *user_key, FILE *file)
{
	curl_easy_setopt(curl, CURLOPT_URL, url);
	curl_easy_setopt(curl, CURLOPT_USERPWD, user_key);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, file);
	curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);
}

void curl_exit(CURL *curl)
{
	curl_easy_cleanup(curl);
	curl_global_cleanup();
}

CURLcode curl_perform(CURL *curl)
{
	CURLcode ret = curl_easy_perform(curl);
	if(ret != 0)
	{
		fprintf(stderr, "Perform curl failed[%d]\n",ret);
		//curl_exit(curl);
		return ret;
	}
	return ret;
}

/****************ftp upload & download api******************/
FTP_STATE ftp_upload(FTP_OPT ftp_option)
{
	FTP_STATE state;
	CURL *curl = NULL;
	char errbuf[CURL_ERROR_SIZE] = {0};
	FILE *fp = fopen(ftp_option.file, "rb");
	if(NULL == fp)
	{
		printf("ftp_option.file=%s\n",ftp_option.file);
		fprintf(stderr, "Open file:%s failed at %s:%d\n",ftp_option.file,__FILE__, __LINE__);
		return FTP_UPLOAD_FAILED;
	}

	curl = curl_init();
	if(curl == NULL)
	{
		printf("curl_init error!!\n");
		return -1;
	}
	curl_set_upload_opt(curl, ftp_option.url, ftp_option.user_key, fp,ftp_option.file,errbuf,ftp_option.valueFun);
	if(CURLE_OK == curl_perform(curl))
	{
		state = FTP_UPLOAD_SUCCESS;
	}else
	{

		state = FTP_UPLOAD_FAILED;
		printf("==========ftp upload error:%s version[%s]\n",errbuf,curl_version());
	}
	curl_exit(curl);
	fclose(fp);
	return state;
}

FTP_STATE ftp_download(FTP_OPT ftp_option)
{
	FTP_STATE state;
	CURL *curl;
	FILE *fp = fopen(ftp_option.file, "w");
	if(NULL == fp)
	{
		fprintf(stderr, "Open file failed at %s:%d\n", __FILE__, __LINE__);
		return FTP_UPLOAD_FAILED;
	}

	curl = curl_init();
	curl_set_download_opt(curl, ftp_option.url, ftp_option.user_key, fp);
	if(CURLE_OK == curl_perform(curl))
		state = FTP_DOWNLOAD_SUCCESS;
	else
		state = FTP_DOWNLOAD_FAILED;

	curl_exit(curl);
	fclose(fp);
	return state;
}

//断点上传
FTP_STATE ftp_upload_continue(FTP_OPT ftp_option)
{
	FTP_STATE state;
	CURL *ptrCurl = NULL;
	char errbuf[CURL_ERROR_SIZE] = {0};
	FILE *fp = fopen(ftp_option.file, "r");
	if(NULL == fp)
	{
		printf("ftp_option.file=%s\n",ftp_option.file);
		fprintf(stderr, "Open file:%s failed at %s:%d\n",ftp_option.file,__FILE__, __LINE__);
		return FTP_UPLOAD_FAILED;
	}

	ptrCurl = curl_init();
	if(ptrCurl == NULL)
	{
		printf("curl_init error!!\n");
		return -1;
	}
	long uploaded_len = 0;
	curl_easy_setopt(ptrCurl, CURLOPT_URL, ftp_option.url);
	curl_easy_setopt(ptrCurl, CURLOPT_USERPWD, ftp_option.user_key);
	curl_easy_setopt(ptrCurl, CURLOPT_READDATA, fp);
	curl_easy_setopt(ptrCurl, CURLOPT_UPLOAD, 1);
	curl_easy_setopt(ptrCurl, CURLOPT_RESUME_FROM,-1);
	curl_easy_setopt(ptrCurl, CURLOPT_INFILESIZE_LARGE, ftp_getFile_size2(ftp_option.file));
	curl_easy_setopt(ptrCurl, CURLOPT_FTP_CREATE_MISSING_DIRS, 2);
	//设置错误缓冲区
	curl_easy_setopt(ptrCurl,CURLOPT_ERRORBUFFER,errbuf);
	// curl_easy_setopt(ptrCurl,CURLOPT_NOPROGRESS, 0);
	// curl_easy_setopt(ptrCurl,CURLOPT_PROGRESSFUNCTION,ftp_option.valueFun);
	// curl_easy_setopt(ptrCurl, CURLOPT_PROGRESSDATA, "flag");

	curl_easy_setopt(ptrCurl, CURLOPT_NOBODY, 1L);
	curl_easy_setopt(ptrCurl, CURLOPT_HEADER, 1L);
	curl_easy_setopt(ptrCurl, CURLOPT_HEADERFUNCTION, OnReadHeader);
	curl_easy_setopt(ptrCurl, CURLOPT_HEADERDATA, &uploaded_len);
	//curl_easy_setopt(curl, CURLOPT_READFUNCTION, read_callback);
	if(CURLE_OK == curl_perform(ptrCurl))
	{
		curl_easy_setopt(ptrCurl, CURLOPT_NOBODY, 0L);
        curl_easy_setopt(ptrCurl, CURLOPT_HEADER, 0L);
		printf("uploaded_len : %ld \n",uploaded_len);
		fseek(fp, uploaded_len, SEEK_SET);
		curl_easy_setopt(ptrCurl, CURLOPT_READDATA, fp);
		curl_easy_setopt(ptrCurl, CURLOPT_INFILESIZE_LARGE, ftp_getFile_size2(ftp_option.file) - uploaded_len);
		printf("%s\n",ftp_option.file);
		curl_perform(ptrCurl);
		if(CURLE_OK == curl_perform(ptrCurl))
		{
			state = FTP_UPLOAD_SUCCESS;
		}else
		{
			state = FTP_UPLOAD_FAILED;
			printf("==========ftp upload continue error:%s version[%s]\n",errbuf,curl_version());
		}
	}else
	{
		state = FTP_UPLOAD_FAILED;
		printf("==========ftp upload continue error:%s version[%s]\n",errbuf,curl_version());
	}
	curl_exit(ptrCurl);
	fclose(fp);
	return state;
}


