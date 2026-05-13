

/*
 * 任务模型
 * 有前台后台任务处理线程
 * 例如：前台任务主要负责接收外界发来的消息，后台负责处理消息
 *
 * */



#ifndef _OS_TSK_H_
#define _OS_TSK_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "os_thr.h"
#include "os_mbx.h"

struct OS_TskHndl;


/*
 * 任务处理的回调函数
 * pTsk: 任务处理句柄
 * pMsg: 具体消息
 * curStat: 任务的状态，用户自定义使用该字段
 *
 * */
typedef int (*OS_TskFncMain)(struct OS_TskHndl *pTsk, OS_MsgHndl *pMsg, Uint32 curState );


/**
  \brief Task Handle
*/
typedef struct OS_TskHndl {

  OS_MbxHndl mbxHndl;      ///< Mailbox handle
  OS_ThrHndl thrHndl;      ///< OS thread handle

  Uint32 curState;          ///< Task state as defined by user
  OS_TskFncMain fncMain;   ///< Task Main, this function is entered when a message is received by the process
  
  void *appData;
    
} OS_TskHndl;


/*
 * 创建一个任务事件
 * @in param pTsk : 任务事件的句柄
 * @in param fncMain : 任务处理的回调函数
 * @in param tskPri : 任务线程是分离线程
 * @in param tskStackSize : 任务线程的堆栈大小
 * @in param initState : 任务事件的初始化状态
 * @in param appData :
 *
 * */
int OS_tskCreate(OS_TskHndl *pTsk, OS_TskFncMain fncMain, Uint32 tskPri, Uint32 tskStackSize, Uint32 initState,
                  void *appData);

/*
 * 删除一个任务事件
 * @in param pTsk : 任务事件的句柄
 *
 * */
int OS_tskDelete(OS_TskHndl *pTsk);

/*
 * 发送一个任务到任务事件处理
 * @in param pTskTo : 待处理的任务事件句柄
 * @in param ptskFrom : 已处理完后的任务事件句柄，返回给发送者的任务句柄，如果不需要任务处理后的消息，则可不使用该字段，置NULL即可
 * @in param cmd : 处理该任务的命令
 * @in param pPrm : 该任务里面的具体消息内容
 * @in param flags : 是否需要阻塞等待消息处理后返回的消息，取值OS_MBX_WAIT_ACK即阻塞等待，否则不等待
 * @out param return : 消息处理好之后返回的ACK信号
 *
 * */
int OS_tskSendMsg(OS_TskHndl *pTskTo, OS_TskHndl *pTskFrom, Uint16 cmd, void *pPrm, Uint16 flags);

/*
 * 把要处理的任务广播到所有的任务事件里面
 * @in param pTskToList : 该处理的任务事件句柄
 * @in param : 同上
 *
 * */
int OS_tskBroadcastMsg(OS_TskHndl *pTskToList[], OS_TskHndl *pTskFrom, Uint16 cmd, void *pPrm, Uint16 flags);

/*
 * 根据发送者对flag的置位情况是返回ACK信号还是销毁消息释放内存空间
 * @in param pMsg : 从队列中获取到的具体消息句柄
 * @in param ackRetVal : 若发送者需要接收者返回ACK应答信号，则该字段表示返回给发送者的ACK应答字段
 *
 * */
int OS_tskAckOrFreeMsg(OS_MsgHndl *pMsg, int ackRetVal);

/*
 * 等待任务
 * @in param pTsk : 任务事件句柄
 * @in param pMsg : 从邮箱消息队列中返回的具体消息句柄
 *
 * */
int OS_tskWaitMsg(OS_TskHndl *pTsk, OS_MsgHndl **pMsg);

/*
 *  等待接收返回者返回的ACK应答信号值
 *  @in param pTsk : 任务事件句柄
 *  @in param pMsg : 返回的具体消息句柄
 *  @out param return : 返回的ACK值
 *
 * */
int OS_tskCheckMsg(OS_TskHndl *pTsk, OS_MsgHndl **pMsg);

/*
 * 一直等待到自己需要的命令的任务
 * @in param pTsk : 任务事件句柄
 * @in param pMsg : 返回的具体消息句柄
 * @in param waitCmd : 需要等待的任务的消息，一直等待的就是该命令的任务
 *
 * */
int OS_tskWaitCmd(OS_TskHndl *pTsk, OS_MsgHndl **pMsg, Uint16 waitCmd);

/*
 * 清空任务事件中所有的事件
 * @in param pTsk : 任务事件句柄
 *
 * */
int OS_tskFlushMsg(OS_TskHndl *pTsk);

/*
 * 设置该任务事件的状态
 * @in param pPrc : 任务事件句柄
 * @in param curState : 任务事件的状态
 *
 * */
int OS_tskSetState(OS_TskHndl *pPrc, Uint32 curState);

/*
 * 获取该任务事件的状态
 * @in param pPrc : 任务事件的句柄
 * @out param return : 返回当前任务事件的状态
 *
 * */
Uint32 OS_tskGetState(OS_TskHndl *pPrc);


#ifdef __cplusplus
}
#endif
#endif /* _OS_TSK_H_ */




