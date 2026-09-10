

#ifndef _OSA_QUE_H_
#define _OSA_QUE_H_

#include "os.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef struct {

  Uint32 curRd;
  Uint32 curWr;
  Uint32 len;
  Uint32 count;

  Int64 *queue;

  pthread_mutex_t lock;
  pthread_cond_t  condRd;
  pthread_cond_t  condWr;
  
} OS_QueHndl;

int OS_queCreate(OS_QueHndl *hndl, Uint32 maxLen);
int OS_queDelete(OS_QueHndl *hndl);
int OS_queCopy(OS_QueHndl *hndl,OS_QueHndl *cphndl);
int OS_quePut(OS_QueHndl *hndl, Int64  value, Int32 timeout);
int OS_queGet(OS_QueHndl *hndl, Int64 *value, Int32 timeout);
int OS_quePeek(OS_QueHndl *hndl, Int64 *value);
Uint32 OS_queGetQueuedCount(OS_QueHndl *hndl);
int OS_queIsEmpty(OS_QueHndl *hndl);

int OS_queIsFull(OS_QueHndl *hndl);

#ifdef __cplusplus
}
#endif

#endif /* _OSA_QUE_H_ */



