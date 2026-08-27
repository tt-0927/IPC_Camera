

#ifndef _OS_MUTEX_H_
#define _OS_MUTEX_H_

#ifdef __cplusplus
extern "C" {
#endif


#ifdef WIN32
#include "pthread.h"
#else
#include <pthread.h>
#endif

typedef struct {

  pthread_mutex_t lock;
  
} OS_MutexHndl;


//#include "os.h"


int OS_mutexCreate(OS_MutexHndl *hndl);
int OS_mutexDelete(OS_MutexHndl *hndl);
int OS_mutexTrylock(OS_MutexHndl *hndl);
int OS_mutexLock(OS_MutexHndl *hndl);
int OS_mutexUnlock(OS_MutexHndl *hndl);

#ifdef __cplusplus
}
#endif
#endif /* _OSA_MUTEX_H_ */



