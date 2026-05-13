
/*	消息队列机制
 * 	1><双向通讯>可用于下列的模型,本基础库中邮箱模型就是下列的模型，使用该模型需要创建两个handle：一个to，一个from
 *	OSA_msgqCreate(to),OSA_msgqCreate(from)
 *
 *			  <----- 通讯对象A <------
 *
 *			|向to队列发送消息			|从from队列中获取对象B返回的ACK信号以及B的消息
 *			|						|
 *			|						|
 *	*to(OS_MsgqHndl)		*from(OS_MsgqHndl)
 *	 		|						|
 *			|						|
 *			|从to队列中获取消息			|向from队列中返回ACK信号以及发给A的消息
 *
 *			--------> 通讯对象B ------>
 *
 *
 *
 *	2><单向通讯>也可用于单队列模型，就是接收者无法返回ACK应答信号给发送者，以及无法双向通讯
 *		创建一个handle即可，如to，OSA_msgqCreate(to)
 * 			  <----- 通讯对象A
 *
 *			|向to队列发送消息
 *			|
 *			|
 *	*to(OS_MsgqHndl)
 *	 		|
 *			|
 *			|从to队列中获取消息
 *
 *			--------> 通讯对象B
 *
 *
 *	注：消息接收者，处理完消息之后，要么返回一个ACK信号，要么free消息句柄
 *
 *
 * */


#ifndef _OS_MSGQ_H_
#define _OS_MSGQ_H_

#ifdef __cplusplus
extern "C" {
#endif
#include "os.h"

#define OS_MSGQ_LEN_MAX    32

struct OS_MsgHndl;

/*
 * 消息队列句柄
 * 记录一条条的消息句柄
 * FIFO	先进先出机制
 * */
typedef struct {

  Uint32 curRd;		//记录了队列消息头--从队列头读消息
  Uint32 curWr;		//记录了队列消息尾--往队列尾写消息
  Uint32 len;		//记录该模型下，消息队列的最大长度
  Uint32 count;		//记录当前消息队列中有多少个消息

  struct OS_MsgHndl *queue[OS_MSGQ_LEN_MAX];

  pthread_mutex_t lock;
  pthread_cond_t  condRd;
  pthread_cond_t  condWr;
  
} OS_MsgqHndl;

/*
 * 消息句柄
 * 记录消息的具体内容
 * */
typedef struct OS_MsgHndl {

  OS_MsgqHndl *pTo;		//指向发送消息队列句柄
  OS_MsgqHndl *pFrom;  	//指向返回消息队列句柄，看需要使用，单队列模型不使用，双向通讯模型使用

  /*使用该模型的上层应用自定义使用的字段*/
  void         *pPrm;		//上层应用指向具体的消息内容
  int           status;		//上层应用记录读取消息后返回的ACK应答值
  Uint16        cmd;		//上层应用自定义使用,用于表示每条消息的命令，因为每条信息都需要带具体的处理命令
  Uint16        flags;		//上层应用自定义使用

} OS_MsgHndl;

#define OS_msgGetCmd(msg)         ( (msg)->cmd )
#define OS_msgGetPrm(msg)         ( (msg)->pPrm )
#define OS_msgGetAckStatus(msg)   ( (msg)->status )

/*
 * 创建消息队列
 * @in param hndl : 消息队列句柄
 * */
int OS_msgqCreate(OS_MsgqHndl *hndl);

/*
 * 删除消息队列
 * @in param hndl : 消息队列句柄
 * */
int OS_msgqDelete(OS_MsgqHndl *hndl);

/*
 * 向to消息队列中写入消息
 * @in parma to :	发送消息队列句柄
 * @in parma from : 接收消息队列句柄
 * @in parma cmd :	具体消息的命令
 * @in parma prm :	具体消息的内容
 * @in parma msgFlags :	具体消息的flag
 * @in parma msg : 具体消息句柄二级指针，在该函数内创建内存
 *
 * */
int OS_msgqSendMsg(OS_MsgqHndl *to, OS_MsgqHndl *from, Uint16 cmd, void *prm, Uint16 msgFlags, OS_MsgHndl **msg);

/*
 * 从消息队列中获取消息
 * @in param hndl : 消息队列句柄
 * @in param msg : 获取消息的句柄
 * @in param timeout : 获取消息超时 设置为OSA_TIMEOUT_NONE则为阻塞等待，其他时间暂不支持
 *
 * */
int OS_msgqRecvMsg(OS_MsgqHndl *hndl, OS_MsgHndl **msg, Uint32 timeout);

/*
 * 向from消息队列中发送一个ACK应答信号
 * @in param msg : 具体的消息句柄
 * @in param ackRetVal : 应答值
 *
 * */
int OS_msgqSendAck(OS_MsgHndl *msg, int ackRetVal);

/*
 * 删除消息句柄
 * @in param msg : 具体的消息句柄
 * */
int OS_msgqFreeMsgHndl(OS_MsgHndl *msg);

#ifdef __cplusplus
}
#endif
#endif /* _OSA_FLG_H_ */



