/*** 
 * @FilePath     : os_hashCode.h
 * @Author       : huangjunda
 * @Date         : 2025-04-17 21:14:42
 * @LastEditors  : huangjunda
 * @LastEditTime : 2025-04-17 21:14:44
 * @Description  : 
 */

#ifndef _OS_API_CORE_SOURCE_STRING_HASH_INCLUDE_
#define _OS_API_CORE_SOURCE_STRING_HASH_INCLUDE_


/*
 *	各个常用的字符串hash算法比较
 * 	Hash函数		数据1		数据2		数据3		数据4		数据1得分	数据2得分	数据3得分	数据4得分	平均分
	BKDRHash 	2		0		4774	481		96.55	100		90.95	82.05	92.64
	APHash		2		3		4754	493		96.55	88.46	100		51.28	86.28
	DJBHash		2		2		4975	474		96.55	92.31	0		100		83.43
	JSHash		1		4		4761	506		100		84.62	96.83	17.95	81.94
	RSHash		1		0		4861	505		100		100		51.58	20.51	75.96
	SDBMHash	3		2		4849	504		93.1	92.31	57.01	23.08	72.41
	PJWHash		30		26		4878	513		0		0		43.89	0		21.95
	ELFHash		30		26		4878	513		0		0		43.89	0		21.95
 *
 * 其中数据1为100000个字母和数字组成的随机串哈希冲突个数。数据2为100000个有意义的英文句子哈希冲突个数。
 * 数据3为数据1的哈希值与1000003(大素数)求模后存储到线性表中冲突的个数。数据4为数据1的哈希值与10000019(更大素数)求模后存储到线性表中冲突的个数。
 *
 * */

unsigned int RSHash(char* str, unsigned int len);
/* End Of RS Hash Function */


unsigned int JSHash(char* str, unsigned int len);
/* End Of JS Hash Function */


unsigned int PJWHash(char* str, unsigned int len);
/* End Of  P. J. Weinberger Hash Function */


unsigned int ELFHash(char* str, unsigned int len);
/* End Of ELF Hash Function */


unsigned int BKDRHash(char* str, unsigned int len);
/* End Of BKDR Hash Function */


unsigned int SDBMHash(char* str, unsigned int len);
/* End Of SDBM Hash Function */


unsigned int DJBHash(char* str, unsigned int len);
/* End Of DJB Hash Function */


unsigned int DEKHash(char* str, unsigned int len);
/* End Of DEK Hash Function */


unsigned int BPHash(char* str, unsigned int len);
/* End Of BP Hash Function */


unsigned int FNVHash(char* str, unsigned int len);
/* End Of FNV Hash Function */


unsigned int APHash(char* str, unsigned int len);
/* End Of AP Hash Function */



#endif //_OS_API_CORE_SOURCE_STRING_HASH_INCLUDE_






