/*
 * @FilePath: sdk_net_base.c
 * @Author: yangwenyao
 * @Date: 2023-04-03 15:41:29
 * @LastEditors: ywy
 * @LastEditTime: 2023-09-18 18:50:25
 * @Descripttion: 
 */
#include <stdlib.h>
#include<stdio.h>
#include <sys/socket.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>

#include "sdk_net_base.h"
#include "dlog.h"
#define NETWAITIME      3000
#define NETWAITNUM      3
#define SOCKETISCLOSE   (-2)

// #define NET_MAX_LEN (8192+1024)
#define NET_MAX_LEN (1024 * 1024)
#define NET_CHECHK_PC_IDEENTIFIER_FIRST_BIT	    '@'
#define NET_CHECHK_PC_IDEENTIFIER_SECOND_BIT	 	'#'
#define NET_CHECHK_PC_IDEENTIFIER_THIRD_BIT 		'$'
#define NET_CHECHK_PC_IDEENTIFIER_FOURTH_BIT		'&'


#define NET_CHECK_STANDARD_FIRST_BIT ('B')
#define NET_CHECK_STANDARD_SECOND_BIT ('L')
#define  NET_MSG_HEAD_VERSION			(2015)

#define IS_FILE 0x1//保留位置第一位代表是文件




typedef struct Net_Server_Handle
{
    int serversocket[2];
    int nConnectNum;//连接的个数
    pthread_mutex_t serverlock;//互斥锁
    InparamServerNet_t inParam;
    pthread_t servertid;
    List_Handle_t pListClient;
    int serverStatus;
}Net_Server_Handle_t;


typedef struct Asynchronous_Info
{
    char *messege;
    int nLen;
    int code;
}Asynchronous_Info_t;
typedef enum
{
    SERVER_STAR = 0,
    SERVER_STOP,
    SERVER_ABOR,
}ServerStaus_t;
static int sdkpthread_cond_timedwait(pthread_cond_t* pCondl,pthread_mutex_t *pLock, int nWaitMillisecond);
int free_asynchronous_sendmessege(Asynchronous_Info_t * asynchronous_sendmessege);
static void * net_clientsend_Thread(void *argv);
static int getclientsocket(Net_Opreate_Hanle_S *pstNetHandle);
static void * net_clientHeartThread(void *argv);

static Net_Status_t getclientstatus(Net_Opreate_Hanle_S *pstNetHandle);
static int setclientsocket(Net_Opreate_Hanle_S *pstNetHandle, int socket);
static int get_head_code(Net_Opreate_Hanle_S *pstNetOperateHandle, void * pHeadBuf);

int sdk_netIpIsValid(const char *ip)
{
    char* ptr;
    int count = 0;
    char str[16] = {0};

    memcpy(str,ip,sizeof(str));
    const char *p = str;

    //1、判断是不是三个 ‘.’
    //2、判断是不是先导0
    //3、判断是不是四部分数
    //4、第一个数不能为0

    while(*p != '\0')
    {
        if(*p == '.')
        count++;
        p++;
    }

    if(count != 3)
    {
        return FALSE;
    }

    count = 0;
    ptr = strtok(str,".");
    while(ptr != NULL)
    {
        count++;
        if(ptr[0] == '0' && isdigit(ptr[1]))
        {
            return FALSE;
        }

        int a = atoi(ptr);
        if(count == 1 && a == 0)
        {
            return FALSE;
        }

        if((a < 0) || (a > 255))
        {
            return FALSE;
        }

        ptr = strtok(NULL,".");
    }

    if(count == 4)
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }

    return TRUE;
}

static int privateprotocol_init_head(Net_Msg_Head_t *pstNetHead)
{
    if (pstNetHead == NULL)
    {
        return -1;
    }

    memset(pstNetHead, 0, sizeof(Net_Msg_Head_t));
    //r_memcpy(head->identifier,Net_Msg_Head_IDENTIFIER,sizeof(head->identifier));
    pstNetHead->identifier[0] = NET_CHECHK_PC_IDEENTIFIER_FIRST_BIT;
    pstNetHead->identifier[1] = NET_CHECHK_PC_IDEENTIFIER_SECOND_BIT;
    pstNetHead->identifier[2] = NET_CHECHK_PC_IDEENTIFIER_THIRD_BIT;
    pstNetHead->identifier[3] = NET_CHECHK_PC_IDEENTIFIER_FOURTH_BIT;

    pstNetHead->version = NET_MSG_HEAD_VERSION;
    return 0;
}
static int standardprotocol_init_head(Net_ComPanyStandard_Head_S *pstStandardNetHead)
{
    if(pstStandardNetHead == NULL)
    {
        return -1;
    }
    pstStandardNetHead->u16CompanyName[0] = NET_CHECK_STANDARD_FIRST_BIT;
    pstStandardNetHead->u16CompanyName[1] = NET_CHECK_STANDARD_SECOND_BIT;
    return 0;
}
static int net_init_head(Net_Opreate_Hanle_S *pstNetOperateHandle, void ** ppHeadBuf ,int nDataLen , int nCode ,
        int* nHeadLen, int nReserve)
{
    if(pstNetOperateHandle->inParam.enProtocolType == SDK_PRIVATE_DEFAULT)
    {
        privateprotocol_init_head(&(pstNetOperateHandle->stProtocolHeadInfo.stPrivateSendMsgHead));
        pstNetOperateHandle->stProtocolHeadInfo.stPrivateSendMsgHead.msg_code = nCode;
        pstNetOperateHandle->stProtocolHeadInfo.stPrivateSendMsgHead.load_len = nDataLen;
        *ppHeadBuf = (&(pstNetOperateHandle->stProtocolHeadInfo.stPrivateSendMsgHead));
        *nHeadLen = sizeof(pstNetOperateHandle->stProtocolHeadInfo.stPrivateSendMsgHead);
        pstNetOperateHandle->stProtocolHeadInfo.stPrivateSendMsgHead.reserve = nReserve;
    }
    else if(pstNetOperateHandle->inParam.enProtocolType == SDK_COMPANY_STANDARD 
            /*|| SDK_COMPANY_VISCA == pstNetOperateHandle->inParam.enProtocolType*/)
    {
        standardprotocol_init_head(&(pstNetOperateHandle->stProtocolHeadInfo.stStandardSendMsgHead));
        pstNetOperateHandle->stProtocolHeadInfo.stStandardSendMsgHead.u16DataLen = nDataLen;
        pstNetOperateHandle->stProtocolHeadInfo.stStandardSendMsgHead.u16ExpansionLen = 0;
        pstNetOperateHandle->stProtocolHeadInfo.stStandardSendMsgHead.u16ProjectCode = pstNetOperateHandle->inParam.u16ProjectCode;
        *ppHeadBuf = (&(pstNetOperateHandle->stProtocolHeadInfo.stStandardSendMsgHead));
        *nHeadLen = sizeof(pstNetOperateHandle->stProtocolHeadInfo.stStandardSendMsgHead);
    }
    return 0;
}
static int get_recvhead_buf(Net_Opreate_Hanle_S *pstNetOperateHandle, void ** ppHeadBuf, int *pHeadLen)
{
    if(pstNetOperateHandle->inParam.enProtocolType == SDK_PRIVATE_DEFAULT)
    {
        *ppHeadBuf = &(pstNetOperateHandle->stProtocolHeadInfo.stPrivateRecvMsgHead);
        *pHeadLen = sizeof(pstNetOperateHandle->stProtocolHeadInfo.stPrivateRecvMsgHead);
    }
    else if(pstNetOperateHandle->inParam.enProtocolType == SDK_COMPANY_STANDARD 
            /*|| SDK_COMPANY_VISCA == pstNetOperateHandle->inParam.enProtocolType*/)
    {
        *ppHeadBuf = &(pstNetOperateHandle->stProtocolHeadInfo.stStandardRecvMsgHead);
        *pHeadLen = sizeof(pstNetOperateHandle->stProtocolHeadInfo.stStandardRecvMsgHead);
    }
    return 0;
}
static int privateprotocol_check_netHead(Net_Msg_Head_t *pstPrivateHead)
{
    if ((pstPrivateHead == NULL) || (pstPrivateHead->identifier[0] != NET_CHECHK_PC_IDEENTIFIER_FIRST_BIT)
        || (pstPrivateHead->identifier[1] != NET_CHECHK_PC_IDEENTIFIER_SECOND_BIT)
        || (pstPrivateHead->identifier[2] != NET_CHECHK_PC_IDEENTIFIER_THIRD_BIT)
        || (pstPrivateHead->identifier[3] != NET_CHECHK_PC_IDEENTIFIER_FOURTH_BIT))
    {
        return -1;
    }

    if (pstPrivateHead->load_len <= 0)
    {
        return -1;
    }

    return 0;
}
static int standardprotocol_check_netHead(Net_Opreate_Hanle_S *pstNetOperateHandle ,
        Net_ComPanyStandard_Head_S *pstStandardNetHead, int* pnDataRcvLen)
{
    if ((pstStandardNetHead == NULL) || (pstStandardNetHead->u16CompanyName[0] != NET_CHECK_STANDARD_FIRST_BIT)
        || (pstStandardNetHead->u16CompanyName[1] != NET_CHECK_STANDARD_SECOND_BIT))

    {
        return -1;
    }
    *pnDataRcvLen = pstStandardNetHead->u16DataLen;
    if(*pnDataRcvLen <= 0)
    {
        return -1;
    }

    return 0;
}
static int net_check_head(Net_Opreate_Hanle_S *pstNetOperateHandle,void * pHeadBuf,int* pnDataRcvLen )
{
    if(pstNetOperateHandle->inParam.enProtocolType == SDK_PRIVATE_DEFAULT)
    {
        *pnDataRcvLen = ((Net_Msg_Head_t*)pHeadBuf)->load_len;
        return privateprotocol_check_netHead(pHeadBuf);
    }
    else if(pstNetOperateHandle->inParam.enProtocolType == SDK_COMPANY_STANDARD 
            /*|| SDK_COMPANY_VISCA == pstNetOperateHandle->inParam.enProtocolType*/)
    {
        return standardprotocol_check_netHead(pstNetOperateHandle, pHeadBuf, pnDataRcvLen);
    }
    return -1;
}
static int  recv_cmd_packet(Net_Opreate_Hanle_S *pstNetOperateHandle, NetCallbackMsg_t* pstNetDealCmd, char * pchRecvData)
{
    if(pstNetOperateHandle->inParam.enProtocolType == SDK_PRIVATE_DEFAULT)
    {
        Net_Msg_Head_t * pHeadBuf = &(pstNetOperateHandle->stProtocolHeadInfo.stPrivateRecvMsgHead);
        pstNetDealCmd->Code = pHeadBuf->msg_code;
        pstNetDealCmd->sOperHandle = pstNetOperateHandle;
        pstNetDealCmd->recvvalue = pchRecvData;
        pstNetDealCmd->nLen = pHeadBuf->load_len;
        pstNetDealCmd->InParam = pstNetOperateHandle->inParam.param;
        pstNetOperateHandle->pRecvMessege = pchRecvData;
        pstNetDealCmd->nReserve = pHeadBuf->reserve;
        pstNetOperateHandle->nReserve = pHeadBuf->reserve;
    }
    else if(pstNetOperateHandle->inParam.enProtocolType == SDK_COMPANY_STANDARD 
            /*|| SDK_COMPANY_VISCA == pstNetOperateHandle->inParam.enProtocolType*/)
    {
        Net_ComPanyStandard_Head_S * pStandardHeadBuf = &(pstNetOperateHandle->stProtocolHeadInfo.stStandardRecvMsgHead);
        pstNetDealCmd->Code = get_head_code(pstNetOperateHandle,pStandardHeadBuf );
        pstNetDealCmd->sOperHandle = pstNetOperateHandle;
        pstNetDealCmd->recvvalue = pchRecvData;
        pstNetDealCmd->nLen = pStandardHeadBuf->u16DataLen;
        pstNetDealCmd->InParam = pstNetOperateHandle->inParam.param;
        pstNetOperateHandle->pRecvMessege = pchRecvData;
        pstNetDealCmd->nReserve = SDK_STANDARD_CODE;
    }
    pstNetDealCmd->enProtocolType = pstNetOperateHandle->inParam.enProtocolType;
    return -1;
}
static int get_head_code(Net_Opreate_Hanle_S *pstNetOperateHandle, void * pHeadBuf)
{
    if(pstNetOperateHandle->inParam.enProtocolType == SDK_PRIVATE_DEFAULT)
    {
        return ((Net_Msg_Head_t*)pHeadBuf)->msg_code;
    }
    return SDK_STANDARD_CODE;
}
int sdk_pthread_create(pthread_t *thread_id,const pthread_attr_t *user_attr, pthread_fun_sdk funtion, void *argv)
{

      int ret ,stacksize = 128000; /*thread 堆栈设置为128K，stacksize以字节为单位。*/
      pthread_attr_t attr;
      ret = pthread_attr_init(&attr); /*初始化线程属性*/
      if (ret != 0)
          return -1;
      /*默认系统分配线程堆栈*/
//      ret = pthread_attr_setstacksize(&attr, stacksize);
      if(ret != 0)
      {
          pthread_attr_destroy(&attr); /*不再使用线程属性，将其销毁*/
          return -1;
      }

      ret = pthread_create (thread_id, &attr, funtion, argv);
      if(ret != 0)
      {
          pthread_attr_destroy(&attr); /*不再使用线程属性，将其销毁*/
          return -1;
      }

      ret = pthread_attr_destroy(&attr); /*不再使用线程属性，将其销毁*/
      if(ret != 0)
      {
          return -1;
      }


    return 0;
}
int getUsermessege(Sdk_Net_Handle_t clienthandle, OfferUserMessege_t * messege)
{
    Net_Opreate_Hanle_S* pclientHandle = (Net_Opreate_Hanle_S*)clienthandle;
    if(messege == NULL || pclientHandle == NULL)
    {
        return -1;
    }
    memcpy(messege, &(pclientHandle->usermessege), sizeof(OfferUserMessege_t));
    return 0;
}

int setConnectClientParam(Sdk_Net_Handle_t clienthandle, void*clientParam)
{
    Net_Opreate_Hanle_S* pclientHandle = (Net_Opreate_Hanle_S*)clienthandle;
    if(pclientHandle == NULL)
    {
        return -1;
    }
    pthread_mutex_lock(&(pclientHandle->lock));
    pclientHandle->serverClientParam = clientParam;
    pthread_mutex_unlock(&(pclientHandle->lock));
    return 0;
}
void* getConnectClientParam(Sdk_Net_Handle_t clienthandle)
{
    Net_Opreate_Hanle_S* pclientHandle = (Net_Opreate_Hanle_S*)clienthandle;
    void *clientParam = NULL;
    if(pclientHandle == NULL)
    {
        return NULL;
    }
    pthread_mutex_lock(&(pclientHandle->lock));
    clientParam = pclientHandle->serverClientParam;
    pthread_mutex_unlock(&(pclientHandle->lock));
    return clientParam;
}


static int setServerStatus(Net_Server_Handle_t *handle, ServerStaus_t status)
{
    if(handle == NULL)
    {
        return -1;
    }
    pthread_mutex_lock(&(handle->serverlock));
    handle->serverStatus = status;
    pthread_mutex_unlock(&(handle->serverlock));
    return 0;
}
static ServerStaus_t getServerStatus(Net_Server_Handle_t *handle)
{
    ServerStaus_t status = SERVER_ABOR;
    if(handle == NULL)
    {
        return status;
    }
    pthread_mutex_lock(&(handle->serverlock));
    status = handle->serverStatus;
    pthread_mutex_unlock(&(handle->serverlock));
    return status;
}
int setclient_currentstatus(Net_Opreate_Hanle_S* pstNetHandle,Net_Status_t status)
{

    pthread_mutex_lock(&(pstNetHandle->lock));
    pstNetHandle->current_status = status;
    pthread_mutex_unlock(&(pstNetHandle->lock));
    return 0;
}

Net_Status_t getclient_currentstatus(Net_Opreate_Hanle_S* pstNetHandle)
{
    Net_Status_t  current_status = SDK_NET_CONNECT;
    pthread_mutex_lock(&(pstNetHandle->lock));
    current_status = pstNetHandle->current_status;
    pthread_mutex_unlock(&(pstNetHandle->lock));
    return current_status;
}
static int addServeConNum(Net_Server_Handle_t *handle)
{
    if(handle == NULL)
    {
        return -1;
    }
    pthread_mutex_lock(&(handle->serverlock));
    handle->nConnectNum++;
    pthread_mutex_unlock(&(handle->serverlock));
    return 0;
}
static int getServeConNum(Net_Server_Handle_t *handle)
{
    int nConnectNum = 0;
    if(handle == NULL)
    {
        return -1;
    }
    pthread_mutex_lock(&(handle->serverlock));
    nConnectNum = handle->nConnectNum;
    pthread_mutex_unlock(&(handle->serverlock));
    return nConnectNum;
}
static int subServeConNum(Net_Server_Handle_t *handle)
{
    if(handle == NULL)
    {
        return -1;
    }
    pthread_mutex_lock(&(handle->serverlock));
    handle->nConnectNum--;
    pthread_mutex_unlock(&(handle->serverlock));
    return 0;
}


int sdknet_TcpSndNonblockFd(int Fd, char *SndBuf, int *SndLen, int Timeout, Net_Opreate_Hanle_S* pstNetHandle)
{

    if(Fd < 0 || NULL == SndBuf || SndLen == NULL || pstNetHandle == NULL) {
        dlog(LOG_ERROR, "<sdknet_TcpSndNonblockFd IS ERROR> <FD : %d> <SndBuf :%p> <SndLen :%p> <Timeout :%d>\n",
              Fd, SndBuf, SndLen, Timeout);
        return RHRETFAIL;
    }

    if(*SndLen < 0 || *SndLen == 0) {
        dlog(LOG_ERROR, "<sdknet_TcpSndNonblockFd IS ERROR> <FD : %d> <SndBuf :%p> <SndLen :%d> <Timeout :%d>\n",
              Fd, SndBuf, *SndLen, Timeout);
    }

    int nCount = 0;
    int RetSelect   = -1;
    int SndTotalLen = 0;
    int SndBytes	= 0;
    int SndTempLen  = *SndLen;
    *SndLen = 0;
    fd_set SndSet;
    struct timeval Time;


    //    Time.tv_sec = Timeout;
    //    Time.tv_usec = 0;

    //int sendlenNum = 0;
    while(SndTotalLen < SndTempLen) {
        FD_ZERO(&SndSet);
        FD_SET(Fd, &SndSet);
        if(Timeout < 0) {
            RetSelect = select(Fd + 1, NULL, &SndSet, NULL, NULL);
        } else {
            Time.tv_sec = Timeout / 1000;
            Time.tv_usec = 1000 * (Timeout % 1000);

            RetSelect = select(Fd + 1, NULL, &SndSet, NULL, &Time);
        }


        if(RetSelect < 0) {
            dlog(LOG_ERROR, "<sdknet_TcpSndNonblockFd IS ERROR> <select> <ERROR_S :%s> <ERROR_D :%d> <FD : %d>\n",
                  strerror(errno), errno, Fd);
            return RH_GetPrivateError();
        } else if(RetSelect == 0) {
            //dlog(LOG_ERROR, "<RH_TcpSndNonblockFd IS ERROR> <Snd timeout> <ERROR_S :%s> <ERROR_D :%d> <FD : %d> <Timeout :%d>\n",
                  ///strerror(errno), errno, Fd, Timeout);
            //return RH_GetPrivateError();
                if(nCount++ > 5 || getclientsocket(pstNetHandle) < 3)
                {
                    dlog(LOG_ERROR, "<sdknet_TcpSndNonblockFd IS ERROR> <Snd timeout> <ERROR_S :%s> <ERROR_D :%d> <FD : %d> <Timeout :%d> nCount:%d\n",
                                      strerror(errno), errno, Fd, Timeout, nCount);
                    return RH_GetPrivateError();
                }
                continue;

        } else {
            if(FD_ISSET(Fd, &SndSet)) {
        /*		if(SndTempLen - *SndLen < SENDMTU)
                {
                    sendlenNum = SndTempLen - *SndLen;
                }
                else
                {
                    sendlenNum = SENDMTU;
                }
                SndBytes = send(Fd , SndBuf + SndTotalLen, sendlenNum, 0);
        */
                SndBytes = send(Fd , SndBuf + SndTotalLen, SndTempLen - SndTotalLen, MSG_NOSIGNAL);
                if(SndBytes <= 0) {
                    //dlog(LOG_ERROR, "<sdknet_TcpSndNonblockFd IS ERROR> <Snd > <sendlen :%d> <ERROR_S :%s> <ERROR_D :%d> <FD : %d>\n",\
                          SndTempLen - SndTotalLen, strerror(errno), errno, Fd);
                    return RH_GetPrivateError();
                } else {
                    SndTotalLen += SndBytes;
                    *SndLen = SndTotalLen;
                }
            } else {
                dlog(LOG_ERROR, "<sdknet_TcpSndNonblockFd IS ERROR> <FD_ISSET> <ERROR_S :%s> <ERROR_D :%d> <FD : %d>\n",
                      strerror(errno), errno, Fd);
                return RH_GetPrivateError();
            }
        }
    }

    return 	RHRETSUCCESS;
}
int net_send_msgdeal(Sdk_Net_Handle_t pOprHandle, char* message, int nLen, int code)
{
    Net_Opreate_Hanle_S* pstNetHandle = (Net_Opreate_Hanle_S*)pOprHandle;
    int ret = RHRETSUCCESS;
    int nSendlen = 0;
    int socket = 0;
    void * pHeadBuf = NULL;

    if (pstNetHandle == NULL || message == NULL || nLen <= 0)
    {
        //dlog(LOG_ERROR, "pstNetHandle is NULL, communication_send_msg failed\n");
        return -1;
    }
    socket = getclientsocket(pstNetHandle);
    if(socket < 3)
    {
        return -1;
    }



    pthread_mutex_lock(&(pstNetHandle->netlock));
    net_init_head(pstNetHandle, &pHeadBuf ,nLen, code , &nSendlen , pstNetHandle->nReserve);
    if(pstNetHandle->inParam.enProtocolType == SDK_COMPANY_STANDARD 
            /*|| SDK_COMPANY_VISCA == pstNetHandle->inParam.enProtocolType*/)
    {
        //printf("aaaaaaaaa");
        char *JsonBuf = (char*)malloc(nSendlen+nLen+1);
        memcpy(JsonBuf,(char *)pHeadBuf,nSendlen);
        memcpy(JsonBuf+nSendlen,message,nLen);
        nSendlen += nLen;
        ret = sdknet_TcpSndNonblockFd(socket, JsonBuf, &nSendlen, pstNetHandle->inParam.overtime, pstNetHandle);
        if (ret != RHRETSUCCESS)
        {
            dlog(LOG_DEBUG, "2 RH_TcpSndNonblockFd********* ret :%d pstNetHandle->inParam.nPort:%d\n", ret, pstNetHandle->inParam.nPort);
            goto EXIT;
        }
        printf("net_send_msgdeal [%d] %s\n",nSendlen,message);
        if(JsonBuf)
        {
            free(JsonBuf);
        }
    }
    else
    {
        ret =  sdknet_TcpSndNonblockFd(socket, (char *)pHeadBuf, &nSendlen, pstNetHandle->inParam.overtime, pstNetHandle);
        if (ret != RHRETSUCCESS)
        {
            //dlog(LOG_DEBUG, "1 RH_TcpSndNonblockFd********* ret :%d pstNetHandle->inParam.nPort:%d\n", ret, pstNetHandle->inParam.nPort);
            goto EXIT;
        }

        nSendlen = nLen;
        ret = sdknet_TcpSndNonblockFd(socket, message, &nSendlen, pstNetHandle->inParam.overtime, pstNetHandle);
        if (ret != RHRETSUCCESS)
        {
            dlog(LOG_DEBUG, "2 RH_TcpSndNonblockFd********* ret :%d pstNetHandle->inParam.nPort:%d\n", ret, pstNetHandle->inParam.nPort);
            goto EXIT;
        }
    }
EXIT:
    pthread_mutex_unlock(&(pstNetHandle->netlock));

    return ret;
}
int net_recv_msg(Sdk_Net_Handle_t pOprHandle, char* message, int nLen, int waitTime)
{
    Net_Opreate_Hanle_S* pstNetHandle = (Net_Opreate_Hanle_S*)pOprHandle;
    int ret = 0;
    if (pstNetHandle == NULL || message == NULL || nLen <= 0)
    {
        //dlog(LOG_ERROR, "pstNetHandle is NULL, communication_send_msg failed\n");
        return -1;
    }
    pthread_mutex_lock(&(pstNetHandle->netlock));
    ret = RH_TcpRcvNonblockFd(pstNetHandle->socket, message, &(nLen), waitTime);
    if (ret != RHRETSUCCESS || nLen == 0)
    {

    }

    pthread_mutex_unlock(&(pstNetHandle->netlock));
    return ret;

}
int net_send_file(Sdk_Net_Handle_t pOprHandle, char* filename,  int code)
{
    void * pHeadBuf = NULL;
    Net_Opreate_Hanle_S* pstNetHandle = (Net_Opreate_Hanle_S*)pOprHandle;
    int ret = RHRETSUCCESS;
    int nSendlen = 0;
    int socket = 0;

    FILE* filefd;
    unsigned long long nLen = 0;
    int nFreadSize = 0;
    char pBuf[1024] =  {0};

    if (pstNetHandle == NULL || filename == NULL )
    {
        //dlog(LOG_ERROR, "pstNetHandle is NULL, communication_send_msg failed\n");
        return -1;
    }
    socket = getclientsocket(pstNetHandle);
    if(socket < 3)
    {
        return -1;
    }


    {

        if ( NULL == (filefd = fopen(filename, "r")))
        {
            dlog(LOG_ERROR,"net_send_file fopen");
            return -1;
        }

        if(-1 == fseek(filefd, 0 ,SEEK_END)) /* 定位到文件末尾 */
        {
            dlog(LOG_ERROR,"net_send_file fseek END");
            fclose(filefd);
            return -1;
        }
        nLen = ftell(filefd); /* 得到文件大小 */
        if(nLen <= 0)
        {
            dlog(LOG_ERROR,"net_send_file fileName is Error");
            return -1;
        }
        if(-1 == fseek(filefd, 0 ,0)) /* 定位到文件末尾 */
        {
            dlog(LOG_ERROR,"net_send_file fseek END");
            fclose(filefd);
            return -1;
        }

    }

    pthread_mutex_lock(&(pstNetHandle->netlock));
    net_init_head(pstNetHandle, &pHeadBuf ,nLen, code , &nSendlen , IS_FILE);
    ret =  sdknet_TcpSndNonblockFd(socket, (char *)pHeadBuf, &nSendlen, pstNetHandle->inParam.overtime, pstNetHandle);
    if (ret != RHRETSUCCESS)
    {
        dlog(LOG_DEBUG, "1 RH_TcpSndNonblockFd********* ret :%d pstNetHandle->inParam.nPort:%d\n", ret, pstNetHandle->inParam.nPort);
        goto EXIT;
    }

    while(1)
    {
        nLen = sizeof(pBuf);
        nFreadSize = fread(pBuf, 1, nLen, filefd);
        nSendlen = nFreadSize;
        ret = sdknet_TcpSndNonblockFd(socket, pBuf, &nSendlen, pstNetHandle->inParam.overtime, pstNetHandle);
        if (ret != RHRETSUCCESS)
        {
            dlog(LOG_DEBUG, "2 RH_TcpSndNonblockFd********* ret :%d pstNetHandle->inParam.nPort:%d\n", ret, pstNetHandle->inParam.nPort);
            break;
        }
        if(nLen != nFreadSize)
        {

            if(feof(filefd))
            {
                dlog(LOG_DEBUG,"the fileis end\n");
                break;
            }
            else
            {
                dlog(LOG_ERROR,"read error\n");
                ret = -1;
                break;
            }
        }
    }

EXIT:
    fclose(filefd);
    pthread_mutex_unlock(&(pstNetHandle->netlock));

    return ret;
}
int net_send_msg(Sdk_Net_Handle_t pOprHandle, char* message, int nLen, int code)
{

    Net_Opreate_Hanle_S* pstNetHandle = (Net_Opreate_Hanle_S*)pOprHandle;
    int ret = RHRETSUCCESS;
    int sendlen = 0;
    int socket = 0;
    if (pstNetHandle == NULL || message == NULL || nLen <= 0)
    {
        dlog(LOG_ERROR, "pstNetHandle is NULL, communication_send_msg failed\n");
        return -1;
    }
    socket = getclientsocket(pstNetHandle);
    if(socket < 3)
    {
        // dlog(LOG_ERROR, "SOCKET:%d", socket);
        return -1;
    }

    Net_Status_t current_status = getclient_currentstatus(pstNetHandle);
    if(current_status != SDK_NET_CONNECT)
    {
        //printf("getclient_currentstatus nPort:%d\n\n" , pstNetHandle->inParam.nPort);
        return -1;
    }
    //异步发送
    if(pstNetHandle->inParam.asynchronous == 1)
    {
        int size = 0;
        int listsize = 0;
        Asynchronous_Info_t * asynchronous_messege = NULL;
        List_LockHandle_t* pHeadHandle = NULL;

        pHeadHandle = pstNetHandle->asynchronous_sendlist;

        listsize = pstNetHandle->inParam.asynchronous_listnum;
        if(listsize <= 0 || listsize > 50)
        {
            listsize = 15;
        }

        if(( size = list_lockAndGet_size(pHeadHandle)) > listsize)
        {
            // dlog(LOG_DEBUG, "net_send_msg listsize is too large: :%d, pleaase check network port:%d\n", size, pstNetHandle->inParam.nPort);
            return -1;
        }

        asynchronous_messege = (Asynchronous_Info_t*)manag_malloc(sizeof(Asynchronous_Info_t));
        if(asynchronous_messege == NULL)
        {
            return -1;
        }
        asynchronous_messege->messege = manag_malloc(nLen);
        if(asynchronous_messege->messege == NULL)
        {
             manag_free(asynchronous_messege);
            return -1;
        }
        memcpy(asynchronous_messege->messege, message, nLen);
        asynchronous_messege->code = code;
        asynchronous_messege->nLen = nLen;

        list_lockAndPush_backSignal(pHeadHandle, asynchronous_messege);
        return 0;
    }
    else
    {
        return net_send_msgdeal( pOprHandle, message,  nLen,  code);
    }

    return ret;
}
List_CurNode_t sdk_list_begin_clientHandle(Sdk_ServerNet_Handle_t pServerHandle ,
        Sdk_Net_Handle_t *pOprHandle)
{
    Net_Server_Handle_t* pNetServerHandle = pServerHandle;
    List_CurNode_t *pFindNode = NULL;
    pFindNode =  list_begin(pNetServerHandle->pListClient);
    if(pFindNode != NULL)
    {
        *pOprHandle = ((DataNode*)pFindNode)->pData;
    }
    return pFindNode;
}
List_CurNode_t sdk_list_next_clientHandle(Sdk_ServerNet_Handle_t pServerHandle ,
        Sdk_Net_Handle_t *pOprHandle, List_CurNode_t listNode)
{
    Net_Server_Handle_t* pNetServerHandle = pServerHandle;
    List_CurNode_t *pFindNode = NULL;
    pFindNode =  list_next(pNetServerHandle->pListClient, listNode);
    if(pFindNode != NULL)
    {
        *pOprHandle = ((DataNode*)pFindNode)->pData;
    }
    return pFindNode;
}
List_CurNode_t sdk_list_end_clientHandle(Sdk_ServerNet_Handle_t pServerHandle)
{
    Net_Server_Handle_t* pNetServerHandle = pServerHandle;
    return list_end( pNetServerHandle->pListClient);
}
pthread_mutex_t* sdk_getlist_serverlock(Sdk_ServerNet_Handle_t pServerHandle)
{
    Net_Server_Handle_t* pNetServerHandle = pServerHandle;
    if(pNetServerHandle == NULL)
    {
        return NULL;
    }
    pthread_mutex_t *serverlock = &(pNetServerHandle->serverlock);
    return serverlock;
}
int netserver_send_allClient(Sdk_ServerNet_Handle_t pServerHandle, char* message, int nLen, int code)
{
    Net_Server_Handle_t* pNetServerHandle = pServerHandle;
    List_Handle_t pListClient = NULL;
    List_CurNode_t pFindNode = NULL;
    DataNode* pNode = NULL;
    int ret = 0;

    if(pNetServerHandle == NULL || message == NULL || nLen <= 0)
    {
        dlog(LOG_ERROR, "message is null ");
        return -1;
    }

    pListClient = pNetServerHandle->pListClient;
    pthread_mutex_lock(&(pNetServerHandle->serverlock));
    for(pFindNode = list_begin(pListClient); pFindNode != list_end(pListClient); pFindNode = list_next(pListClient, pFindNode))
    {
        
        pNode = (DataNode*)pFindNode;
        ret = net_send_msg(pNode->pData, message, nLen, code);
        if(ret != RHRETSUCCESS)
        {
            // dlog(LOG_ERROR, "send message no success ");
            break;
        }

    }
    pthread_mutex_unlock(&(pNetServerHandle->serverlock));

    return ret;

}



static Net_Status_t getclientstatus(Net_Opreate_Hanle_S *pstNetHandle)
{
    Net_Status_t status;
    if (pstNetHandle == NULL)
    {
        return SDK_NET_ERROR;
    }
    pthread_mutex_lock(&(pstNetHandle->lock));
    status = pstNetHandle->status;
    pthread_mutex_unlock(&(pstNetHandle->lock));
    return status;
}
int setclientstatus(Net_Opreate_Hanle_S *pstNetHandle, Net_Status_t status)
{
    if (pstNetHandle == NULL)
    {
        return -1;
    }
    pthread_mutex_lock(&(pstNetHandle->lock));
    pstNetHandle->status = status;
    pthread_mutex_unlock(&(pstNetHandle->lock));
    return 0;
}

static int getclientsocket(Net_Opreate_Hanle_S *pstNetHandle)
{
    int socket = 0;
    if (pstNetHandle == NULL)
    {
        return -1;
    }
    pthread_mutex_lock(&(pstNetHandle->lock));
    socket = pstNetHandle->socket;
    pthread_mutex_unlock(&(pstNetHandle->lock));
    return socket;
}
static int setclientsocket(Net_Opreate_Hanle_S *pstNetHandle, int socket)
{
    if (pstNetHandle == NULL)
    {
        return -1;
    }
    pthread_mutex_lock(&(pstNetHandle->lock));
    pstNetHandle->socket = socket;
    pthread_mutex_unlock(&(pstNetHandle->lock));
    usleep(10000);
    return 0;
}



static int dealCmd(NetCallbackMsg_t* dealmsg)
{

    Net_Opreate_Hanle_S *pstNetHandle = NULL;
    if (dealmsg == NULL || dealmsg->sOperHandle == NULL)
    {
        return-1;
    }
    pstNetHandle = (Net_Opreate_Hanle_S*)dealmsg->sOperHandle;
    InparamClientNet_t* pInParam = &(pstNetHandle->inParam);

    if (pInParam->cmdfun)
    {
        dealmsg->InParam = pInParam->param;
        pInParam->cmdfun(dealmsg);
    }

    return 0;
}

static int destoryclient(Net_Opreate_Hanle_S *pstNetHandle)
{
    Asynchronous_Info_t * asynchronous_sendmessege = NULL;
    if (pstNetHandle)
    {
        if (pstNetHandle->socket > 2)
        {
            RH_Close((char *)__FILE__, (char *)__func__, pstNetHandle->socket);
        }
        pstNetHandle->socket = -1;

        //防止正在发送
        pthread_mutex_lock(&(pstNetHandle->lock));
        pthread_mutex_unlock(&(pstNetHandle->lock));
        pthread_mutex_lock(&(pstNetHandle->netlock));
        pthread_mutex_unlock(&(pstNetHandle->netlock));
        pthread_mutex_destroy(&(pstNetHandle->lock));
        pthread_mutex_destroy(&(pstNetHandle->netlock));
        uninit_heart_signal(pstNetHandle);
        if(pstNetHandle->asynchronous_sendlist)

        {
            //清空链表
            while(1)
            {
                asynchronous_sendmessege  = list_lockAndPop_front(pstNetHandle->asynchronous_sendlist);
                if(asynchronous_sendmessege == NULL)
                {
                    break;
                }
                free_asynchronous_sendmessege(asynchronous_sendmessege);
            }
            list_lockAndDestory(pstNetHandle->asynchronous_sendlist);

        }
         manag_free(pstNetHandle);

        pstNetHandle = NULL;
    }
    return 0;

}

int netclient_callback_statusfun(Net_Status_t status, Net_Opreate_Hanle_S* pstNetHandle, void *inparam)
{
    setclient_currentstatus(pstNetHandle, status);
    pstNetHandle->inParam.statusFun(status, pstNetHandle, inparam);
    return 0;
}
static void* netclient_thread(void * argv)
{
    Net_Opreate_Hanle_S *pstNetHandle = (Net_Opreate_Hanle_S*)argv;
    NetCallbackMsg_t stNetDealCmd;
    InparamClientNet_t* pInParam = NULL;
    char * pRecvLongBuf = NULL;
    char recvshortBuf[NET_MAX_LEN];
    char *recvPoint = NULL;
    int ret = 0;
    int nHeadRecvLen = 0;
    int noRecvTime = 0;
    int hearttime = NETWAITIME + 1000;
    int setstatus = 0;
    int socket = 0;
    int readlen = 0;
    int maxlongBufLen = 0;
    int creatsendthread = 0;
    int isipv6 = 0;
    int creatheartthread = 0;
    int nDataRcvLen = 0;
    //由于网络协议不同，所以接收数据头要用无符号
    void *pHeadRecvBuf = NULL;
#ifdef WIN32
    WORD wVersionRequested;
    WSADATA wsaData;
    wVersionRequested = MAKEWORD(1, 1);
    WSAStartup(wVersionRequested, &wsaData);
#endif
    if (pstNetHandle == NULL)
    {
        goto EXIT;
    }
    if(strlen(pstNetHandle->inParam.ip) > 16)
    {
        isipv6 = 1;
    }
    pInParam = &(pstNetHandle->inParam);
RECONNECT:
    if(getclientstatus(pstNetHandle) != SDK_NET_CONNECT)
    {
        goto EXIT;
    }
    socket = getclientsocket(pstNetHandle);
    if(socket > 2)
    {
        setclientsocket(pstNetHandle, -1);
        shutdown(socket, SHUT_RDWR);
        close(socket);
        sleep(2);
    }
    setclient_currentstatus(pstNetHandle, SDK_NET_DISCONNECT);
    //因为服务器主动断开的才通知，连接不上的不通知
    if(pInParam->nReconnect && setstatus)
    {
        netclient_callback_statusfun(SDK_NET_DISCONNECT, pstNetHandle, pInParam->param);
    }
    if(isipv6 == 1)
    {
        socket = RH_CreateTcpNoBindIPV6Fd();
    }
    else
    {
        socket = RH_CreateTcpNoBindFd();
    }

    if(socket < 3)
    {
        if(pInParam->nReconnect)
        {
            goto RECONNECT;
        }
        goto EXIT;
    }
    if(isipv6 == 1)
    {
        ret = RH_ConnetNonblockIPV6Fd(socket, pInParam->nPort, pInParam->ip, 2000);
    }
    else
    {
        ret = RH_ConnetNonblockFd(socket, pInParam->nPort, pInParam->ip, 2000);
    }

    //printf("pInParam->nPort= %d,pInParam->ip=%s\n",pInParam->nPort,pInParam->ip);
    //exit(0);


    if (ret != RHRETSUCCESS || socket < 3)
    {

        setstatus = 0;
        if(socket > 2)
        {
            setclientsocket(pstNetHandle, -1);
            RH_Close((char *)__FILE__, (char *)__func__, socket);
        }
        if(getclientstatus(pstNetHandle) != SDK_NET_CONNECT)
        {
            goto EXIT;
        }
        sleep(2);
        pInParam->logFun("connect is fail ret:%d, socket:%d,\n", ret, socket);

        if(pInParam->nReconnect)
        {
            goto RECONNECT;
        }
        goto EXIT;
    }
    setclientsocket(pstNetHandle, socket);
    setstatus = 1;
    RH_SetSndTimeoutFd(socket , hearttime/1000, hearttime%1000);
    RH_SetRcvTimeoutFd(socket , hearttime/1000, hearttime%1000);
    netclient_callback_statusfun(SDK_NET_CONNECT, pstNetHandle, pInParam->param);
      struct sockaddr_in flocal;
      int faddr_len = sizeof( struct sockaddr_in);

      int errret = 0;
      memset(&flocal , 0 ,sizeof(struct sockaddr_in));
      errret = getsockname(socket,(struct sockaddr*)&flocal, (socklen_t *)&faddr_len);
      //printf("ip:%s port:%d pstNetHandle->socket:%d ret:%d\n", inet_ntoa(flocal.sin_addr), ntohs(flocal.sin_port),socket, errret);
      printf("ip:%s port:%d pstNetHandle->socket:%d ret:%d\n", pInParam->ip, pInParam->nPort,socket, errret);


          if(creatheartthread == 0)
          {
                ret = sdk_pthread_create(&(pstNetHandle->heart_tid), NULL, net_clientHeartThread, (void *)(pstNetHandle));
                if(ret != 0)
                {
                    pInParam->logFun("sdk_pthread_create net_clientHeartThread is fail errno:%s  ret:%d\n", strerror(errno), ret);
                    goto EXIT;
                }
            creatheartthread = 1;
          }

        if(pstNetHandle->inParam.asynchronous == 1 && pstNetHandle->asynchronous_sendlist == NULL)
        {
            pstNetHandle->asynchronous_sendlist =  list_lockAndCreate();
            ret = sdk_pthread_create(&(pstNetHandle->send_tid), NULL, net_clientsend_Thread, (void *)(pstNetHandle));
            if(ret != 0)
            {
                pInParam->logFun(" netclient_thread sdk_pthread_create send_clientsetreamThread is fail\n");
                goto EXIT;
            }
            creatsendthread = 1;
        }
        get_recvhead_buf(pstNetHandle, &pHeadRecvBuf, &nHeadRecvLen);

    while (getclientstatus(pstNetHandle) == SDK_NET_CONNECT)
    {
        memset(pHeadRecvBuf, 0, nHeadRecvLen);
        ret = RH_TcpRcvBlockFd(socket, (char *)pHeadRecvBuf, nHeadRecvLen, &readlen);
        if (ret != RHRETSUCCESS || readlen != nHeadRecvLen)
        {
            dlog(LOG_TRACE, "readlen:%d ret:%d recvLen:%d pInParam->nPort:%d\n", readlen, ret, nHeadRecvLen ,pInParam->nPort);

            noRecvTime++;
            if (-ret != EAGAIN) //if (-ret == ECONNRESET || -ret == ECONNREFUSED)//连接被对方复位
            {
                pInParam->logFun("server is disconnect ret:%d\n", ret);
                if(pInParam->nReconnect)
                {
                    //sleep(2);
                    goto RECONNECT;
                }
                goto EXIT;
            }
            if (1 != pInParam->nSetStatus)
            {
                if (noRecvTime >= NETWAITNUM)
                {
                    pInParam->logFun("NETNoRecvTime>NETWAITNUM Heart Error buf is failed,the port=[%d]\n", ret);
                    //socket 异常，断链退出。
                    if(pInParam->nReconnect)
                    {
                        //sleep(2);
                        goto RECONNECT;
                    }
                    goto EXIT;
                }
            }
            else
            {
                RH_SetRcvTimeoutFd(socket , hearttime/1000, hearttime%1000);
            }
            continue;
        }

        noRecvTime = 0;

        if (net_check_head(pstNetHandle, pHeadRecvBuf, &nDataRcvLen) != 0)
        {
            pInParam->logFun("%s %s %d,communtication_check_head is failed,the port =.\n", WITH_ALL);
            if(pInParam->nReconnect)
            {
                //sleep(2);
                goto RECONNECT;
            }
            goto EXIT;
        }

        //memset(recvshortBuf, 0, sizeof(recvshortBuf));
        if (nDataRcvLen >= NET_MAX_LEN)
        {
            if(nDataRcvLen > maxlongBufLen)
            {
                if(pRecvLongBuf)
                {
                     manag_free(pRecvLongBuf);
                    pRecvLongBuf = NULL;
                }

                pRecvLongBuf = (char *)manag_malloc(nDataRcvLen);
                if (pRecvLongBuf == NULL)
                {
                    pInParam->logFun("%s %s %d,malloc  long recvbuf is fail =.\n", WITH_ALL);
                    if(pInParam->nReconnect)
                    {
                        //sleep(2);
                        goto RECONNECT;
                    }
                    goto EXIT;
                }
                maxlongBufLen = nDataRcvLen;
            }

            recvPoint = pRecvLongBuf;
        }
        else
        {
            recvPoint = recvshortBuf;
            recvshortBuf[nDataRcvLen] = '\0';
        }

        ret = RH_TcpRcvBlockFd(socket, recvPoint, nDataRcvLen, &readlen);
        if (ret != RHRETSUCCESS || readlen != nDataRcvLen)
        {
            pInParam->logFun("RH_TcpRcvNonblockFd is failed,the msg_code is [%d],\n", get_head_code(pstNetHandle, pHeadRecvBuf));
            if(pInParam->nReconnect)
            {
                //sleep(2);
                goto RECONNECT;
            }
            goto EXIT;
        }
         recv_cmd_packet( pstNetHandle, &stNetDealCmd, recvPoint);


        if (dealCmd(&stNetDealCmd) != 0)
        {
            pInParam->logFun("dealCmd is failed,the msg_code is [%d],\n", get_head_code(pstNetHandle, pHeadRecvBuf));
            if(pInParam->nReconnect)
            {
                //sleep(2);
                goto RECONNECT;
            }
            goto EXIT;
        }
    }
EXIT:
    if (pRecvLongBuf)
    {
         manag_free(pRecvLongBuf);
        pRecvLongBuf = NULL;
    }
    if (pstNetHandle)
    {

        socket = getclientsocket(pstNetHandle);
        if(socket > 2)
        {
            RH_Close((char *)__FILE__, (char *)__func__, socket);
            setclientsocket(pstNetHandle, -1);
        }
        setclientstatus(pstNetHandle, SDK_NET_DISCONNECT);
        if(pstNetHandle->inParam.asynchronous == 1 && creatsendthread == 1)
        {
            dlog(LOG_DEBUG, "pthread_join  pstNetHandle->send_tid start\n");
            list_cond_signal(pstNetHandle->asynchronous_sendlist);
            pthread_join(pstNetHandle->send_tid, NULL);
            dlog(LOG_DEBUG, "pthread_join  pstNetHandle->send_tid end\n");
        }
        netclient_callback_statusfun(SDK_NET_DISCONNECT, pstNetHandle, pInParam->param);
    }
    //pthread_detach(pthread_self());
    return NULL;
}
Sdk_Net_Handle_t sdkclient_init_net(InparamClientNet_t netparm)
{
    Net_Opreate_Hanle_S* pstNetHandle = NULL;
    if (netparm.nPort <= 0 || netparm.logFun == NULL || netparm.overtime <= 0 || netparm.statusFun == NULL
        || netparm.cmdfun == NULL || (strcmp(netparm.ip, "") == 0) )
    {
        return NULL;
    }
    if(!sdk_netIpIsValid(netparm.ip))
    {
        return NULL;
    }
    pstNetHandle = (Net_Opreate_Hanle_S*)manag_malloc(sizeof(Net_Opreate_Hanle_S));
    if (pstNetHandle == NULL)
    {
        netparm.logFun("malloc Net_Opreate_Hanle_S is fail\n");
    }
    memset(pstNetHandle, 0, sizeof(Net_Opreate_Hanle_S));
    memcpy(&(pstNetHandle->inParam), &netparm, sizeof(InparamClientNet_t));

    pthread_mutex_init(&(pstNetHandle->lock), NULL);
    pthread_mutex_init(&(pstNetHandle->netlock), NULL);
    init_heart_signal(pstNetHandle);
    setclientstatus(pstNetHandle, SDK_NET_CONNECT);
    int ret = sdk_pthread_create(&(pstNetHandle->client_tid), NULL, netclient_thread, (void *)(pstNetHandle));

    if (ret != 0)
    {
        netparm.logFun("crate NetCreateclint thread failed\n");
         manag_free(pstNetHandle);
        return NULL;
    }

    return pstNetHandle;
}
RetErr_t sdknet_add_value(tar_messege_t messege,const char *keyname, Value_Type_t valuetype, void *value)

{
    RetErr_t ret = RET_SUCCESS;
    Net_Opreate_Hanle_S* pstNetHandle = (Net_Opreate_Hanle_S*)messege.handle;
    if (pstNetHandle == NULL || messege.valueHandle == NULL || keyname == NULL || value == NULL)
    {
        return RET_PARAMER_ERR;
    }

    if (*(messege.valueHandle) == NULL)
    {
        *(messege.valueHandle) = (void*)xml_init_doc();
    }
    if (valuetype == SDK_INT)
    {
        if (FALSE == xml_set_intNode1(keyname, *((int*)(value)), *(messege.valueHandle)))
        {
            pstNetHandle->inParam.logFun("sdk_add_value xml set is fail\n");
            ret = RET_PARAMER_ERR;
        }
    }
    else if (valuetype == SDK_UINT)
    {
        if (FALSE == xml_set_uintNode1(keyname, *((unsigned int*)(value)), *(messege.valueHandle)))
        {
            pstNetHandle->inParam.logFun("sdk_add_value xml set is fail\n");
            ret = RET_PARAMER_ERR;
        }
    }
    else if (valuetype == SDK_SHORT)
    {
        if (FALSE == xml_set_shortNode1(keyname, *((short*)(value)), *(messege.valueHandle)))
        {
            pstNetHandle->inParam.logFun("xml_set_shortNode1 xml set is fail\n");
            ret = RET_PARAMER_ERR;
        }
    }
    else if (valuetype == SDK_CHAR)
    {
        if (FALSE == xml_set_charNode1(keyname, ((char*)(value)), *(messege.valueHandle)))
        {
            pstNetHandle->inParam.logFun("sdk_add_value  xml_set_charNode1 fail\n");
            ret = RET_PARAMER_ERR;
        }
    }
    else if (valuetype == SDK_FLOAT)
    {
        if (FALSE == xml_set_floatNode1(keyname, *((float*)(value)), *(messege.valueHandle)))
        {
            pstNetHandle->inParam.logFun("sdk_add_value xml xml_set_floatNode1 is fail\n");
            ret = RET_PARAMER_ERR;
        }
    }
    else if (valuetype == SDK_DOUBLE)
    {
        if (FALSE == xml_set_doubleNode1(keyname, *((double*)(value)), *(messege.valueHandle)))
        {
            pstNetHandle->inParam.logFun("sdk_add_value xml xml_set_doubleNode1 is fail\n");
            ret = RET_PARAMER_ERR;
        }
    }
    else if (valuetype == SDK_FILE)
    {
        if (FALSE == xml_set_charNode1((char *)"/root/MsgBody/fileName/", ((char*)(value)), *(messege.valueHandle)))
        {
            pstNetHandle->inParam.logFun("sdk_add_value  xml_set_charNode1 fail\n");
            ret = RET_PARAMER_ERR;
        }
    }
    else
    {
        pstNetHandle->inParam.logFun("no support\n");
        ret = RET_NOT_EXIST;
    }

    return ret;
}
RetErr_t sdknet_send_value(Sdk_Net_Handle_t handle, void ** valueHandle, int code)
{
    RetErr_t ret = RET_SUCCESS;
    Net_Opreate_Hanle_S *pOprHandle = (Net_Opreate_Hanle_S*)handle;
    char * outBuf = NULL;
    if (pOprHandle == NULL || valueHandle == NULL || *valueHandle == NULL)
    {
        return RET_PARAMER_ERR;
    }
    xml_handleTo_str(*valueHandle, "UTF-8", &outBuf);
    if (outBuf == NULL)
    {
        ret = RET_PARSE_ERR;
        goto SENDEXIT;
    }
    if (0 != net_send_msg(pOprHandle, outBuf, strlen(outBuf)+1, code))
    {
        ret = RET_SOCKET_FALL;
    }

SENDEXIT:
    if (*valueHandle)
    {
        xml_free_docHandle(*valueHandle);
        *valueHandle = NULL;
    }
    if (outBuf)
    {
        xml_free_baseBuf(outBuf);
        outBuf = NULL;
    }
    return ret;
}
RetErr_t sdknet_get_value(tar_messege_t messege,const char *keyname, Value_Type_t valuetype, void *value, int nLen)
{
    RetErr_t ret = RET_SUCCESS;
    Net_Opreate_Hanle_S* pstNetHandle = (Net_Opreate_Hanle_S*)messege.handle;
    if (pstNetHandle == NULL || keyname == NULL || value == NULL || pstNetHandle->pRecvMessege == NULL || messege.nodeindex < 0)
    {
        return RET_PARAMER_ERR;
    }

    if (valuetype == SDK_INT)
    {
        if (FALSE == xml_get_intNode3(keyname, ((int*)(value)), pstNetHandle->pRecvMessege, messege.nodeindex))
        {
            pstNetHandle->inParam.logFun("sdk_get_value xml get is fail\n");
            ret = RET_PARAMER_ERR;
        }
    }
    else if (valuetype == SDK_SHORT)
    {
        if (FALSE == xml_get_shortNode3(keyname, ((short*)(value)), pstNetHandle->pRecvMessege, messege.nodeindex))
        {
            pstNetHandle->inParam.logFun("sdk_get_value xml get is fail\n");
            ret = RET_PARAMER_ERR;
        }
    }
    else if (valuetype == SDK_CHAR)
    {
        if (FALSE == xml_get_charNode3(keyname, ((char*)(value)), pstNetHandle->pRecvMessege, nLen, messege.nodeindex))
        {
            pstNetHandle->inParam.logFun("sdk_get_value  xml_get_charNode1 fail\n");
            ret = RET_PARAMER_ERR;
        }
    }
    else if (valuetype == SDK_FLOAT)
    {
        if (FALSE == xml_get_floatNode3(keyname, ((float*)(value)), pstNetHandle->pRecvMessege, messege.nodeindex))
        {
            pstNetHandle->inParam.logFun("sdk_get_value xml xml_get_floatNode1 is fail\n");
            ret = RET_PARAMER_ERR;
        }
    }
    else if (valuetype == SDK_DOUBLE)
    {
        if (FALSE == xml_get_doubleNode3(keyname, ((double*)(value)), pstNetHandle->pRecvMessege, messege.nodeindex))
        {
            pstNetHandle->inParam.logFun("sdk_get_value xml xml_get_doubleNode1 is fail\n");
            ret = RET_PARAMER_ERR;
        }
    }
    else
    {
        pstNetHandle->inParam.logFun("no support\n");
        ret = RET_NOT_EXIST;
    }

    return ret;
}
RetErr_t sdkclient_stop_net(Sdk_Net_Handle_t handle)
{
    Net_Opreate_Hanle_S *pOprHandle = (Net_Opreate_Hanle_S*)handle;
    RetErr_t ret = RET_SUCCESS;
    if (pOprHandle == NULL)
    {
        return RET_PARAMER_ERR;
    }
    setclientstatus(pOprHandle, SDK_NET_DISCONNECT);
#ifdef WIN32
    WSACleanup();
#endif
    return ret;
}
RetErr_t sdkclient_uninit_net(Sdk_Net_Handle_t handle)
{
    Net_Opreate_Hanle_S *pOprHandle = (Net_Opreate_Hanle_S*)handle;
    RetErr_t ret = RET_SUCCESS;
    int socket = 0;
    if (pOprHandle == NULL)
    {
        return RET_PARAMER_ERR;
    }
    pOprHandle->inParam.nReconnect = 0;

    setclientstatus(pOprHandle, SDK_NET_DISCONNECT);
    sdkpthread_condl_signal(&pOprHandle->condHeart,&pOprHandle->lockHeart);
    socket = getclientsocket(pOprHandle);
    if(socket > 2)
    {
        setclientsocket(pOprHandle, -1);
        shutdown(socket, SHUT_RDWR);
        close(socket);
        printf("socket************ close:%d\n", socket);
    }
    if(pOprHandle->client_tid != 0)
    {
        pthread_join(pOprHandle->client_tid, NULL);
    }
    if(pOprHandle->heart_tid != 0)
    {
        pthread_join(pOprHandle->heart_tid, NULL);
    }

    destoryclient(pOprHandle);
#ifdef WIN32
    WSACleanup();
#endif
    return ret;
}





static void * net_clientHeartThread(void *argv)
{
    Net_Opreate_Hanle_S* pOprHandle = (Net_Opreate_Hanle_S*)argv;
    InparamClientNet_t  *inParam = NULL;
    int inputLen = 2048;
    char *big_data = malloc(2048);
    int nSleepTime = NETWAITIME;
    int sendLen = 0;
    char * sendHeartStr = "live";
    if(nSleepTime == 0)
    {
        nSleepTime = 2000;
    }
    if(pOprHandle == NULL)
    {
        return NULL;
    }

    inParam = &(pOprHandle->inParam);
    memset(big_data, 0, inputLen);
    while(getclientstatus(pOprHandle) == SDK_NET_CONNECT)
    {
        //发送心跳流程
        if(inParam->heartmsg != NULL)
        {
            inParam->heartmsg(big_data, inputLen, pOprHandle, inParam->param, &sendLen);
        }
        if(sendLen <= 0 || sendLen > inputLen)
        {
            if(sendLen > inputLen)
            {
                sendLen = inputLen;
                if(big_data != NULL)
                {
                    free(big_data);
                    big_data = NULL;
                }
                inputLen = sendLen + 1;
                big_data = (char*)malloc(inputLen);
                memset(big_data, 0 ,inputLen);
                continue;
            }
            else
            {
                sendLen = strlen(sendHeartStr) + 1;
                memcpy(big_data, sendHeartStr, sendLen);
            }
        }
#if 1
        if (0 != net_send_msgdeal(pOprHandle, big_data, sendLen, SDK_NET_HEARTBIT_CMD))
        {
            sdkpthread_cond_timedwait(&pOprHandle->condHeart, &pOprHandle->lockHeart, 1000);
            continue;
        }
#endif
        sdkpthread_cond_timedwait(&pOprHandle->condHeart, &pOprHandle->lockHeart, nSleepTime);
    }
    if(big_data)
    {
        free(big_data);
    }
    //pthread_detach(pthread_self());
    pthread_exit(0);
    return NULL;
}
int free_asynchronous_sendmessege(Asynchronous_Info_t * asynchronous_sendmessege)
{
    if(asynchronous_sendmessege)
    {
        if(asynchronous_sendmessege->messege)
        {
             manag_free(asynchronous_sendmessege->messege);
        }
         manag_free(asynchronous_sendmessege);
    }
    return 0;
}
int sdk_netis_h264(unsigned char *pos, int nLen)
{
    //4个头自己再加上NAUL 8个字节
    if(nLen <= 12)
    {
        return 0;
    }

    if(!((*pos == 0 && *(pos + 1) == 0 && *(pos + 2) == 0 && *(pos + 3) == 1))) {
        return 0;    //not h264
    } else {
        return 1;    //is h264
    }
}
int sdk_netis_iframe(unsigned char *pos)
{
    if((pos[4] & 0xf) != 0x1)
    {
        return 1;
    }
    return 0;
}
void * net_clientsend_Thread(void *argv)
{
    Net_Opreate_Hanle_S* pOprHandle = (Net_Opreate_Hanle_S*)argv;
    int time = 0;
    Asynchronous_Info_t * asynchronous_sendmessege = NULL;
    List_LockHandle_t* pHeadHandle = NULL;
    int h264sendfail = 0;
    if(pOprHandle == NULL)
    {
        return NULL;
    }
    pHeadHandle = pOprHandle->asynchronous_sendlist;
    time = pOprHandle->inParam.asynchronous_time;
    if(time < 20)
    {
        time = 20;
    }
    if(time > 2000)
    {
        time = 2000;
    }
    // dlog(LOG_DEBUG, "net_clientsend_Thread asynchronous_sendmessege is sucessful port:%d\n", pOprHandle->inParam.nPort);
    while(getclientstatus(pOprHandle) == SDK_NET_CONNECT)
    {
        asynchronous_sendmessege  = list_lockAndPop_frontSignal(pHeadHandle, time);
        if(asynchronous_sendmessege)
        {
            //发送失败做处理
            if(h264sendfail == 1)
            {
                if(sdk_netis_h264((unsigned char *)asynchronous_sendmessege->messege,asynchronous_sendmessege->nLen) == 1)
                {
                    //将i帧发送，紧接着可以发送p帧
                    if(sdk_netis_iframe((unsigned char *)asynchronous_sendmessege->messege) == 1)
                    {
                        h264sendfail = 0;
                    }
                    else
                    {
                        free_asynchronous_sendmessege(asynchronous_sendmessege);
                        continue;
                    }
                }
            }
            if (0 != net_send_msgdeal(pOprHandle, asynchronous_sendmessege->messege, asynchronous_sendmessege->nLen,
                    asynchronous_sendmessege->code))
            {
                //h264发送失败得抛帧处理
                if(sdk_netis_h264((unsigned char *)asynchronous_sendmessege->messege, asynchronous_sendmessege->nLen) == 1)
                {
                    h264sendfail = 1;
                    //dlog(LOG_ERROR, "net_send_msg is fail port:%d", pOprHandle->inParam.nPort);
                }

            }
        }
        free_asynchronous_sendmessege(asynchronous_sendmessege);
    }

    return 0;
}
int init_heart_signal(Net_Opreate_Hanle_S *pstNetHandle )
{
    pthread_mutex_init(&(pstNetHandle->lockHeart), NULL);
    pthread_cond_init(&(pstNetHandle->condHeart),NULL);
    return 0;
}
int uninit_heart_signal(Net_Opreate_Hanle_S *pstNetHandle )
{
    pthread_mutex_destroy(&(pstNetHandle->lockHeart));
    pthread_cond_destroy(&(pstNetHandle->condHeart));
    return 0;
}
int sdkpthread_condl_signal(pthread_cond_t* pCondl,pthread_mutex_t *pLock)
{
    pthread_mutex_lock(pLock);
    pthread_cond_signal(pCondl);
    pthread_mutex_unlock(pLock);
    return 0;
}
static int sdkpthread_cond_timedwait(pthread_cond_t* pCondl,pthread_mutex_t *pLock, int nWaitMillisecond)
{
    struct timespec stAbstime = {2, 0};
    struct timeval stNowTime;
    int nRet = 0;
    gettimeofday(&stNowTime, NULL);
    stAbstime.tv_sec = stNowTime.tv_sec + nWaitMillisecond / 1000;
    stAbstime.tv_nsec = (stNowTime.tv_usec * 1000) + (nWaitMillisecond % 1000) * 1000 * 1000;
    if(stAbstime.tv_nsec >= 1000000000)
    {
        stAbstime.tv_nsec -= 1000000000;
        stAbstime.tv_sec+=1;
    }
    pthread_mutex_lock(pLock);
    nRet =  pthread_cond_timedwait(pCondl, pLock, &stAbstime);
    pthread_mutex_unlock(pLock);
    return nRet;

}

static void * server_process_clientThread(void * argc)
{

        pthread_detach(pthread_self());
        static int numpth = 0;
        Net_Opreate_Hanle_S *pstNetHandle = (Net_Opreate_Hanle_S*)argc;

        NetCallbackMsg_t stNetDealCmd;
        InparamClientNet_t* pInParam = NULL;
        int creatheart = 0;
        int creatsend = 0;
        char * pRecvLongBuf = NULL;
        char recvshortBuf[NET_MAX_LEN];
        char *recvPoint = NULL;
        int ret = 0;
        int nDataRecvLen = 0;
        int nRealRecvLen = 0;
        int noRecvTime = 0;
        int hearttime = NETWAITIME;
        Net_Server_Handle_t* serverHandle = NULL;
        int maxLongBufLen = 0;
        int nDataRcvLen = 0;
        //由于网络协议不同，所以接收数据头要用无符号
        void *pHeadRecvBuf = NULL;
        int nHeadRecvLen = 0;
    #ifdef WIN32
        WORD wVersionRequested;
        WSADATA wsaData;
        wVersionRequested = MAKEWORD(1, 1);
        WSAStartup(wVersionRequested, &wsaData);
    #endif
        if (pstNetHandle == NULL)
        {
            goto EXIT;
        }
        serverHandle = (Net_Server_Handle_t*)pstNetHandle->serverHandle;



        pInParam = &(pstNetHandle->inParam);
        pInParam->logFun("clientip:%s is connect now connectnum:%d\n", pstNetHandle->usermessege.ip, getServeConNum(pstNetHandle->serverHandle));

        pthread_mutex_init(&(pstNetHandle->lock), NULL);
        pthread_mutex_init(&(pstNetHandle->netlock), NULL);
        init_heart_signal(pstNetHandle);
        setclientstatus(pstNetHandle, SDK_NET_CONNECT);


        RH_SetSndTimeoutFd(pstNetHandle->socket , hearttime/1000, hearttime%1000);
        RH_SetRcvTimeoutFd(pstNetHandle->socket , hearttime/1000, hearttime%1000);

        netclient_callback_statusfun(SDK_NET_CONNECT, pstNetHandle, pInParam->param);
        numpth++;

        ret = sdk_pthread_create(&(pstNetHandle->heart_tid), NULL, net_clientHeartThread, (void *)(pstNetHandle));
        if(ret != 0)
        {
            pInParam->logFun("sdk_pthread_create net_clientHeartThread is fail errno:%s numpth:%d ret:%d\n", strerror(errno), numpth, ret);
            goto EXIT;
        }
        creatheart = 1;
        pstNetHandle->inParam.asynchronous = serverHandle->inParam.asynchronous;
        pstNetHandle->inParam.asynchronous_listnum = serverHandle->inParam.asynchronous_listnum;
        pstNetHandle->inParam.nPort = serverHandle->inParam.nPort;
        pstNetHandle->inParam.asynchronous_time = serverHandle->inParam.asynchronous_time;
        if(serverHandle->inParam.asynchronous == 1)
        {
            pstNetHandle->asynchronous_sendlist =  list_lockAndCreate();
            ret = sdk_pthread_create(&(pstNetHandle->send_tid), NULL, net_clientsend_Thread, (void *)(pstNetHandle));
            if(ret != 0)
            {
                pInParam->logFun("sdk_pthread_create send_clientsetreamThread is fail\n");
                goto EXIT;
            }
            usleep(200000);
            creatsend = 1;
        }


        pthread_mutex_lock(&(serverHandle->serverlock));
        list_push_back(serverHandle->pListClient, pstNetHandle);
        pthread_mutex_unlock(&(serverHandle->serverlock));

        get_recvhead_buf(pstNetHandle, &pHeadRecvBuf, &nHeadRecvLen);
        while (getclientstatus(pstNetHandle) == SDK_NET_CONNECT)
        {

            memset(pHeadRecvBuf, 0, nHeadRecvLen);
            nDataRecvLen = nHeadRecvLen;
            ret = RH_TcpRcvNonblockFd(pstNetHandle->socket, pHeadRecvBuf, &nDataRecvLen, hearttime);
            if(SOCKETISCLOSE == ret)
            {
                pInParam->logFun("client is disconnect ret:%d\n", pstNetHandle->socket);
                goto EXIT;
            }
            
            if (ret != RHRETSUCCESS || nDataRecvLen <= 0)
            {
                /*if(SDK_COMPANY_VISCA != pInParam->enProtocolType)*/ noRecvTime++;

                if (-ret == ECONNRESET || -ret == ECONNREFUSED)//连接被对方复位
                {
                    pInParam->logFun("server is disconnect ret:%d\n", ret);
                    goto EXIT;
                }
                if (noRecvTime >= NETWAITNUM)
                {
                    pInParam->logFun("NETNoRecvTime>NETWAITNUM Heart Error buf is failed,the port=%d ret=[%d]\n",serverHandle->inParam.nPort, ret);
                    //socket 异常，断链退出。
                    goto EXIT;
                }
                continue;
            }

            noRecvTime = 0;

            if (net_check_head( pstNetHandle,pHeadRecvBuf,&nDataRecvLen ) != 0)
            {
                pInParam->logFun("%s %s %d,communtication_check_head is failed,the port =.\n", WITH_ALL);
                goto EXIT;
            }

            //memset(recvshortBuf, 0, sizeof(recvshortBuf));
            if (nDataRecvLen >= NET_MAX_LEN)
            {

                if(nDataRecvLen >= maxLongBufLen)
                {
                    if (pRecvLongBuf)
                    {
                         manag_free(pRecvLongBuf);
                        pRecvLongBuf = NULL;
                    }
                    pRecvLongBuf = (char *)manag_malloc(nDataRecvLen);
                    if (pRecvLongBuf == NULL)
                    {
                        pInParam->logFun("%s %s %d,malloc  long recvbuf is fail =.\n", WITH_ALL);
                        goto EXIT;
                    }
                    maxLongBufLen = nDataRecvLen;
                }

                recvPoint = pRecvLongBuf;
            }
            else
            {
                recvPoint = recvshortBuf;
                recvshortBuf[nDataRecvLen] = '\0';
            }
            nRealRecvLen = nDataRecvLen;
            ret = RH_TcpRcvNonblockFd(pstNetHandle->socket, recvPoint, &(nRealRecvLen), hearttime);
            if (ret != RHRETSUCCESS || nDataRecvLen == 0)
            {
                pInParam->logFun("RH_TcpRcvNonblockFd is failed,the msg_code is [%d], headbuf.load_len:%d"
                        "ret:%d RealRecvLen:%d\n", get_head_code(pstNetHandle, pHeadRecvBuf), nDataRecvLen, ret, nRealRecvLen);
                goto EXIT;
            }

             recv_cmd_packet( pstNetHandle, &stNetDealCmd, recvPoint);
            if (dealCmd(&stNetDealCmd) != 0)
            {
                pInParam->logFun("dealCmd is failed,the msg_code is [%d],\n", get_head_code(pstNetHandle, pHeadRecvBuf));
                goto EXIT;
            }


        }
    EXIT:
        if (pstNetHandle)
        {
            if (pRecvLongBuf)
            {
                 manag_free(pRecvLongBuf);
                pRecvLongBuf = NULL;
            }
            if(serverHandle)
            {
                pthread_mutex_lock(&(serverHandle->serverlock));
                list_earse_data(serverHandle->pListClient, pstNetHandle);
                pthread_mutex_unlock(&(serverHandle->serverlock));
            }
            setclientstatus(pstNetHandle, SDK_NET_DISCONNECT);
            netclient_callback_statusfun(SDK_NET_DISCONNECT, pstNetHandle, pstNetHandle->inParam.param);

            subServeConNum(pstNetHandle->serverHandle);

            if(creatheart == 1)
            {
                pthread_join(pstNetHandle->heart_tid, NULL);
            }

            if(serverHandle->inParam.asynchronous == 1 && creatsend == 1)
            {
                pthread_join(pstNetHandle->send_tid, NULL);
            }
            int closesocket = 0;
            if((closesocket = getclientsocket(pstNetHandle)) > 2)
            {
                // dlog(LOG_DEBUG, "CLIENT CONNECT");
                setclientsocket(pstNetHandle, -1);
                shutdown(closesocket, SHUT_RDWR);
                RH_Close((char *)__FILE__, (char *)__func__, closesocket);
            }
            destoryclient(pstNetHandle);
        }
        numpth--;

    return NULL;
}
//默认0是IPV4
int select_socket_accept(int* serversocket, int socketnum, int Timeout, char *AcceptIp, int ipLen)
{
    int i = 0;
    int max_fid = 0;
    struct timeval TimeoutVal;
    fd_set read_fds;
    int s32Ret = 0;
    int client_socket = 0;
    struct sockaddr_in ClnAddr;
    struct sockaddr_in6 ClnIPV6Addr;
    int Len = sizeof(struct sockaddr_in);
    for(i = 0; i < socketnum; i++)
    {
        if(serversocket[i] > max_fid)
        {
            max_fid = serversocket[i];
        }
    }
    
    //printf("nLen:%d %d %d\n", sizeof(struct sockaddr), sizeof(struct sockaddr_in), sizeof(struct sockaddr_in6));
    TimeoutVal.tv_sec  = Timeout;
    TimeoutVal.tv_usec = 0;
    FD_ZERO(&read_fds);
    for(i = 0; i < socketnum; i++)
    {
        FD_SET(serversocket[i],&read_fds);
    }
    s32Ret = select(max_fid+1, &read_fds, NULL, NULL, &TimeoutVal);
    if (s32Ret > 0)
    {

        for(i = 0; i < socketnum; i++)
        {
            if (FD_ISSET(serversocket[i], &read_fds))
            {
                if(i == 0)
                {
                    Len = sizeof(struct sockaddr_in);
                    client_socket = accept(serversocket[i], (void *)&ClnAddr, (socklen_t *)&Len);
                }
                else
                {
                    Len = sizeof(struct sockaddr_in6);
                    client_socket = accept(serversocket[i], (void *)&ClnIPV6Addr, (socklen_t *)&Len);
                }


                if(client_socket < 0)
                {
                    return RH_GetPrivateError();
                }
                else
                {
//    				dlog(LOG_DEBUG, "<RH_GetConnectNonblockFd IS OK> <accept> <FD : %d> <ClientAddr :%s>\n", Fd, inet_ntoa(ClnAddr.sin_addr));
                    if(i == 0)
                    {
                        inet_ntop(AF_INET, &(ClnAddr.sin_addr), AcceptIp, ipLen);

//    					struct sockaddr_in local;
//    					getsockname(client_socket,(struct sockaddr*)&local, Len);
//    					printf("++++++++ip:%s Len:%d\n", (char*)inet_ntoa(local.sin_addr), Len);
                    }
                    else
                    {
                        struct sockaddr_in6 *addrbak = (struct sockaddr_in6*)&ClnIPV6Addr;
                        inet_ntop(AF_INET6, &(addrbak->sin6_addr), AcceptIp, ipLen);
                        //inet_ntop(AF_INET, &(ClnAddr.sin_addr), AcceptIp, ipLen);
                       // sprintf(AcceptIp, inet_ntoa(ClnAddr.sin_addr));

//    					struct sockaddr_in local;
//    					struct sockaddr_in6 *addrbak = &local;
//    					getsockname(client_socket,(struct sockaddr*)&local, Len);
//    					inet_ntop(AF_INET6, &(addrbak->sin6_addr), AcceptIp, ipLen);
//    					printf("++++++++ip:%s\n", (AcceptIp));
                    }
                    // printf("AcceptIp:%s ipLen:%d\n", AcceptIp, ipLen);
                    return client_socket;

                }
            }
        }

    }
    else if(0 == s32Ret)
    {
        return RH_GetPrivateError();
    }
    else
    {
        return RH_GetPrivateError();
    }
    return 0;
}

static void* netServer_thread(void * argc)
{
    Net_Server_Handle_t* serverHandle = (Net_Server_Handle_t*)argc;
    InparamServerNet_t* inParam = NULL;
    Net_Opreate_Hanle_S* pOprHandle = NULL;
    int client_socket = -1;
    pthread_t tid;
    int ret = 0;
    char clientip[MAX_IPV6_BUF_LEN] = {0};
    int socketnum = 1;
    if(serverHandle == NULL)
    {
        goto EXIT;
    }
    inParam = &(serverHandle->inParam);
    serverHandle->serversocket[0] = RH_CreateTcpBindFd(inParam->nPort, NULL);
    if(serverHandle->serversocket[0] < 2)
    {
        inParam->logFun("server_socket create failed,port[%u].\n", inParam->nPort);
        goto EXIT;
    }

    if(listen(serverHandle->serversocket[0] , 30) < 0)
    {
        inParam->logFun("listen error:%d,error msg:=%s,port[%u].\n", errno, strerror(errno), inParam->nPort);
        goto EXIT;
    }
    if(inParam->support_ipv6 == 1)
    {
        serverHandle->serversocket[1] = RH_CreateTcpBindIPV6Fd(inParam->nPort, NULL);
        if(serverHandle->serversocket[1] < 2)
        {
            inParam->logFun("ipv6 server_socket create failed,port[%u].\n", inParam->nPort);

        }
        else
        {
            if(listen(serverHandle->serversocket[1] , 30) < 0)
            {
                inParam->logFun("ipv6 listen error:%d,error msg:=%s,port[%u].\n", errno, strerror(errno), inParam->nPort);
            }
        }
        //有些机器未必支持IPV6
        if(serverHandle->serversocket[1] > 2)
        {
            socketnum = 2;
        }

    }

    setServerStatus(serverHandle, SERVER_STAR);
    inParam->logFun("creat server is sucessful*********\n");
    while(getServerStatus(serverHandle) == SERVER_STAR)
    {

        client_socket = select_socket_accept(serverHandle->serversocket,socketnum,  5, clientip, sizeof(clientip));
        if(client_socket > 2)
        {
            //当小于等于0默认不限制，来者不拒
            if((getServeConNum(serverHandle) >= inParam->nConnectMaxNum) && (inParam->nConnectMaxNum > 0))
            {
                inParam->logFun("maxConnectNum:%d nowConnectNum:%d\n",inParam->nConnectMaxNum ,  getServeConNum(serverHandle));
                continue;
            }
            pOprHandle =(Net_Opreate_Hanle_S*) manag_malloc(sizeof(Net_Opreate_Hanle_S));
            if(pOprHandle == NULL)
            {
                dlog(LOG_ERROR, "RH_GetConnectNonblockFd manag_malloc is error\n");
                goto EXIT;
            }
              struct sockaddr_in flocal;
              int faddr_len = sizeof(struct sockaddr_in );
             getpeername(client_socket,(struct sockaddr*)&flocal, (socklen_t *)&faddr_len);


            memset(pOprHandle, 0 , sizeof(Net_Opreate_Hanle_S));

            pOprHandle->inParam.logFun = inParam->logFun;
            pOprHandle->inParam.statusFun = inParam->statusFun;
            pOprHandle->inParam.cmdfun = inParam->cmdfun;
            pOprHandle->inParam.param = inParam->param;
            pOprHandle->inParam.overtime = inParam->overtime;
            pOprHandle->inParam.heartmsg = inParam->heartmsg;
            pOprHandle->inParam.u16ProjectCode = inParam->u16ProjectCode;
            pOprHandle->inParam.enProtocolType = inParam->enProtocolType;
            pOprHandle->socket = client_socket;
            pOprHandle->serverHandle = serverHandle;
            strncpy(pOprHandle->usermessege.ip, clientip, sizeof(pOprHandle->usermessege.ip) - 1);
            pOprHandle->usermessege.nPort =  ntohs(flocal.sin_port);

            addServeConNum(serverHandle);
            ret = sdk_pthread_create(&tid, NULL, server_process_clientThread, (void *)(pOprHandle));
            if(ret != 0)
            {
                subServeConNum(serverHandle);
                if(pOprHandle != NULL)
                {
                    manag_free(pOprHandle);
                }
                close(client_socket);
                // printf("pthreadcreate error:%d,error msg:=%s,port[%u].\n", errno, strerror(errno), inParam->nPort);
                inParam->logFun("pthreadcreat error:%d,error msg:=%s,port[%u].\n", errno, strerror(errno), inParam->nPort);
                continue;
            }

        }
        //sleep(1);
    }
EXIT:
    dlog(LOG_ERROR, "*********netServer_thread is error code is error error please check code*******\n");
    pthread_detach(pthread_self());
    exit(0);
    return NULL;
}
Sdk_ServerNet_Handle_t sdkserver_init_net(InparamServerNet_t netserverparm)
{
    //Sdk_ServerNet_Handle_t serverHandle = NULL;
    if(netserverparm.cmdfun == NULL || netserverparm.logFun == NULL || netserverparm.statusFun == NULL ||
            netserverparm.nPort <= 0 || netserverparm.overtime <= 0)
    {
        return NULL;
    }
    Net_Server_Handle_t* serverHandle = (Net_Server_Handle_t*)manag_malloc(sizeof(Net_Server_Handle_t));
    if(serverHandle == NULL)
    {
        netserverparm.logFun("sdkserver_init_net is malloc fail\n");
        return serverHandle;
    }
    memset(serverHandle, 0 ,sizeof(Net_Server_Handle_t));
    memcpy(&(serverHandle->inParam), &netserverparm, sizeof(InparamServerNet_t));

    pthread_mutex_init(&(serverHandle->serverlock), NULL);
    serverHandle->pListClient = list_create();

    int ret = sdk_pthread_create(&(serverHandle->servertid), NULL, netServer_thread, (void *)(serverHandle));

    if (ret != 0)
    {
        netserverparm.logFun("crate NetServer thread failed\n");
         manag_free(serverHandle);
        return NULL;
    }
    return serverHandle;
}

