

#ifndef _OSA_FILE_H_
#define _OSA_FILE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "os.h"

int OS_fileReadFile(char *fileName, Uint8 *addr, Uint32 readSize, Uint32 *actualReadSize);
int OS_fileWriteFile(char *fileName, Uint8 *addr, Uint32 size, const char *mode);

#ifdef __cplusplus
}
#endif
#endif /* _OSA_FILE_H_ */



