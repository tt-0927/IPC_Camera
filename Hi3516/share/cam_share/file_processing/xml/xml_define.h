#ifndef __XML_DEFINE__
#define __XML_DEFINE__

typedef void* Xml_NodePtr_t;
typedef void* Xml_DocPtr_t;
typedef struct _Xml_ParseStr_t
{
	Xml_DocPtr_t pXmlHandle;//解析字符串的句柄
	Xml_NodePtr_t pNRootNode;//解析字符串的根节点
}Xml_ParseStr_t;

#endif
