

#ifndef _OS_COND_H_
#define _OS_COND_H_

#ifdef __cplusplus
extern "C" {
#endif
#include "os.h"

typedef struct {

  pthread_mutex_t lock;
  pthread_cond_t  cond;

} OS_CondHndl;


// cond
int OS_condCreate(OS_CondHndl *hndl);
int OS_condWait(OS_CondHndl *hndl);
int OS_condSignal(OS_CondHndl *hndl);
int OS_condDelete(OS_CondHndl *hndl);

#ifdef __cplusplus
}
#endif

#endif /* _OS_COND_H_ */


