

#ifndef _OS_SEM_H_
#define _OS_SEM_H_

#ifdef __cplusplus
extern "C" {
#endif
#include "os.h"

typedef struct {

  Uint32 count;
  Uint32 maxCount;
  pthread_mutex_t lock;
  pthread_cond_t  cond;

} OS_SemHndl;


// sem
int OS_semCreate(OS_SemHndl *hndl, Uint32 maxCount, Uint32 initVal);
int OS_semWait(OS_SemHndl *hndl, Int32 timeout, Uint32 *remainVal);
int OS_semSignal(OS_SemHndl *hndl);
int OS_semDelete(OS_SemHndl *hndl);

#ifdef __cplusplus
}
#endif
#endif /* _OS_FLG_H_ */



