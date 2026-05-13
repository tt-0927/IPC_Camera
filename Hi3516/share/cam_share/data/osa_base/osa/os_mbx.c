

#include "os_mbx.h"
#include "os_debug.h"

int OS_mbxCreate(OS_MbxHndl *pMbxHndl)
{
	int status=OS_SOK;

	status |= OS_msgqCreate(&pMbxHndl->rcvMbx);
	status |= OS_msgqCreate(&pMbxHndl->ackMbx);

	if (status!=OS_SOK)
		OS_ERROR("OS_mbxCreate() = %d \r\n", status);

	return status;
}

int OS_mbxDelete(OS_MbxHndl *pMbxHndl)
{
	OS_msgqDelete(&pMbxHndl->rcvMbx);
	OS_msgqDelete(&pMbxHndl->ackMbx);

	return OS_SOK;
}

int OS_mbxFreeMsg(OS_MsgHndl *pMsg)
{
  if(pMsg->flags & OS_MBX_FREE_PRM) {
    if(pMsg->pPrm!=NULL) {
      OS_memFree(pMsg->pPrm);
    }
  }

  OS_assertSuccess( OS_msgqFreeMsgHndl(pMsg) );

  return OS_SOK;
}

int OS_mbxSendMsg(OS_MbxHndl *pMbxTo, OS_MbxHndl *pMbxFrom, Uint32 cmd, void *pPrm, Uint32 flags)
{
  OS_MsgHndl *pSentMsg, *pRcvMsg;
  int waitAck = 0;
  int retVal=OS_SOK;
  OS_MsgqHndl *ackMbx;

  OS_assert(pMbxTo!=NULL);

  //判断是否有返回队列
  if(pMbxFrom==NULL)
  {
    if(flags & OS_MBX_WAIT_ACK)
    {
      // if from mail box is NULL, then cannot wait for ACK
      OS_assert(0);
    }
    ackMbx = NULL;
  } else
  {
    ackMbx = &pMbxFrom->ackMbx;
  }

  retVal = OS_msgqSendMsg(&pMbxTo->rcvMbx, ackMbx, cmd, pPrm, flags, &pSentMsg);
  OS_assertSuccess(retVal);

  //判断是否需要等待
  if( (flags & OS_MBX_WAIT_ACK) && ackMbx != NULL )
  {

    waitAck = TRUE;

    do
    {
      // wait for ACK
      retVal = OS_msgqRecvMsg(ackMbx, &pRcvMsg, OS_TIMEOUT_FOREVER);
      OS_assertSuccess(retVal);

      if(pRcvMsg==pSentMsg)
      {
        // ACK received for sent MSG
        waitAck = FALSE;
        retVal  = OS_msgGetAckStatus(pRcvMsg);

      }else
      {
          OS_printf("OS_MBX:MSG RECEIVED OUT OF ORDER.SERIOUS ERROR!!! Expected[%d]:Received[%d]",pSentMsg->cmd,pRcvMsg->cmd);
          OS_assert(FALSE);
      }

      //释放消息体
      OS_mbxFreeMsg(pRcvMsg);

    } while(waitAck);
  }

  return retVal;
}

int OS_mbxBroadcastMsg(OS_MbxHndl *pMbxToList[], OS_MbxHndl *pMbxFrom, Uint32 cmd, void *pPrm, Uint32 flags)
{
  OS_MsgHndl *pSentMsgList[OS_MBX_BROADCAST_MAX];
  int  ackList[OS_MBX_BROADCAST_MAX];
  OS_MsgHndl *pRcvMsg;
  int waitAck, unknownAck;
  Uint32 i, numMsg;
  int retVal=OS_SOK, ackRetVal = OS_SOK;
  OS_MsgqHndl *ackMbx;

  OS_assert(pMbxToList!=NULL);

  if(pMbxFrom==NULL) {
    if(flags & OS_MBX_WAIT_ACK) {
      // if from mail box is NULL, then cannot wait for ACK
      OS_assert(0);
    }
    ackMbx = NULL;
  } else {
    ackMbx = &pMbxFrom->ackMbx;
  }

   // count number of mailboxes to boadcast to
   numMsg=0;
   while(pMbxToList[numMsg]!=NULL) {
     numMsg++;
     if(numMsg>=OS_MBX_BROADCAST_MAX) {
        // cannot broadcast to more than OS_MBX_BROADCAST_MAX mailboxes
        OS_assert(0);
     }
   }

   if(numMsg == 0) {
     // no mailboxes in 'to' mailbox list
     return OS_SOK;
   }

   // Cannot broadcast with malloc'ed prm and not wait for ACK
   if(flags & OS_MBX_FREE_PRM) {
     if( !(flags & OS_MBX_WAIT_ACK) && numMsg > 1) {
       OS_assert(0);
     }
   }

   // reset sent MSG list and ACK list
   for(i=0; i<OS_MBX_BROADCAST_MAX; i++) {
     ackList[i]  = FALSE;
     pSentMsgList[i] = NULL;
   }

   // send MSG to all mailboxes
   for(i=0; i<numMsg; i++) {
      retVal = OS_msgqSendMsg(&pMbxToList[i]->rcvMbx, ackMbx, cmd, pPrm, flags, &pSentMsgList[i]);
      OS_assertSuccess(retVal);
   }

   if((flags & OS_MBX_WAIT_ACK) && ackMbx!=NULL ) {
      // wait for ACKs
      do {
        // wait ACK
        retVal = OS_msgqRecvMsg(ackMbx, &pRcvMsg, OS_TIMEOUT_FOREVER);
        OS_assertSuccess(retVal);

        unknownAck=TRUE;

        // mark ACK as received for sent MSG
        for(i=0; i<numMsg; i++) {
          if(pRcvMsg == pSentMsgList[i] ) {
            ackList[i]=TRUE;
            unknownAck=FALSE;
            if(ackRetVal==OS_SOK) {
              ackRetVal  = OS_msgGetAckStatus(pRcvMsg);
            }
            break;
          }
        }

        // check if all ACKs received
        waitAck = FALSE;
        for(i=0; i<numMsg; i++) {
          if(ackList[i]==FALSE ) {
            waitAck = TRUE;
            break;
          }
        }

        if(unknownAck) {
          // ACK received is for some old message, hence free MSG and prm
          OS_mbxFreeMsg(pRcvMsg);
        } else {
          // only free MSG now, free prm after all ACKs are received
          OS_assertSuccess( OS_msgqFreeMsgHndl(pRcvMsg) );
        }

      } while(waitAck);

      if(flags & OS_MBX_FREE_PRM) {
        if(pPrm!=NULL) {
          OS_memFree(pPrm);
        }
      }

      retVal = ackRetVal;
   }

   return retVal;
}


int OS_mbxWaitMsg(OS_MbxHndl *pMbxHndl, OS_MsgHndl **pMsg)
{
   int retVal;

   retVal = OS_msgqRecvMsg(&pMbxHndl->rcvMbx, pMsg, OS_TIMEOUT_FOREVER);
   OS_assertSuccess(retVal);

   return retVal;
}

int OS_mbxCheckMsg(OS_MbxHndl *pMbxHndl, OS_MsgHndl **pMsg)
{
   int retVal;

   retVal = OS_msgqRecvMsg(&pMbxHndl->rcvMbx, pMsg, OS_TIMEOUT_NONE);

   return retVal;
}

int OS_mbxAckOrFreeMsg(OS_MsgHndl *pMsg, int ackRetVal)
{
  int retVal=OS_SOK;

  if(pMsg==NULL)
    return OS_EFAIL;

  if(pMsg->flags & OS_MBX_WAIT_ACK)
  {
     // ACK message 返回ACK应答信号
     retVal = OS_msgqSendAck(pMsg, ackRetVal);
     OS_assertSuccess(retVal);
  } else
  {
    // FREE message and prm
    OS_mbxFreeMsg(pMsg);
  }

  return retVal;
}

int OS_mbxFlush(OS_MbxHndl *pMbxHndl)
{
  int retVal;
  OS_MsgHndl *pMsg;

  // flush receive mailbox
  do {
    retVal = OS_mbxCheckMsg(pMbxHndl, &pMsg);
    if(retVal==OS_SOK)
    {
      OS_mbxAckOrFreeMsg(pMsg, OS_SOK);
    }
  }while(retVal==OS_SOK);

  // flush ACK mailbox
  do {
    retVal = OS_msgqRecvMsg(&pMbxHndl->ackMbx, &pMsg, OS_TIMEOUT_NONE);
    if(retVal==OS_SOK) {
      OS_mbxFreeMsg(pMsg);
    }
  } while(retVal==OS_SOK);

  return retVal;
}


int OS_mbxWaitCmd(OS_MbxHndl *pMbxHndl, OS_MsgHndl **pMsg, Uint16 waitCmd)
{
  OS_MsgHndl *pRcvMsg;

  while(1)
  {
    OS_mbxWaitMsg(pMbxHndl, &pRcvMsg);
    if(OS_msgGetCmd(pRcvMsg)==waitCmd)
      break;

    OS_mbxAckOrFreeMsg(pRcvMsg, OS_SOK);
  }

  if(pMsg==NULL)
  {
    OS_mbxAckOrFreeMsg(pRcvMsg, OS_SOK);
  } else {
    *pMsg = pRcvMsg;
  }

  return OS_SOK;
}

