#ifndef _TIXMLMGR_H_
#define _TIXMLMGR_H_
#include<stdlib.h>
#include<stdio.h>
#include "typedef.h"
#include "errordef.h"
#include "dbgdefs.h"

/*! Type of the parsing function defined in struct tixmlmgr_parsing_map, refer to tixmlmgr_parsing.c for the default parsing functions */
typedef void (* FParsingAction) (const char *szArgs, void *pObject);

/*! TiXmlMgr parsing map (used for getting multiple configurations) */
typedef struct tixmlmgr_parsing_map
{
	/*! Config name to be parsed */
	const char *szName;
	/*! Corresponding parsing action  */
	FParsingAction pfnParsingAction;
	/*! Output to pObject */
	void *pObject;
} TTiXmlMgrParsingMap;

typedef SCODE (* TIXMLMGR_StartElemHandler) (void *userData, const CHAR* s, const CHAR** atts);
typedef void  (* TIXMLMGR_CharacterDataHandler)(void *userData, const CHAR *s, SDWORD len);
typedef void  (* TIXMLMGR_EndElementHandler) (void *userData, const CHAR *name);

/*! \b TiXmlMgr tree map (used for construct xmltree and handler) */
typedef struct tixmlmgr_treemap  
{ 
	/*! Config name to be parsed */
    char            *szElemName; 
	/*! start element call back function  */
    TIXMLMGR_StartElemHandler      pfStartHdl; 
	/*! charater data call back function  */
    TIXMLMGR_CharacterDataHandler  pfChDataHdl;   
	/*! end element call back function  */
    TIXMLMGR_EndElementHandler     pfEndHdl; 
} TTiXmlMgrTreeMap; 

#ifdef __cplusplus
extern "C" {
#endif

void xml_set_string(const char *szConfValue, void *pObj);

void xml_set_long(const char *szConfValue, void *pObj);

void xml_set_short(const char *szConfValue, void *pObj);

void xml_set_int(const char *szConfValue, void *pObj);

/*!
* Read XML file and test the write_flag 
* if the write_flag is begin ,use the "_bak" XML to replace it
*/


// just for sysmgr,use this to replace file with a temp file
/*@替换xml文件
 *@源文件 目的文件
 *@失败返回非0值
 */

SCODE TiXmlMgr_ReplaceFile(const char *szFileSrc, const char *szFileDst);

/*@获取单个的xml节点值
 *@文件名    节点目录   节点值
 *@失败返回非0值
 */

SCODE TiXmlMgr_GetConfValue(const char *szFileName, const char *szConfName, char *szConfValue);

/*@获取多个节点值
 *@文件名    节点结构体
 *@失败返回非0值
 */


SCODE TiXmlMgr_GetMultiConfValue(const char *szFileName, TTiXmlMgrParsingMap *ptParsingMap);

/*@设置单个节点值
 *@文件名    节点目录 节点值
 *@失败返回非0值
 */


SCODE TiXmlMgr_SetConfValue(const char *szFileName, const char *szConfName, const char *szConfValue);

/*@设置多个节点值
 *@文件名    节点数  节点数组 获取到的节点值数组 
 *@失败返回非0值
 */


SCODE TiXmlMgr_SetMultiConfValue(const char *szFileName, int iConfNum, const char *szConfNames[], const char *szConfValues[]);


#ifdef __cplusplus
}
#endif

#endif /* _TIXMLMGR_H_ */
/*! \example Demo_app.c
 *  This is a example of how to use this library.
 */

