
#include "os_tsk.h"
#include "os_debug.h"

void *OS_tskThrMain(void *pPrm)
{
	OS_MsgHndl *pMsg = NULL;
	OS_TskHndl *pPrc = NULL;

	pPrc = (OS_TskHndl *)pPrm;
	OS_assert(pPrc != NULL);

	while(1)
	{
		OS_assertSuccess(OS_tskWaitMsg(pPrc, &pMsg));
		OS_assert(pPrc->fncMain!=NULL);
		pPrc->fncMain(pPrc, pMsg, pPrc->curState);
	}

	return NULL;
}

int OS_tskCreate(OS_TskHndl *pPrc, OS_TskFncMain fncMain, Uint32 tskPri, Uint32 tskStackSize, Uint32 initState,
                  void *appData)
{
	pPrc->curState      = initState;
	pPrc->fncMain       = fncMain;
	pPrc->appData       = appData;

	OS_assert(pPrc->fncMain != NULL);

	OS_mbxCreate(&pPrc->mbxHndl);

	OS_assertSuccess(OS_thrCreate(&pPrc->thrHndl, OS_tskThrMain, tskPri, tskStackSize,  pPrc));

	return OS_SOK;
}

int OS_tskDelete(OS_TskHndl *pPrc)
{
	OS_printf("%s:In progress...",__func__);
	OS_thrDelete(&pPrc->thrHndl);
	OS_mbxDelete(&pPrc->mbxHndl);

	pPrc->curState      = 0;
	pPrc->fncMain       = NULL;

 	return OS_SOK;
}

int OS_tskSendMsg(OS_TskHndl *pPrcTo, OS_TskHndl *pPrcFrom, Uint16 cmd, void *pPrm, Uint16 flags)
{
	int retVal;
	OS_MbxHndl *pMbxFrom;

	OS_assert(pPrcTo!=NULL);

	if (pPrcFrom==NULL)
	{
		pMbxFrom = NULL;
	} 
	else
	{
		pMbxFrom = &pPrcFrom->mbxHndl;
	}

	retVal = OS_mbxSendMsg(&pPrcTo->mbxHndl, pMbxFrom, cmd, pPrm, flags);

	return retVal;
}

int OS_tskBroadcastMsg(OS_TskHndl *pPrcToList[], OS_TskHndl *pPrcFrom, Uint16 cmd, void *pPrm, Uint16 flags)
{
	int retVal;

	OS_MbxHndl *pMbxToList[OS_MBX_BROADCAST_MAX];
	Uint32 i, numMsg;
	OS_MbxHndl *pMbxFrom;

	OS_assert(pPrcToList!=NULL);

	if (pPrcFrom==NULL) 
	{
		pMbxFrom = NULL;
	} 
	else
	{
		pMbxFrom = &pPrcFrom->mbxHndl;
	}

	for	(i = 0; i<OS_MBX_BROADCAST_MAX; i++)
	{
		pMbxToList[i]=NULL;
	}

	numMsg = 0;
	while (pPrcToList[numMsg]!=NULL) 
	{
		pMbxToList[numMsg] = &pPrcToList[numMsg]->mbxHndl;
		numMsg++;
		if (numMsg>=OS_MBX_BROADCAST_MAX)
		{
	  // cannot broadcast to more than OS_MBX_BROADCAST_MAX mailboxes
			OS_assert(0);
		}
	}

	if (numMsg == 0) 
	{
	 // no mailboxes in 'to' mailbox list
		return OS_SOK;
	}

	retVal = OS_mbxBroadcastMsg(&pMbxToList[0], pMbxFrom, cmd, pPrm, flags);

	return retVal;
}


int OS_tskWaitMsg(OS_TskHndl *pPrc, OS_MsgHndl **pMsg)
{
	int retVal;

	retVal = OS_mbxWaitMsg(&pPrc->mbxHndl, pMsg);

	return retVal;
}

int OS_tskCheckMsg(OS_TskHndl *pPrc, OS_MsgHndl **pMsg)
{
	int retVal;

	retVal = OS_mbxCheckMsg(&pPrc->mbxHndl, pMsg);

	return retVal;
}

int OS_tskAckOrFreeMsg(OS_MsgHndl *pMsg, int ackRetVal)
{
	int retVal;

	retVal = OS_mbxAckOrFreeMsg(pMsg, ackRetVal);

	return retVal;
}

int OS_tskFlushMsg(OS_TskHndl *pPrc)
{
	int retVal;

	retVal = OS_mbxFlush(&pPrc->mbxHndl);

	return retVal;
}

int OS_tskWaitCmd(OS_TskHndl *pPrc, OS_MsgHndl **pMsg, Uint16 waitCmd)
{
	int retVal;

	retVal = OS_mbxWaitCmd(&pPrc->mbxHndl, pMsg, waitCmd);

	return retVal;
}

int OS_tskSetState(OS_TskHndl *pPrc, Uint32 curState)
{
	pPrc->curState = curState;
	return OS_SOK;
}

Uint32 OS_tskGetState(OS_TskHndl *pPrc)
{
	return pPrc->curState;
}

