

#include "os_msgq.h"
#include "os_debug.h"

int OS_msgqCreate(OS_MsgqHndl *hndl)
{
  pthread_mutexattr_t mutex_attr;
  pthread_condattr_t cond_attr;
  int status=OS_SOK;
 
  status |= pthread_mutexattr_init(&mutex_attr);
  status |= pthread_condattr_init(&cond_attr);  
  
  status |= pthread_mutex_init(&hndl->lock, &mutex_attr);
  status |= pthread_cond_init(&hndl->condRd, &cond_attr);    
  status |= pthread_cond_init(&hndl->condWr, &cond_attr);  

  hndl->curRd = hndl->curWr = 0;
  hndl->count = 0;
  hndl->len   = OS_MSGQ_LEN_MAX;

  if(status!=OS_SOK)
    OS_ERROR("OS_msgqCreate() = %d \r\n", status);
    
  pthread_condattr_destroy(&cond_attr);
  pthread_mutexattr_destroy(&mutex_attr);
    
  return status;
}

int OS_msgqDelete(OS_MsgqHndl *hndl)
{
  pthread_cond_destroy(&hndl->condRd);
  pthread_cond_destroy(&hndl->condWr);
  pthread_mutex_destroy(&hndl->lock);  
  
  return OS_SOK;
}

OS_MsgHndl *OS_msgqAllocMsgHndl(OS_MsgqHndl *to, OS_MsgqHndl *from, Uint16 cmd, void *prm, Uint16 msgFlags)
{
  OS_MsgHndl *msg;
  
  msg = OS_memAlloc( sizeof(OS_MsgHndl) );
  
  if(msg!=NULL) {
    msg->pTo = to;
    msg->pFrom = from;
    msg->pPrm = prm;
    msg->status = OS_SOK;
    msg->cmd = cmd;
    msg->flags = msgFlags;
  }

  return msg;    
}

/*
 * 模型的内部函数
 * 把消息句柄存放在消息队列中
 * 可设置阻塞发送
 * */
int OS_msgqSend(OS_MsgqHndl *hndl, OS_MsgHndl *msg, Uint32 timeout)
{
  int status = OS_EFAIL;

  pthread_mutex_lock(&hndl->lock);

  while(1)
  {
	  /*判断队列中是否有空位写入新的消息*/
    if( hndl->count < hndl->len )
    {
      hndl->queue[hndl->curWr] = msg;
      hndl->curWr = (hndl->curWr+1)%hndl->len;	//队列消息尾往下走，循环队列
      hndl->count++;
      status = OS_SOK;
      pthread_cond_signal(&hndl->condRd);
      break;
    } else
    {
      if(timeout == OS_TIMEOUT_NONE)
        break;

      status = pthread_cond_wait(&hndl->condWr, &hndl->lock);
    }
  }

  pthread_mutex_unlock(&hndl->lock);

  return status;
}


int OS_msgqSendMsg(OS_MsgqHndl *to, OS_MsgqHndl *from, Uint16 cmd, void *prm, Uint16 msgFlags, OS_MsgHndl **msg)
{
  int status;
  OS_MsgHndl *data;

  /*创建消息句柄*/
  data = OS_msgqAllocMsgHndl(to, from, cmd, prm, msgFlags);
  if(data==NULL)
    return OS_EFAIL;
  
  status = OS_msgqSend(to, data, OS_TIMEOUT_FOREVER);
  
  if(status==OS_SOK) {
    if(msg!=NULL)
      *msg = data;
  }

  return status;
}

int OS_msgqRecvMsg(OS_MsgqHndl *hndl, OS_MsgHndl **msg, Uint32 timeout)
{
  int status = OS_EFAIL;
  
  pthread_mutex_lock(&hndl->lock);
  
  while(1)
  {
    if(hndl->count > 0 )
    {

      if(msg!=NULL) {
        *msg = hndl->queue[hndl->curRd];
      }
      
      hndl->curRd = (hndl->curRd+1)%hndl->len;
      hndl->count--;
      status = OS_SOK;
      pthread_cond_signal(&hndl->condWr);
      break;

    } else
    {
      if(timeout == OS_TIMEOUT_NONE)
        break;
      status = pthread_cond_wait(&hndl->condRd, &hndl->lock);
    }
  }

  pthread_mutex_unlock(&hndl->lock);

  return status;
}

int OS_msgqSendAck(OS_MsgHndl *msg, int ackRetVal)
{
  int status;

  msg->status = ackRetVal;
  
  status = OS_msgqSend(msg->pFrom, msg, OS_TIMEOUT_FOREVER);

  return status;
}

int OS_msgqFreeMsgHndl(OS_MsgHndl *msg)
{
  OS_memFree(msg);
  return OS_SOK;
}



