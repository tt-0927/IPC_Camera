
#ifndef __XML_BASE__
#define __XML_BASE__
#ifdef __cplusplus
extern "C"
{
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "xml_define.h"
#include <time.h>

#include "public_define.h"




typedef struct _XML_SPECIAL_CHARACTER__
{
	char *xmlOld;
	char *xmlNew;
}xmlSpecialCharacter;















/**
*@读取xml文件
*@param[in] DoCHandle,xml句柄
*@param[in] 成功返回文件句柄,失败返回NULL
*/
Xml_DocPtr_t xml_read_file(const char *fileName ,const char *encoding);

/**
*@不能在线程中被调用,因为xmlInit()不是原子操作,可能会引起线程竞争,导致程序意外.，最好在程序开始时（1次）
*/
void xml_init();
/**
*@不能在线程中被调用,因为先结束的进程会把共享内存清除,接下来尚未结束的的线程就无法正确访问，最好在程序结束后（1次）
*/
void xml_unit();
//注意：所有节点操作都得先基于句柄*******************************************
//最后退出时需销毁句柄 Xml_DocPtr_t DoCHandle

/**
*@句柄销毁
*@param[in] DoCHandle,xml句柄
*@param[in] 成功返回TRUE,失败返回FALSE
*/
BOOL xml_free_docHandle(Xml_DocPtr_t DoCHandle);

/**
*@内存销毁
*@param[in] pBuf内存
*@param[in] 成功返回TRUE,失败返回FALSE
*/
BOOL xml_free_baseBuf(char *pBuf);

//文件相关操作--------------------------------------------------------


/**
*@ref 基于DocHandle句柄把内存中数据保存到文件
*@param[in] cszFileFullPath XML文件输出全路径
*@param[in] encoding编码格式,例如（"UTF-8"）当为空时会默认编码（"UTF-8"）
*@return 成功返回读人文件的字节数,失败返回负数
*/
int xml_save_handleToFile(const char* cszFileFullPath, Xml_DocPtr_t DocHandle, const char *encoding);	// 保存文件
/**
*@ref 基于xml字符串把内存中数据保存到文件
*@param[in] cszFileFullPath XML文件输出全路径
*@param[in] encoding编码格式,例如（"UTF-8"）当为空时会默认编码（"UTF-8"）
*@param[in] pXmlBuf字符串内容
*@return 成功返回读人文件的字节数,失败返回负数
*/
int xml_save_strToFile(const char* cszFileFullPath, const char *pXmlBuf, const char *encoding);	// 保存文件
///**
//*@ref 得到XML的长度,该函数慎用，因为有些中文字符有格式
//*@ref *@param[out]XML的长度
//*@return 返回值xml_Success为成功，其他失败
//*/
//RetErr_t  xml_get_fileLen(const char* cszFileFullPath, int* nLen);
/**
*@ref 从硬盘中得到一个XML格式的数据
*@param[out] pszBuf 返回XML的内容，大小要自己申请，可先获取长度（若为空，则不获取内容）
*@param[in] nBufLength pszBuf指向内存的大小
*@param[in] encoding编码格式例如（"UTF-8"）
*@return 成功返回文档句柄,失败返回空
*/
Xml_DocPtr_t xml_get_file(const char* FileFullPath, const char *encoding, char* pszBuf);



//节点的添加设置--------------------------------------------------------

/*@ref xml初始化文档
 *@#param[in] version 版本号（如1.0）
 *@成功返回文档句柄,失败返回空
 */
Xml_DocPtr_t xml_init_doc();


/**
*@ref 添加根节点
*@param[in] XmlDocHandle xml文档的句柄
*@param[in] nodeChildName 要添加的根节点的名字
*@return 成功返回根节点
*/
Xml_NodePtr_t xml_append_rootNode(Xml_DocPtr_t XmlDocHandle, const char* nodeChildName);
/**
*@ref 添加节点
*@param[in] nodeParent 父节点
*@param[in] nodeChildName 要添加的节点的名字
*@return 成功返回该子节点
*/
Xml_NodePtr_t xml_append_childNode(Xml_NodePtr_t nodeParent,  const char* nodeChildName);

/**
*@ref 设置节点的字符串值
*@param[in] nodeDest 节点的指针
*@param[in] cszText 字符串值
*@return 成功返回TRUE，失败返回FALSE
*/
BOOL xml_set_charNode(Xml_NodePtr_t nodeDest, const char* cszText);
/**
*@ref 设置节点的整型值
*@param[in] nodeDest 节点的指针
*@param[in] thevalue 整型值
*@return 成功返回TRUE，失败返回FALSE
*/
BOOL xml_set_intNode(Xml_NodePtr_t nodeDest, int thevalue);

/**
*@ref 设置节点的整型值
*@param[in] nodeDest 节点的指针
*@param[in] thevalue 整型值
*@return 成功返回TRUE，失败返回FALSE
*/
BOOL xml_set_uintNode(Xml_NodePtr_t nodeDest,unsigned int thevalue);
/**
*@ref 设置节点的浮点型值
*@param[in] nodeDest 节点的指针
*@param[in] thevalue 浮点型值
*@return 成功返回TRUE，失败返回FALSE
*/
BOOL xml_set_doubleNode(Xml_NodePtr_t nodeDest, double thevalue);

/**
*@ref 设置节点的浮点型值
*@param[in] nodeDest 节点的指针
*@param[in] thevalue 浮点型值
*@return 成功返回TRUE，失败返回FALSE
*/
BOOL xml_set_floatNode(Xml_NodePtr_t nodeDest, float thevalue);
/**
*@ref 设置节点的时间型值
*@param[in] nodeDest 节点的指针
*@param[in] thevalue 时间型值
*@return 成功返回TRUE，失败返回FALSE
*/
//时间格式为2015-03-15 12:35:02或者12:35:00
BOOL xml_set_timeNode(Xml_NodePtr_t nodeDest, ScSystemTime thevalue);

//节点的删除查询获取-----------------------------------------------------

/**
*@ref 删除节点
*@param[in] nodeRemoving 要删除的节点
*@return 成功返回下一个节点，失败返回空
*/
Xml_NodePtr_t xml_remove_node(Xml_NodePtr_t nodeRemoving);

/**
*@ref 删除节点
*@param[in] nodeRemoving 要删除的节点
*@param[in] fileName 文件名
*@return 成功返回TRUE失败返回FALSE
*/
BOOL xml_remove_node1(const char * nodeName, char *fileName);
/**
*@ref 获得根节点
*@param[in] DocHandle, xml的句柄
*@return 有字节点返回根节点，失败返回空
*/
Xml_NodePtr_t xml_get_rootNode(Xml_DocPtr_t DocHandle);
/**
*@ref 取得第一个子节点的指针
*@param[in] nodeParent 父节点
*@return 成功返回第一个子节点的指针，失败返回空
*/
Xml_NodePtr_t xml_get_firstChildNode(Xml_NodePtr_t nodeParent);
/**
*@ref 取得前一个兄弟节点的指针
*@param[in] nodeFrom 节点
*@return 成功返回前一个兄弟节点的指针，失败返回空
*/
Xml_NodePtr_t xml_get_prevSiblingNode(Xml_NodePtr_t node);
/**
*@ref 取得下一个兄弟节点的指针
*@param[in] nodeFrom 节点
*@return 成功返回下一个兄弟节点的指针，失败返回空
*/
Xml_NodePtr_t xml_get_nextSiblingNode(Xml_NodePtr_t node);
/**
*@ref 获取节点
*@param[in] nodeParent 父节点
*@param[in] 字段名字Key
*@return 成功返回节点名字的指针，失败返回空
*/
Xml_NodePtr_t xml_get_childNode(Xml_NodePtr_t nodeParent, const char *key);


/**
*@ref获取节点名字相同的兄弟节点
*@param[in] nodeParent 父节点
*@param[in] 字段名字Key
*@return 成功返回节点名字的指针，失败返回空
*/
Xml_NodePtr_t xml_get_sameNameBroNode(Xml_NodePtr_t nodecur, const char *key);




//节点名字操作----------------------------------------------------------------
/**
*@ref 获取节点的名字
*@param[in] nodeParent 当前节点
*@return 成功返回节点名
*/
char * xml_get_nodeName(Xml_NodePtr_t node);

/**
*@ref 设置节点的名字
*@param[in] nodeParent 当前节点
*@return 成功返回节点名
*/
char * xml_set_nodeName(Xml_NodePtr_t node, const char * name);

//节点值的获取----------------------------------------------------------
/**
*@ref 对xml字符串初始化，获得句柄,关闭时注意将parse_xml的文件句柄pXmlHandle销毁
*@param[out] parse_xml 字符串的句柄，和字符串根节点
*@param[in] xml 字符串内容
*@return 成功返回TRUE，失败返回FALSE
*/
RetErr_t xml_init_str(Xml_ParseStr_t *parse_xml, const char *xml);

/**
*@ref 基于xml句柄，获得xml字符串
*@param[out] OutBuf 获得的字符串，由内部创建，由内部xml_free_baseBuf释放
*@param[in] DoCHandle xml的句柄
*@return 返回值xml_Success为成功，其他失败
*/
RetErr_t xml_handleTo_str(Xml_DocPtr_t DoCHandle,const char *encoding, char** OutBuf);
/**
*@ref 获取节点的字符串
*@param[in] nodeDest 节点的指针
*@param[in] thevalue 字符串
*@param[in] 拷贝内容的最大长度
*@return 成功返回TRUE，失败返回FALSE
*/
BOOL xml_get_charNode(Xml_NodePtr_t nodeDest, char* cszText, int nLen);
/**
*@ref 获取节点的整型值
*@param[in] nodeDest 节点的指针
*@param[in] thevalue 整型值
*@return 成功返回TRUE，失败返回FALSE
*/
BOOL xml_get_intNode(Xml_NodePtr_t nodeDest, int* thevalue);
/**
*@ref 获取节点的浮点型值
*@param[in] nodeDest 节点的指针
*@param[in] thevalue 浮点型值
*@return 成功返回TRUE，失败返回FALSE
*/
BOOL xml_get_doubleNode(Xml_NodePtr_t nodeDest, double* thevalue);

/**
*@ref 获取节点的浮点型值
*@param[in] nodeDest 节点的指针
*@param[in] thevalue 浮点型值
*@return 成功返回TRUE，失败返回FALSE
*/
BOOL xml_get_floatNode(Xml_NodePtr_t nodeDest, float* thevalue);
/**
*@ref 获取节点的时间型值
*@param[in] nodeDest 节点的指针
*@param[in] thevalue 时间型值
*@return 成功返回TRUE，失败返回FALSE
*/

//时间格式为2015-03-15 12:35:02或者12:35:00
BOOL xml_get_timeNode(Xml_NodePtr_t nodeDest, ScSystemTime* thevalue);






/**
*@ref 获取节点的字符串
*@param[in] char * nodeName节点名称，不包含根节点
*@param[in] thevalue 字符串
*@param[in] 获取内容buf的长度
*@return 成功返回TRUE，失败返回FALSE
*/
BOOL xml_get_charNode1(const char * nodeName, char* cszText, char *xmlBuf, int nLen);
/**
*@ref 获取节点的整型值
*@param[in] char * nodeName节点名称，不包含根节点
*@param[in] thevalue 整型值
*@return 成功返回TRUE，失败返回FALSE
*/
BOOL xml_get_intNode1(const char * nodeName, int* thevalue, char *xmlBuf);



/**
*@ref 获取节点的整型值
*@param[in] char * nodeName节点名称，不包含根节点
*@param[in] thevalue 整型值
*@return 成功返回TRUE，失败返回FALSE
*/
BOOL xml_get_shortNode1(const char * nodeName, short* thevalue, char *xmlBuf);
/**
*@ref 获取节点的浮点型值
*@param[in] char * nodeName节点名称，不包含根节点
*@param[in] thevalue 浮点型值
*@return 成功返回TRUE，失败返回FALSE
*/
BOOL xml_get_doubleNode1(const char * nodeName, double* thevalue, char *xmlBuf);

/**
*@ref 获取节点的浮点型值
*@param[in] char * nodeName节点名称，不包含根节点
*@param[in] thevalue 浮点型值
*@return 成功返回TRUE，失败返回FALSE
*/
BOOL xml_get_floatNode1(const char * nodeName, float* thevalue, char *xmlBuf);
/**
*@ref 获取节点的时间型值
*@param[in] char * nodeName节点名称，不包含根节点
*@param[in] thevalue 时间型值
*@return 成功返回TRUE，失败返回FALSE
*/

//时间格式为2015-03-15 12:35:02或者12:35:00
BOOL xml_get_timeNode1(const char * nodeName, ScSystemTime* thevalue, char *xmlBuf);





/**
*@ref 获取节点的字符串
*@param[in] char * nodeName节点名称，不包含根节点
*@param[in] thevalue 字符串
*@return 成功返回TRUE，失败返回FALSE
*/
BOOL xml_get_charNode2(const char * nodeName, char* cszText, char *fileName, int nLen);
/**
*@ref 获取节点的整型值
*@param[in] char * nodeName节点名称，不包含根节点
*@param[in] thevalue 整型值
*@return 成功返回TRUE，失败返回FALSE
*/
BOOL xml_get_intNode2(const char * nodeName, int* thevalue, char *fileName);
/**
*@ref 获取节点的浮点型值
*@param[in] char * nodeName节点名称，不包含根节点
*@param[in] thevalue 浮点型值
*@return 成功返回TRUE，失败返回FALSE
*/
BOOL xml_get_doubleNode2(const char * nodeName, double* thevalue, char *fileName);

/**
*@ref 获取节点的浮点型值
*@param[in] char * nodeName节点名称，不包含根节点
*@param[in] thevalue 浮点型值
*@return 成功返回TRUE，失败返回FALSE
*/
BOOL xml_get_floatNode2(const char * nodeName, float* thevalue, char *fileName);
/**
*@ref 获取节点的时间型值
*@param[in] char * nodeName节点名称，不包含根节点
*@param[in] thevalue 时间型值
*@return 成功返回TRUE，失败返回FALSE
*/

//时间格式为2015-03-15 12:35:02或者12:35:00
BOOL xml_get_timeNode2(const char * nodeName, ScSystemTime* thevalue, char *fileName);







/**
*@ref 设置节点的字符串值
*@param[in] char * nodeName节点名称，不包含根节点
*@param[in] cszText 字符串值
*@return 成功返回TRUE，失败返回FALSE
*/
BOOL xml_set_charNode1(const char * nodeName, const char* cszText, Xml_DocPtr_t pHandle);
/**
*@ref 设置节点的整型值
*@param[in] char * nodeName节点名称，不包含根节点
*@param[in] thevalue 整型值
*@return 成功返回TRUE，失败返回FALSE
*/
BOOL xml_set_intNode1(const char * nodeName, int thevalue, Xml_DocPtr_t pHandle);


/**
*@ref 设置节点的整型值
*@param[in] char * nodeName节点名称，不包含根节点
*@param[in] thevalue 整型值
*@return 成功返回TRUE，失败返回FALSE
*/
BOOL xml_set_uintNode1(const char * nodeName, unsigned int thevalue, Xml_DocPtr_t pHandle);
/**
*@ref 设置节点的整型值
*@param[in] char * nodeName节点名称，不包含根节点
*@param[in] thevalue 整型值
*@return 成功返回TRUE，失败返回FALSE
*/
BOOL xml_set_shortNode1(const char * nodeName, short thevalue, Xml_DocPtr_t pHandle);
/**
*@ref 设置节点的浮点型值
*@param[in] char * nodeName节点名称，不包含根节点
*@param[in] thevalue 浮点型值
*@return 成功返回TRUE，失败返回FALSE
*/
BOOL xml_set_doubleNode1(const char * nodeName, double thevalue, Xml_DocPtr_t pHandle);

/**
*@ref 设置节点的浮点型值
*@param[in] char * nodeName节点名称，不包含根节点
*@param[in] thevalue 浮点型值
*@return 成功返回TRUE，失败返回FALSE
*/
BOOL xml_set_floatNode1(const char * nodeName, float thevalue, Xml_DocPtr_t pHandle);
/**
*@ref 设置节点的时间型值
*@param[in] char * nodeName节点名称，不包含根节点
*@param[in] thevalue 时间型值
*@return 成功返回TRUE，失败返回FALSE
*/
//时间格式为2015-03-15 12:35:02或者12:35:00
BOOL xml_set_timeNode1(const char * nodeName, ScSystemTime thevalue, Xml_DocPtr_t pHandle);


/**
*@ref 设置节点的字符串值
*@param[in] char * nodeName节点名称，不包含根节点
*@param[in] cszText 字符串值
*@param[in] fileName 文件名
*@return 成功返回TRUE，失败返回FALSE
*/
BOOL xml_set_charNode2(const char * nodeName, const char* cszText, char *fileName);
/**
*@ref 设置节点的整型值
*@param[in] char * nodeName节点名称，不包含根节点
*@param[in] thevalue 整型值
*@return 成功返回TRUE，失败返回FALSE
*/
BOOL xml_set_intNode2(const char * nodeName, int thevalue, char *fileName);
/**
*@ref 设置节点的浮点型值
*@param[in] char * nodeName节点名称，不包含根节点
*@param[in] thevalue 浮点型值
*@return 成功返回TRUE，失败返回FALSE
*/
BOOL xml_set_doubleNode2(const char * nodeName, double thevalue, char *fileName);

/**
*@ref 设置节点的浮点型值
*@param[in] char * nodeName节点名称，不包含根节点
*@param[in] thevalue 浮点型值
*@return 成功返回TRUE，失败返回FALSE
*/
BOOL xml_set_floatNode2(const char * nodeName, float thevalue, char *fileName);
/**
*@ref 设置节点的时间型值
*@param[in] char * nodeName节点名称，不包含根节点
*@param[in] thevalue 时间型值
*@return 成功返回TRUE，失败返回FALSE
*/
//时间格式为2015-03-15 12:35:02或者12:35:00
BOOL xml_set_timeNode2(const char * nodeName, ScSystemTime thevalue, char *fileName);




/**
*@ref 获取节点的字符串
*@param[in] char * nodeName节点名称，不包含根节点
*@param[in] thevalue 字符串
*@param[in] 获取内容的长度
*@param[in] index防止一模一样的节点名，通过索引查找出第几个
*@return 成功返回TRUE，失败返回FALSE
*/
BOOL xml_get_charNode3(const char * nodeName, char* cszText, char *xmlBuf, int nLen ,int index);
/**
*@ref 获取节点的整型值
*@param[in] char * nodeName节点名称，不包含根节点
*@param[in] thevalue 整型值
*@param[in] index防止一模一样的节点名，通过索引查找出第几个
*@return 成功返回TRUE，失败返回FALSE
*/
BOOL xml_get_intNode3(const char * nodeName, int* thevalue, char *xmlBuf, int index);

/**
*@ref 获取节点的整型值
*@param[in] char * nodeName节点名称，不包含根节点
*@param[in] thevalue 整型值
*@param[in] index防止一模一样的节点名，通过索引查找出第几个
*@return 成功返回TRUE，失败返回FALSE
*/
BOOL xml_get_shortNode3(const char * nodeName, short* thevalue, char *xmlBuf, int index);
/**
*@ref 获取节点的浮点型值
*@param[in] char * nodeName节点名称，不包含根节点
*@param[in] thevalue 浮点型值
*@param[in] index防止一模一样的节点名，通过索引查找出第几个
*@return 成功返回TRUE，失败返回FALSE
*/
BOOL xml_get_doubleNode3(const char * nodeName, double* thevalue, char *xmlBuf, int index);

/**
*@ref 获取节点的浮点型值
*@param[in] char * nodeName节点名称，不包含根节点
*@param[in] thevalue 浮点型值
*@param[in] index防止一模一样的节点名，通过索引查找出第几个
*@return 成功返回TRUE，失败返回FALSE
*/
BOOL xml_get_floatNode3(const char * nodeName, float* thevalue, char *xmlBuf, int index);
/**
*@ref 获取节点的时间型值
*@param[in] char * nodeName节点名称，不包含根节点
*@param[in] thevalue 时间型值
*@param[in] index防止一模一样的节点名，通过索引查找出第几个
*@return 成功返回TRUE，失败返回FALSE
*/

//时间格式为2015-03-15 12:35:02或者12:35:00
BOOL xml_get_timeNode3(const char * nodeName, ScSystemTime* thevalue, char *xmlBuf, int index);

int parseUploadTime(int select,char *xmlPath);
int stream_getDirFile(char *dirPath,char** fileName);
char *strreplace(char **dest, char *src, const char *oldstr, const char *newstr, size_t len);
//char* convert(const char *cszText,char **new, size_t len)
;
char* convert(const char *cszText,/*char **new,*/size_t len)
;



RetErr_t  xml_get_fileLen(const char * szFileName, int* nLen);


#ifdef __cplusplus
}
#endif
#endif
