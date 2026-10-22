/*
 * @Description: 
 * @Version: 1.0
 * @Autor: Ducr
 * @Date: 2023-02-17 15:40:50
 * @LastEditors: yangwenyao
 * @LastEditTime: 2023-04-23 10:24:27
 */
#ifdef  __cplusplus
extern "C" {
#endif

/*unicode转utf8
return:  成功：0，失败：负数
*/
int  unicode_to_utf8(unsigned int unicode, unsigned char *byte, unsigned char *pCount);
/*utf8转unicode
return:  成功：0，失败：负数*/
int  utf8_to_unicode(unsigned char  *byte, int index, int count, unsigned short* unicode);

/*
func: 字符串gb2312转utf8
return:  成功：0，失败：负数
*/
int gb2312_to_utf8(const char* gb2312, int gb_len, char *utf8, int utf_bufSize);
/*
func: 字符串utf8转gb2312
return:  成功：0，失败：负数
*/
int utf8_to_gb2312(const char* utf8, int len, char *temp);
#ifdef  __cplusplus
}
#endif