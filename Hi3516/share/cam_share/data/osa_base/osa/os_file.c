
#include <stdio.h>
#include "os_file.h"
#include "os_debug.h"

#define OS_DEBUG_FILE

int OS_fileReadFile(char *fileName, Uint8 *addr, Uint32 readSize, Uint32 *actualReadSize)
{
  int retVal = OS_SOK;
  Uint8  *curAddr;

  Uint32 readDataSize, fileSize, chunkSize=6221760;
  Uint32 userReadSize;
  FILE *hndlFile = NULL;

  #ifdef OS_DEBUG_FILE
  OS_printf(" [FILE ] Reading file [%s] ... ", fileName);
  #endif

  hndlFile = fopen(fileName, "rb");

  if(hndlFile == NULL) {
  	retVal = OS_EFAIL;
    goto exit;
	}

  curAddr = addr;
  fileSize = 0;

	userReadSize = readSize;

  while(1) {
		if(userReadSize != 0) {
			if(chunkSize > userReadSize)
				chunkSize = userReadSize;
			readDataSize = fread(curAddr, 1, chunkSize, hndlFile);
			fileSize += readDataSize;
			if(chunkSize != readDataSize)
				break;
			if(userReadSize==fileSize)
				break;
			curAddr += chunkSize;
		}
		else {
			readDataSize = fread(curAddr, 1, chunkSize, hndlFile);
      fileSize+=readDataSize;
      if(chunkSize!=readDataSize)
        break;
      curAddr+=chunkSize;
    }
	}
  #ifdef OS_DEBUG_FILE
  OS_printf("Done. [%d bytes] \r\n", fileSize);
  #endif
  fclose(hndlFile);

exit:
  if(retVal!=OS_SOK) {
    #ifdef OS_DEBUG_FILE
    OS_printf("ERROR \r\n");
    #endif
    fileSize=0;
  }
	if(actualReadSize != NULL)
    *actualReadSize = fileSize;

  return retVal;
}

int OS_fileWriteFile(char *fileName, Uint8 *addr, Uint32 size, const char *mode)
{
  int retVal = OS_SOK;
  Uint32 writeDataSize = 0;

  //Bool errorInWriting = FALSE;
  FILE *hndlFile = NULL;

  if(size==0)
  {
    return OS_SOK;
  }

  #ifdef OS_DEBUG_FILE
  //OS_printf(" [FILE ] Writing to file [%s] (%d bytes) ... ", fileName, size);
  #endif
  hndlFile = fopen(fileName, mode);

  if(hndlFile == NULL) {
  	retVal = OS_EFAIL;
    goto exit;
	}

  {
    // write in units of chunkSize
    Int32 fileSize = 0,chunkSize = 96*1024;
    Int8  *curAddr;


    fileSize = size;
    curAddr  = (Int8*)addr;
    while(fileSize>0) {
      if(fileSize<chunkSize) {
        chunkSize = fileSize;
      }
      writeDataSize=0;
      writeDataSize = fwrite(curAddr, 1, chunkSize, hndlFile);
      if(writeDataSize != (Uint32)chunkSize) {
        // error in writing, abort
        //errorInWriting = TRUE;
        retVal = OS_EFAIL;
        break;
      }
      curAddr += chunkSize;
      fileSize -= chunkSize;
    }
    writeDataSize = size - fileSize;
  }

  #ifdef OS_DEBUG_FILE
 // OS_printf("Done. [%d bytes] \r\n", writeDataSize);
  #endif
  fflush(hndlFile);

  fclose(hndlFile);

exit:
  if(retVal!=OS_SOK) {
    #ifdef OS_DEBUG_FILE
    OS_printf("ERROR \r\n");
    #endif
  }
  return retVal;

}

