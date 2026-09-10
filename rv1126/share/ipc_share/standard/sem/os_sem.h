

#ifndef _OS_SEM_H_
#define _OS_SEM_H_
#include<stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
#include"pthread.h"
#include<stdio.h>
#include<string.h>
#include<stdint.h>

typedef struct {

  uint32_t count;
  uint32_t maxCount;
  pthread_mutex_t lock;
  pthread_cond_t  cond;

} OS_SemHndl;


// sem
int OS_semCreate(OS_SemHndl *hndl, uint32_t maxCount, uint32_t initVal);
int OS_semWait(OS_SemHndl *hndl, int32_t timeout, uint32_t *remainVal);
int OS_semWaitAll( OS_SemHndl *hndl );
int OS_semSignal(OS_SemHndl *hndl);
int OS_semBroad(OS_SemHndl *hndl);
int OS_semDelete(OS_SemHndl *hndl);

#ifdef __cplusplus
}
#endif
#endif /* _OS_FLG_H_ */



