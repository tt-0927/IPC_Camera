

/*	邮箱队列机制
 *
 *			  <----- 通讯对象A <------
 *
 *			|向rcvMbx队列发送消息		|从ackMbx队列中获取对象B返回的ACK信号以及B的消息
 *			|						|
 *			|						|
 *	*rcvMbx(OSA_MsgqHndl)		*ackMbx(OSA_MsgqHndl)
 *	 		|						|
 *			|						|
 *			|从rcvMbx队列中获取消息		|向ackMbx队列中返回ACK信号以及发给A的消息
 *
 *			--------> 通讯对象B ------>
 *
 *
 * */



#ifndef _OS_MBX_H_
#define _OS_MBX_H_

#ifdef __cplusplus
extern "C" {
#endif
#include "os_msgq.h"

#define OS_MBX_WAIT_ACK      0x0002   ///< Message Flag : Wait for ACK
#define OS_MBX_FREE_PRM      0x0004   ///< Message Flag : Message parameters are malloc'ed and need to be free'ed

#define OS_MBX_BROADCAST_MAX     10   ///< Max message queues/PRC's that can be broadcast to

/**
  \brief Mailbox handle
*/
typedef struct {

  OS_MsgqHndl rcvMbx;    ///< Receive mailbox
  OS_MsgqHndl ackMbx;    ///< ACK mailbox

} OS_MbxHndl;

/*
 * create mbx handle
 *
 * */
int OS_mbxCreate(OS_MbxHndl *pHndl);

/*
 * delete mbx handle
 *
 * */
int OS_mbxDelete(OS_MbxHndl *pHndl);

/*
 * 发送消息到邮箱队列中
 * @in param pTo : 发送邮箱队列句柄
 * @in param pFrom : 返回邮箱队列句柄，可设置pTo的句柄，跟上个参数的句柄相同，也可置为NULL，则无返回应答ACK信号
 * @in param cmd : 具体消息中的命令
 * @in param pPrm : 具体消息（上层用户自定义）
 * @in parma flags : 判断是否等待返回的ACK信号，取值OSA_MBX_WAIT_ACK即阻塞等待，否则就是上层应用可使用的flag
 * @out param return : 返回的ACk应答信号
 *
 * */
int OS_mbxSendMsg(OS_MbxHndl *pTo, OS_MbxHndl *pFrom, Uint32 cmd, void *pPrm, Uint32 flags);

/*
 * 广播到所有的发送邮箱队列中
 * @in param pToList : 需要发送的发送邮箱队列句柄
 * @in 同上
 *
 * */
int OS_mbxBroadcastMsg(OS_MbxHndl *pToList[], OS_MbxHndl *pFrom, Uint32 cmd, void *pPrm, Uint32 flags);

/*
 * 根据发送者对flag的置位情况是返回ACK信号还是销毁消息
 * @in param pMsg : 从队列中获取到的具体消息句柄
 * @in param ackRetVal : 若发送者需要接收者返回ACK应答信号，则该字段表示返回给发送者的ACK应答字段
 *
 * */
int OS_mbxAckOrFreeMsg(OS_MsgHndl *pMsg, int ackRetVal);

/*
 * 等待邮箱消息
 * @in param pHndl : 需要等待的邮箱消息队列句柄
 * @in param pMsg : 从邮箱消息队列中返回的具体消息句柄
 *
 * */
int OS_mbxWaitMsg(OS_MbxHndl *pHndl, OS_MsgHndl **pMsg);

/*
 *  等待接收返回者返回的ACK应答信号值
 *  @in param pHndl : 需要等待的邮箱消息队列句柄
 *  @in param pMsg : 从邮箱队列中返回的具体消息句柄
 *  @out param return : 返回的ACK值
 *
 * */
int OS_mbxCheckMsg(OS_MbxHndl *pHndl, OS_MsgHndl **pMsg);

/*
 * 一直等待到自己需要的命令的消息
 * @in param pHndl : 需要等待的邮箱消息队列句柄
 * @in param pMsg : 从邮箱队列中返回的具体消息句柄
 * @in param waitCmd : 需要等待的命令的消息，一直等待的就是该命令的消息
 *
 * */
int OS_mbxWaitCmd(OS_MbxHndl *pHndl, OS_MsgHndl **pMsg, Uint16 waitCmd);

/*
 * 清空邮箱队列的消息
 * @in param pHndl : 邮箱句柄
 *
 * */
int OS_mbxFlush(OS_MbxHndl *pHndl);

#ifdef __cplusplus
}
#endif
#endif /* _OSA_MBX_H_ */



