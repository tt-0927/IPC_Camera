
#include "os_mutex.h"
#include "os_debug.h"

int OS_mutexCreate(OS_MutexHndl *hndl)
{
  pthread_mutexattr_t mutex_attr;
  int status=OS_SOK;
 
  status |= pthread_mutexattr_init(&mutex_attr);
  status |= pthread_mutex_init(&hndl->lock, &mutex_attr);
  
  if(status!=OS_SOK)
    OS_ERROR("OS_mutexCreate() = %d \r\n", status);

  pthread_mutexattr_destroy(&mutex_attr);
    
  return status;
}

int OS_mutexDelete(OS_MutexHndl *hndl)
{
  pthread_mutex_destroy(&hndl->lock);  

  return OS_SOK;
}

int OS_mutexLock(OS_MutexHndl *hndl)
{
  return pthread_mutex_lock(&hndl->lock);
}

int OS_mutexUnlock(OS_MutexHndl *hndl)
{
  return pthread_mutex_unlock(&hndl->lock);
}


