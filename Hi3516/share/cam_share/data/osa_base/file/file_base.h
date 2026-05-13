#ifndef __FILEOPRATE__
#define __FILEOPRATE__
#include "list_base.h"
#include "public_define.h"
#define LISTCASH 30
typedef enum
{
	DATA_FORMAT=0,//普通数据
	DATA_EOF=1,//最后一个数据
	DATA_FAIL,//外界有异常情况
}FileRecvStatus;
typedef enum
{
	FTP_START_DEL = 0,
	FTP_STOP_DEF = 1,
	FTP_Clean = 2,
	FTP_Unit = 2,//代表内部已经销毁完成可以反初始化
}FTP_FILE_STATUS;
typedef void* File_Handle_t;
/*@功能写文件
 *@[out]pBuf,内容
 *@[in]fileName文件名
 *@[in]size,大小
 *@[in]mode,例如“w”只读
 *@return 返回读入buf的个数,失败返回-1，不用再销毁
 */

int file_write_data(const void *pBuf, int size, const char *fileName, const char *mode);
/*@功能将文件内容读到buf里面,适合小文件，不占用太多内存带宽
 *@[out]pBuf,将内容放进buf中，内存有内部自动申请，最终内部释放file_free_buf
 *@[in]fileName文件名
 *@[in]set，代表从什么位置开始读0代表开始位置
 *@[in]mode,例如“r”只读
 *@return 返回读入buf的个数,失败返回-1，不用再销毁
 */
int file_auto_fread(char **pBuf, char *fileName, int set, const char *mode);
/*@功能，将创建的临时缓存buf销毁
 *@[in]pBuf，要销毁的缓存buf
 *@return 成功返回0,失败返回-1
 */
int file_free_buf(char **pBuf);


/*@功能将文件内容读到buf里面
 *@[out]pBuf,将内容放进buf中，内存由自己申请
 *@[in]fileName文件名
 *@[in]set，代表从什么位置开始读0代表开始位置
 *@[in]nSize,度多少个字节
 *@return 成功返回文件的长度，失败返回负数
 */
int file_reply_fread(char *pBuf, char *fileName, int set, int nSize, const char *mode);
/*@功能将文件内容读到buf里面
 *@[in]fileName文件名
 *@return 成功返回文件的长度，失败返回负数
 */
unsigned long long file_get_len(char *fileName);



//************************************************************************
//异步处理时得先初始化，前面为同步的
/*@由于异步处理需初始化，防止异常状况时外部可以对其通知，使其不再操作
 * @return 成功返回文件句柄File_Handle_t，失败返回空
 */
File_Handle_t file_init();

/*@反初始化
 * @return 成功返回0,失败返回-1
 */
int file_unit(File_Handle_t pFileHandle);

/*@开始处理文件时对其通知，或者外界有异常通知其停止
 * @return 成功返回0,失败返回-1
 */
int file_set_status(File_Handle_t pFileHandle, FTP_FILE_STATUS FileStatus);

/*@获取内部文件处理状态，以防外界直接销毁
 * @return 成功返回文件处理状态，失败返回-1
 */
FTP_FILE_STATUS file_get_status(File_Handle_t pFileHandle);

/*@将链表清空
 * @return 成功返回0,失败返回-1
 */
int file_clean_list(File_Handle_t pFileHandle);
/*@功能将文件内容读到链表里面，为了可以异步读取，提高效率
 *@[in]fileName文件名
 *@[in]set从文件哪里开始，开始位置为0
 *@[in]nSize每次读多少个字节，放进链表
 *@[in]lock，互斥锁
 *@[in]LISThandle链表的句柄
 *@return RetErr_t,参见RetErr_t枚举
 */
RetErr_t file_push_list(char *fileName, unsigned long long set, int nSize, File_Handle_t pFileHandle);
/*@功能将文件内容从链表取出，和file_push_list配套使用
 *@[in]fileName文件名
 *@[in][in]LISThandl链表的句柄
 *@[in][in]lock，互斥锁,与file_push_list相同
 *@[out]nSize,读入buf的个数（当为文件最后一段时，大小不一定是file_push_list进去的nSize）
 *@[out]pBuf，将内容取出，pbuf申请内存多大与file_push_list的nSize配套
 *@return RetErr_t,参见RetErr_t枚举
 */
RetErr_t file_pop_list(char * pBuf, int* nSize, File_Handle_t pFileHandle);

/*@功能将内容读到链表里面，为了可以异步存取和DataPopToFILe配套，提高效率
 *@[in]lock，互斥锁
 *@[in]LISThandle链表的句柄
 *@[in]pBuf，放入链表的数据
 *@[in]nSize,数据的大小
 *@[in]eof,参见枚举
 *@return RetErr_t,参见RetErr_t枚举
 */

RetErr_t file_dataPushTo_list(char * pBuf, int nSize, FileRecvStatus eof, File_Handle_t pFileHandle);

/*@功能将链表内 容写到文件里面和file_dataPushTo_list配套，为了可以异步存取，提高效率
 *@[in]fileName文件名
 *@[in]set从文件哪里开始，开始位置为0
 *@[in]lock，互斥锁
 *@[in]LISThandle链表的句柄
 *@return RetErr_t,参见RetErr_t枚举
 */

RetErr_t file_listPopTo_file(char *fileName, unsigned long long set, File_Handle_t pFileHandle);
/*@功能获取文件列表的缓存个数
 *@[in][in]LISThandl链表的句柄
 *@[in][in]lock，互斥锁,与file_push_list相同
 *@return 成功返回获取个数，失败返回负数
 */
int file_list_size(File_Handle_t pFileHandle);
#endif
