
#ifndef MALLOC_CHECK_MEMORY_LEAK_H_
#define MALLOC_CHECK_MEMORY_LEAK_H_


#ifdef __cplusplus
extern "C" {
#endif
#define manag_malloc(length) memory_leak_malloc(length, __FILE__, __func__, __LINE__)
#define manag_free(addr) memory_leak_free(addr, __FILE__, __func__, __LINE__)
void* memory_leak_malloc(int length, const char * filename,const char * function, int linenum);

void memory_leak_free(void* addr, const char * filename,const char * function, int linenum);
#ifdef __cplusplus
}
#endif


#endif /* MALLOC_CHECK_MEMORY_LEAK_H_ */
