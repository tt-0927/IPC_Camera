

#ifndef _OSA_QUE_H_
#define _OSA_QUE_H_
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>

//#include "os.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef struct {

  uint32_t curRd;
  uint32_t curWr;
  uint32_t len;
  uint32_t count;

  int64_t *queue;

  pthread_mutex_t lock;
  pthread_cond_t  condRd;
  pthread_cond_t  condWr;
  
} OS_QueHndl;

int OS_queCreate(OS_QueHndl *hndl, uint32_t maxLen);
int OS_queDelete(OS_QueHndl *hndl);
int OS_queCopy(OS_QueHndl *hndl,OS_QueHndl *cphndl);
int OS_quePut(OS_QueHndl *hndl, int64_t  value, int32_t timeout);
int OS_queGet(OS_QueHndl *hndl, int64_t *value, int32_t timeout);
int OS_quePeek(OS_QueHndl *hndl, int64_t *value);
uint32_t OS_queGetQueuedCount(OS_QueHndl *hndl);
int OS_queIsEmpty(OS_QueHndl *hndl);

int OS_queIsFull(OS_QueHndl *hndl);

#ifdef __cplusplus
}
#endif

#endif /* _OSA_QUE_H_ */



