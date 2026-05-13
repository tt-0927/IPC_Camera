

#include "os_cond.h"
#define OS_SOK      0  ///< Status : OK
#define OS_EFAIL   -1  ///< Status : Generic error

int OS_condCreate(OS_CondHndl *hndl)
{
  pthread_mutexattr_t mutex_attr;
  pthread_condattr_t cond_attr;
  int status = OS_SOK;

  status |= pthread_mutexattr_init(&mutex_attr);
  status |= pthread_condattr_init(&cond_attr);

  status |= pthread_mutex_init(&hndl->lock, &mutex_attr);
  status |= pthread_cond_init(&hndl->cond, &cond_attr);

  pthread_condattr_destroy(&cond_attr);
  pthread_mutexattr_destroy(&mutex_attr);

  return status;
}

int OS_condWait(OS_CondHndl *hndl)
{
  int status = OS_SOK;

  pthread_mutex_lock(&hndl->lock);
  pthread_cond_wait(&hndl->cond, &hndl->lock);
  pthread_mutex_unlock(&hndl->lock);

  return status;
}

int OS_condSignal(OS_CondHndl *hndl)
{
  int status = OS_SOK;

  pthread_mutex_lock(&hndl->lock);
  status |= pthread_cond_signal(&hndl->cond);
  pthread_mutex_unlock(&hndl->lock);

  return status;
}

int OS_condDelete(OS_CondHndl *hndl)
{
  pthread_cond_destroy(&hndl->cond);
  pthread_mutex_destroy(&hndl->lock);

  return OS_SOK;
}


