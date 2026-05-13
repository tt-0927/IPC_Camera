
#include <stdio.h>
#include "os.h"
#include "os_pipe.h"

#ifndef WIN32
#include <unistd.h>

typedef struct OS_PIPE_ {
    int pfd[2];
    unsigned long  pipeSize;
    unsigned long  messageSize;
    unsigned char  isFixedMessage;
    int messageCount;
    int totalBytesInPipe;
} OS_PIPE;


int OS_CreatePipe (OS_PTR *pPipe, unsigned long  pipeSize,
                                          unsigned long  messageSize,
                                          unsigned char  isFixedMessage)
{
    int bReturnStatus = OS_EFAIL;
    OS_PIPE *pHandle = NULL;
    OS_PIPE *pHandleBackup = NULL;

    *pPipe = NULL;

    pHandle = (OS_PIPE *)OS_memAlloc(sizeof(OS_PIPE));

    if (NULL == pHandle) {
        goto EXIT;
    }

    if (0 != pipe(pHandle->pfd)) {
        goto EXIT;
    }

	if(pHandle->pfd[0] == 0 || pHandle->pfd[0] == 1 || pHandle->pfd[0] == 2 ||
       pHandle->pfd[1] == 0 || pHandle->pfd[1] == 1 || pHandle->pfd[1] == 2)
    {
        pHandleBackup = (OS_PIPE *)OS_memAlloc(sizeof(OS_PIPE));
        if (NULL == pHandleBackup)
        {
            goto EXIT;
        }
		if (0 != pipe(pHandleBackup->pfd))
        {
            goto EXIT;
        }

		if(pHandleBackup->pfd[0] == 2 || pHandleBackup->pfd[1] == 2)
        {
            int pfdDummy[2];

            if (0 != close(pHandleBackup->pfd[0]))
            {
               goto EXIT;
            }
            if (0 != close(pHandleBackup->pfd[1]))
            {
                goto EXIT;
            }
            /*Allocating the reserved file descriptor to dummy*/
            if(0 != pipe(pfdDummy))
            {
                goto EXIT;
            }
            /*Now the backup pfd will not get a reserved value*/
            if (0 != pipe(pHandleBackup->pfd))
            {
                goto EXIT;
            }
            /*Closing the dummy pfd*/
            if (0 != close(pfdDummy[0]))
            {
               goto EXIT;
            }
            if (0 != close(pfdDummy[1]))
            {
                goto EXIT;
            }

        }
		if (0 != close(pHandle->pfd[0]))
        {
            goto EXIT;
        }
        if (0 != close(pHandle->pfd[1]))
        {
            goto EXIT;
        }
        OS_memFree(pHandle);

        pHandle = NULL;

        pHandleBackup->pipeSize = pipeSize;
        pHandleBackup->messageSize = messageSize;
        pHandleBackup->isFixedMessage = isFixedMessage;
        pHandleBackup->messageCount = 0;
        pHandleBackup->totalBytesInPipe = 0;

		*pPipe = (OS_PTR) pHandleBackup ;
	}
	else
    {
        pHandle->pipeSize = pipeSize;
        pHandle->messageSize = messageSize;
        pHandle->isFixedMessage = isFixedMessage;
        pHandle->messageCount = 0;
        pHandle->totalBytesInPipe = 0;

        *pPipe = (OS_PTR) pHandle ;
    }

    bReturnStatus = OS_SOK;

EXIT:
    if ((OS_SOK != bReturnStatus) && (NULL != pHandle)) {
       OS_memFree(pHandle);
    }

    if ((OS_SOK != bReturnStatus) && (NULL != pHandleBackup)) {
       OS_memFree(pHandleBackup);
    }
    return bReturnStatus;
}

int OS_DeletePipe (OS_PTR pPipe)
{
    int bReturnStatus = OS_SOK;

    OS_PIPE *pHandle = (OS_PIPE *)pPipe;

    if(NULL == pHandle) {
        bReturnStatus = OS_EFAIL;
        goto EXIT;
    }

    if (0 != close(pHandle->pfd[0])) {
        /*OSA_Error ("Delete_Pipe Read fd failed!!!");*/
        bReturnStatus = OS_EFAIL;
    }
    if (0 != close(pHandle->pfd[1])) {
        /*OSA_Error ("Delete_Pipe Write fd failed!!!");*/
        bReturnStatus = OS_EFAIL;
    }

    OS_memFree(pHandle);
EXIT:
    return bReturnStatus;
}


int OS_WriteToPipe (OS_PTR pPipe, void *pMessage,
                                           unsigned long size,
                                           unsigned long timeout)
{
    int bReturnStatus = OS_SOK;
    unsigned long lSizeWritten = -1;

    OS_PIPE *pHandle = (OS_PIPE *)pPipe;

    if(size == 0) {
        printf("Nothing to write");
        bReturnStatus = OS_EFAIL;
        goto EXIT;
    }

    lSizeWritten = write(pHandle->pfd[1], pMessage, size);

    if(lSizeWritten != size){
        printf("Writing to Pipe failed");
        bReturnStatus = OS_EFAIL;
        goto EXIT;
    }

    /*Update message count and size*/
    pHandle->messageCount++;
    pHandle->totalBytesInPipe += size;

    bReturnStatus = OS_SOK;

EXIT:
    return bReturnStatus;
}

int OS_ReadFromPipe (OS_PTR pPipe, void *pMessage,
                                            unsigned long size,
                                            unsigned long *actualSize,
                                            long timeout)
{
        int bReturnStatus = OS_SOK;
        unsigned long lSizeRead = -1;
        OS_PIPE *pHandle = (OS_PIPE *)pPipe;

        if((size == 0) /*|| (pHandle->messageCount == 0)*/) {           /*|| size > SSIZE_MAX)*/
            /*OSA_Error("Read size has error.");*/
            bReturnStatus = OS_EFAIL;
            goto EXIT;
        }

        *actualSize =  lSizeRead = read(pHandle->pfd[0], pMessage, size);
        if(0 == lSizeRead){
            /*OSA_Error("EOF reached or no data in pipe");*/
            bReturnStatus = OS_EFAIL;
            goto EXIT;
        }

        bReturnStatus = OS_SOK;

        /*Update message count and pipe size*/
        pHandle->messageCount--;
        pHandle->totalBytesInPipe -= size;

    EXIT:
        return bReturnStatus;

}

#endif



