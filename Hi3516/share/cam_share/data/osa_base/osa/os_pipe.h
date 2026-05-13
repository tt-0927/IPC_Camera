

#ifndef _OS_PIPE_H_
#define _OS_PIPE_H_

#ifdef __cplusplus
extern "C" {
#endif
#include "os.h"

int OS_CreatePipe (OS_PTR *pPipe, unsigned long  pipeSize,
                                          unsigned long  messageSize,
                                          unsigned char  isFixedMessage) ;
int OS_DeletePipe (OS_PTR pPipe) ;
int OS_WriteToPipe (OS_PTR pPipe, void *pMessage,
                                           unsigned long size,
                                           unsigned long timeout) ;
int OS_ReadFromPipe (OS_PTR pPipe, void *pMessage,
                                            unsigned long size,
                                            unsigned long *actualSize,
                                            long timeout) ;


#ifdef __cplusplus
}
#endif

#endif /* _OS_PIPE_H_ */

