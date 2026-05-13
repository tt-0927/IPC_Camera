#ifndef _DESKTOP_INI_FILE_H_
#define _DEKSTOP_INI_FILE_H_
 
#ifdef __cplusplus
extern "C"
{
#endif
 
#define  REDA_INI_SUCCESS  1
#define  REDA_INI_FAIL     0

int ini_read_profile_char( const char *section, const char *key,char *value, int size,const char *default_value, const char *file);
int ini_read_profile_int( const char *section, const char *key,int *nValue ,int default_value,const char *file);
int ini_write_profile_char( const char *section, const char *key,const char *value, const char *file);
int ini_write_profile_int(const char *section, const char *key, int value, const char *file);  
  
#ifdef __cplusplus
}; //end of extern "C" {
#endif
 
#endif //end of INI_FILE_H_
