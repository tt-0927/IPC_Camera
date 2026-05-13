/*
*  File Name        : previewlog.c
*  Created on       : 2021-12-25
*  Author           : EasonLu
*  description      : MDS预览日志时调用的日志记录按行读取操作
*  Modify date      : 2021-12-25
*  Modifier Author  : EasonLu
*  description      : 用于详细说明此程序文件完成的主要功能
*/
#include "previewlog.h"
#include "share_jsonBase.h"
#include "device_data_type.h"
#include "config_file_path.h"

RETURN_STATUS_E readRowBufferSize(RowBuffer_S *pRow, const char *pLogPath, int nLine)
{
    /*打开文件*/
    FILE *pFile = NULL;
    pFile = fopen(pLogPath,"r+");
    if(pFile==NULL){
        printf("open log failed,log path is %s\r\n",pLogPath);
        dlog(LOG_ERROR,"open log failed,log path is %s",pLogPath);
        return RETURN_ERROR;
    }

    /*获取文件大小*/
    long nFileSize = 0;
    fseek(pFile,0,SEEK_END);
    nFileSize = ftell(pFile);
//    dlog(LOG_WARN,"[Total File Size:%ld]",nFileSize);

    /*记录段落的传入初始值*/
    int nBeginSize = pRow->nBeginSize;
    int nEndSize = pRow->nEndSize;

    /*负值为需要计算的段落值*/
    bool bFromBegin = false;
    if(nBeginSize == -1 && nEndSize != -1){
        bFromBegin = false;
    }else if(nBeginSize != -1 && nEndSize == -1){
        bFromBegin = true;
    }else if(nBeginSize == -1 && nEndSize == -1){
        /*两边均为负值时则从尾部进行读取*/
        nEndSize = nFileSize;
    }



    int nCountCR = 0;
    int nRunTime = 1;
    char *pData = NULL;
    int nTotalSize = 0;
    /*读取一定字节数，计算行数，满足需要的行数后再进行截取*/
    while((nCountCR < nLine + 1)){
        if(bFromBegin){
            nEndSize = nBeginSize + READ_PAGE_SIZE * nRunTime;
            nEndSize = nEndSize > nFileSize ? nFileSize : nEndSize;
        }else{
            nBeginSize = nEndSize - READ_PAGE_SIZE * nRunTime;
            nBeginSize = nBeginSize > 0 ? nBeginSize : 0;
        }
        nTotalSize = nEndSize - nBeginSize;
        nTotalSize = nTotalSize > 0 ? nTotalSize : 0;
//        dlog(LOG_DEBUG,"[Read Total Size:%d]\t[Begin:%d]\t[End:%d]",nTotalSize,nBeginSize,nEndSize);
        fseek(pFile,(bFromBegin ? nBeginSize : nEndSize - nTotalSize),SEEK_SET);
        if(pData!=NULL){
            free(pData);
            pData = NULL;
        }
        pData = (char*)malloc(nTotalSize);
        if(pData==NULL){
            dlog(LOG_ERROR,"malloc failed~~~~");
            break;
        }
        memset(pData,0,nTotalSize);
        fread(pData,nTotalSize,1,pFile);
        nCountCR = countCR(pData,nTotalSize);
        // dlog(LOG_DEBUG, "[pData:%p][CR:%d] Read Buffer:\n%s", pData, nCountCR, pData);
        if(!bFromBegin && (nCountCR < nLine + 1 && nBeginSize == 0)){
            /*从尾部读到头部时会不满足需要读的行数*/
            break;
        }

        if(bFromBegin && (nCountCR < nLine + 1 && nEndSize == nFileSize)){
            /*从头部读到尾部时会不满足需要读的行数*/
            break;
        }

        nRunTime++;
    }

    if(pData!=NULL){
        if(bFromBegin){
            /*截取头部需要的行数*/
            int nLineCount = 0;
            for(int i=0;i<nTotalSize;i++){
                if(pData[i] == '\n'){
                    nLineCount++;
//                    dlog(LOG_DEBUG,"From Begin Size Line Count [%d]" ,nLineCount);
                }
                if(nLineCount >= nLine){
                    nEndSize = nBeginSize + i;
//                    dlog(LOG_DEBUG,"From Begin Size Line Count [%d],End Size [%d]" ,nLineCount,nEndSize);
                    break;
                }
            }
            pRow->nEndSize = nEndSize;
        }else{
            /*去掉头部多余的行数，截取最后面所需要的行数*/
            int nLineCount = nLine - nCountCR;
            int nNewBeginSize = 0;
            for(int i=0;i<nTotalSize;i++){
                if(pData[i] == '\n'){
                    nLineCount++;
                    if(nLineCount == 0){
                        nNewBeginSize = nBeginSize + i + 1;
                    }else if(nLineCount == nLine){
                        nEndSize = nBeginSize + i;
                    }
                }
            }
            pRow->nBeginSize = nNewBeginSize;
            pRow->nEndSize = nEndSize;

            /*NOTE: 调试输出*/
#if 0
            nTotalSize = nEndSize - nNewBeginSize;
            nTotalSize = nTotalSize > 0 ? nTotalSize : 0;
            fseek(pFile,nNewBeginSize,SEEK_SET);
            dlog(LOG_DEBUG,"[Read Total Size:%d]\t[Begin:%d]\t[End:%d]",nTotalSize,nNewBeginSize,nEndSize);
            if(pData!=NULL){
                free(pData);
            }
            pData = (char*)malloc(nTotalSize);
            memset(pData,0,nTotalSize);
            fread(pData,nTotalSize,nTotalSize,pFile);
            nCountCR = countCR(pData,nTotalSize);
            dlog(LOG_DEBUG,"[CR:%d] Read Buffer:\n%s",nCountCR,pData);
#endif

        }

        free(pData);
        pData = NULL;
    }

    fclose(pFile);
    pFile = NULL;
    return RETURN_DATA_SUCCESS;
}

int countCR(char *pData, int nLength)
{
    if(pData==NULL){

        return -1;
    }
    int nCount = 0;
    for(int i=0;i<nLength;i++){
        if(pData[i] == '\n'){
            nCount++;
        }
    }

    return nCount;
}

RETURN_STATUS_E getBufferSizeLog(RowBuffer_S *pRow, const char *pLogPath, char *pData)
{

    int nTotalSize = pRow->nEndSize - pRow->nBeginSize;
    if(nTotalSize <= 0){
        return RETURN_ERROR;
    }

    /*打开文件*/
    FILE *pFile = NULL;
    pFile = fopen(pLogPath,"r+");
    if(pFile==NULL){
        dlog(LOG_ERROR,"open log failed,log path is %s",pLogPath);
        return RETURN_ERROR;
    }

    if(pData==NULL){
        dlog(LOG_ERROR,"data is null");
        return RETURN_ERROR;
    }

    fseek(pFile,pRow->nBeginSize,SEEK_SET);
    if(fread(pData,nTotalSize,1,pFile) < 0){
        dlog(LOG_ERROR,"read file fail");
        return RETURN_ERROR;
    }

    fclose(pFile);
    pFile = NULL;

    return RETURN_DATA_SUCCESS;
}

RETURN_STATUS_E getDataFromCallback(char *pCallBack,char **pJsonBuf)
{
    /*解析日志类型，采用dlog的日志类型*/
    int nLogType = -1;
    if(json_get_int1(pCallBack,"data","LogType",&nLogType) == FALSE){
        dlog(LOG_ERROR,"json_get_int1() fail");
        return RETURN_ERROR;
    }

    /*根据日志类型转换日志名称的关键词*/
    char *pLogType = NULL;
    switch (nLogType) {
    case LOG_USER:pLogType="user";break;
    case LOG_DACU:pLogType="dacu";break;
    case LOG_FAULT:pLogType="fault";break;
    default:{
        dlog(LOG_ERROR,"no log type return,error log type is [%d]",nLogType);
        return RETURN_ERROR;
    }
    }

    /*解析日期*/
    char aDate[64] = {0};
    if(json_get_char1(pCallBack,"data","Date",aDate,sizeof(aDate)) == FALSE){
        dlog(LOG_ERROR,"aDate json_get_char1() fail");
        return RETURN_ERROR;
    }

    /*解析需要读取的段落字节数*/
    RowBuffer_S sSize={-1,-1};
    if(json_get_int1(pCallBack,"data","Begin",&sSize.nBeginSize) == FALSE){
        dlog(LOG_ERROR,"RowBuffer_S nBeginSize json_get_int1() fail");
        return RETURN_ERROR;
    }

    if(json_get_int1(pCallBack,"data","End",&sSize.nEndSize) == FALSE){
        dlog(LOG_ERROR,"RowBuffer_S nEndSize json_get_int1() fail");
        return RETURN_ERROR;
    }

    /*解析需要读取的行数*/
    int nLine = -1;
    if(json_get_int1(pCallBack,"data","Line",&nLine) == FALSE){
        dlog(LOG_ERROR,"nLine json_get_int1() fail");
        return RETURN_ERROR;
    }

    /*解析设备类型枚举*/
    int nDevType = -1;
    if(json_get_int1(pCallBack,"data","DevType",&nDevType) == FALSE){
        dlog(LOG_ERROR,"nDevType json_get_int1() fail");
        return RETURN_ERROR;
    }

    char *pDevType = NULL;
    switch (nDevType) {
    case PIS_DEVICE_MPS:;
    case PIS_DEVICE_MPS_SUPPORT:pDevType="mps";break;
    case PIS_DEVICE_NVR:;
    case PIS_DEVICE_NVR_SUPPORT:pDevType="nvr";break;
    case PIS_DEVICE_MPD:pDevType="mpd";break;
    case PIS_DEVICE_DM:pDevType="dm";break;
    case PIS_DEVICE_MDS:pDevType="mds";break;
    case PIS_DEVICE_DACU:pDevType="dacu";break;
    default:break;
    }

    /*拼接当前设备的日志路径*/
    char aLogPath[1024] = {0};
    if(strcmp(pDevType,"dacu") == 0)
    {
         sprintf(aLogPath,"%s/%s_%s_%s.log",DACU_FILE_PATH,pDevType,pLogType,aDate);
    }
    else
    {
        sprintf(aLogPath,"%s/%s_%s_%s.log",LOG_FILE_PATH,pDevType,pLogType,aDate);
    }
    
    dlog(LOG_DEBUG,"sprintf log path is [%s]",aLogPath);

    /*组装数据返回给MPS*/
    cJSON *pJsonData = cJSON_CreateObject();
    if(pJsonData == NULL){
        dlog(LOG_ERROR,"cJSON_CreateObject error!");
        return RETURN_ERROR;
    }

    /*计算日志对应行数的字节数*/
    readRowBufferSize(&sSize,aLogPath,nLine);
    if(sSize.nBeginSize == -1 || sSize.nEndSize == -1){
        dlog(LOG_ERROR,"readRowBufferSize() fail");
        /*无法加载日志也需返回空的日志记录*/
        cJSON_AddNumberToObject(pJsonData,"Begin",sSize.nBeginSize);
        cJSON_AddNumberToObject(pJsonData,"End",sSize.nEndSize);
        cJSON_AddNumberToObject(pJsonData,"LogType",nLogType);
        *pJsonBuf = cJSON_Print(pJsonData);
        if(pJsonData){
            cJSON_Delete(pJsonData);
            pJsonData = NULL;
        }
        return RETURN_ERROR;
    }

    /*需要读取的总字节数*/
    int nTotalSize = sSize.nEndSize - sSize.nBeginSize;
    if(nTotalSize <= 0){
        dlog(LOG_ERROR,"Read file size < 0");
        return RETURN_ERROR;
    }

    /*读取日志*/
    char *pData = (char*)malloc(nTotalSize + 1);
    if(pData==NULL){
        dlog(LOG_ERROR,"malloc fail~~~");
        return RETURN_ERROR;
    }
    memset(pData,0,nTotalSize + 1);
    getBufferSizeLog(&sSize,aLogPath,pData);



    /*需要添加本次读取的字节数区间记录*/
    cJSON_AddNumberToObject(pJsonData,"Begin",sSize.nBeginSize);
    cJSON_AddNumberToObject(pJsonData,"End",sSize.nEndSize);
    cJSON_AddNumberToObject(pJsonData,"LogType",nLogType);

    cJSON_AddStringToObject(pJsonData,"LogBuffer",pData);

    *pJsonBuf = cJSON_Print(pJsonData);
    if(pJsonData){
        cJSON_Delete(pJsonData);
        pJsonData = NULL;
    }

    return RETURN_DATA_SUCCESS;
}

RETURN_STATUS_E responseMDSLogRequest(char *pCallBack, \
                                      network_Handle_t pClinetHandle,\
                                      network_Handle_t pClinetHandleBackup)
{
    if(pCallBack==NULL){
        dlog(LOG_ERROR,"Callback Message is NULL  !!!");
        return RETURN_ERROR;
    }

    if(pClinetHandle==NULL){
        dlog(LOG_ERROR,"Clinet Handle is NULL !!!");
        return RETURN_ERROR;
    }

    /*响应SHARE_CMD_RESPONSE_MDS_GET_LOG指令*/
    char *pJsonBuf = NULL;
    /*传入回调报文和接收处理完Json的数据指针即可*/
    getDataFromCallback(pCallBack,&pJsonBuf);

    cJSON *pRoot;
    char *pJsonBuffer = NULL;
    pRoot = cJSON_CreateObject();
    if(pRoot == NULL){
        dlog(LOG_ERROR,"cJSON_CreateObject error!");
        return RETURN_ERROR;
    }

    /*设置通信代码*/
    cJSON_AddNumberToObject(pRoot,"actioncode",20012);
    cJSON *pData = cJSON_Parse(pJsonBuf);
    if(pJsonBuf){
        /*设置数据内容pData*/
        cJSON_AddItemToObject(pRoot,"data",pData);
    }

    pJsonBuffer = cJSON_Print(pRoot);
    dlog(LOG_DEBUG,"Send json:\n%s",pJsonBuffer);

    if(pRoot){/*释放根节点即可，会递归释放*/
        cJSON_Delete(pRoot);
        pRoot = NULL;
        pData = NULL;
    }

    if(pJsonBuf){
        free(pJsonBuf);
        pJsonBuf = NULL;
    }

    if(pJsonBuffer){
        dlog(LOG_DEBUG,"[数据长度:%d]",(int)strlen(pJsonBuffer)+1);
        /*主备服务器同步发送数据*/
        /*发送给主服务器*/
        network_send_data(pClinetHandle,pJsonBuffer,strlen(pJsonBuffer)+1,0);
        if(pClinetHandleBackup){/*发送给备服务器*/
            network_send_data(pClinetHandleBackup,pJsonBuffer,strlen(pJsonBuffer)+1,0);
        }

        free(pJsonBuffer);
        pJsonBuffer = NULL;
    }

    return RETURN_DATA_SUCCESS;
}
