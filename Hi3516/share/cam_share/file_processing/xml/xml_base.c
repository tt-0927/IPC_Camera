#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>
#include <string.h>
#include "xml_base.h"
#include "./libxml/parser.h"
#include "./libxml/tree.h"
#define XMLVERSION "1.0"
#define ENCODE_MODE "UTF-8"
#define XMLFORMAT 1
/*
xml所有转义符

和 & &amp; //验证确实&符号不支持，连同内容也不能设置
大于 >  &gt ;
小于 <  &lt;
空格  &nbsp;//其他特殊符号不能出现在节点名字上
单引号 ‘ &apos;
双引号 “ &quot;
井号 # &#35;
*/


xmlSpecialCharacter Special_character[] =
{
	{"&", "&amp;"},	
	{">", "&gt;"},
	{"<", "&lt;"},
	{" ", "&nbsp;"},
	//{"'", "&apos;"},
	{"\"", "&quot;"},
	//{"#", "&#35;"},
};








char *strreplace(char **dest, char *src, const char *oldstr, const char *newstr, size_t len)
{
#if 0

		int lenPtr = 0;
		if(strcmp(oldstr, newstr)==0)
			return src;
		char *needle;
		char *tmp;
		*dest = src;
		while((needle = strstr(*dest+lenPtr+1, oldstr)) && ((lenPtr = needle -*dest) <= len))
		{
			tmp=(char*)malloc(strlen(*dest)+(strlen(newstr)-strlen(oldstr))+1);
			strncpy(tmp, *dest, needle-*dest);
			tmp[needle-*dest]='\0';
			strcat(tmp, newstr);
			strcat(tmp, needle+strlen(oldstr));
			if(*dest != src && *dest != NULL)
			{
				free(*dest);
			}
			*dest = strdup(tmp);
			free(tmp);
		}
		return *dest;
#else

		int flags = 0;
		int lenPtr = 0;
		if(strcmp(oldstr, newstr)==0)
		{
			return src;
		}
		int NewLen = strlen(newstr);	
		char *needle,*tmp;
		*dest = src;
		tmp=(char*)malloc(len);
		while((needle = strstr(*dest+lenPtr+flags*NewLen, oldstr)) && ((lenPtr = needle -*dest) <= len))
		{
			memset(tmp,0,len);
			strncpy(tmp, *dest, needle-*dest);
			tmp[needle-*dest]='\0';
			strcat(tmp, newstr);
			strcat(tmp, needle+strlen(oldstr));
			if(*dest != src && *dest != NULL)
			{
				free(*dest);
			}
			*dest = strdup(tmp);
			flags = 1;
		}
		free(tmp);
		return *dest;
#endif
}


char* convert(const char *cszText,/*char **new,*/size_t len)
{
	char *xmlNew = NULL;
	int nCirCul = 0;
	int nLen = sizeof(Special_character) /sizeof(Special_character[0]); 
	strreplace(&xmlNew,cszText,Special_character[nCirCul].xmlOld,Special_character[nCirCul].xmlNew,len);
	for(nCirCul = 1; nCirCul< nLen; nCirCul++)
	{
				if(strcmp(Special_character[nCirCul].xmlOld, Special_character[nCirCul].xmlNew)==0)
					continue;
	}
	return xmlNew;
}







Xml_DocPtr_t xml_read_file(const char *fileName ,const char *encoding)
{
	return xmlReadFile(fileName, encoding, XML_PARSE_RECOVER);
}

void xml_init()
{
	xmlInitParser();
}
void xml_unit()
{
	xmlCleanupParser();
}
Xml_DocPtr_t xml_init_doc()
{
	/*xml初始化doc文档*/
	xmlDocPtr pDocHandle = xmlNewDoc((unsigned char*)XMLVERSION);
	if(NULL == pDocHandle)
	{
		return NULL;
	}
	return pDocHandle;
}

int xml_save_handleToFile(const char* cszFileFullPath, Xml_DocPtr_t DocHandle, const char *encoding)// 保存文件
{
	char EncodeBuf[16] = {0};
	xmlDocPtr pXmlHandle = (xmlDocPtr)DocHandle;
	if(cszFileFullPath == NULL || pXmlHandle == NULL)
	{
		return -1;
	}

	if(encoding ==  NULL)
	{
		strcpy(EncodeBuf, ENCODE_MODE);
	}
	else
	{
		strcpy(EncodeBuf, encoding);
	}
	xmlKeepBlanksDefault(0);//当xmlKeepBlanksDefault(0)时，format设置为1才能生效
	int nLen = xmlSaveFormatFileEnc(cszFileFullPath, pXmlHandle, EncodeBuf, XMLFORMAT);
	return nLen;
}
int xml_save_strToFile(const char* cszFileFullPath, const char *pXmlBuf, const char *encoding)	// 保存文件
{
	char EncodeBuf[16] = {0};
	if(cszFileFullPath == NULL || pXmlBuf == NULL)
	{
		return -1;
	}
	if(encoding ==  NULL)
	{
		strcpy(EncodeBuf, ENCODE_MODE);
	}
	else
	{
		strcpy(EncodeBuf, encoding);
	}
	xmlDocPtr pXmlHandle= xmlParseMemory((char *)pXmlBuf , strlen((char *)pXmlBuf));
	if(NULL == pXmlHandle)
	{
		return -1;
	}
	xmlKeepBlanksDefault(0);//当xmlKeepBlanksDefault(0)时，format设置为1才能生效
	int nLen = xmlSaveFormatFileEnc(cszFileFullPath, pXmlHandle, EncodeBuf, XMLFORMAT);
	if(pXmlHandle)
	{
		xmlFreeDoc(pXmlHandle);
	}
	return nLen;

}
//RetErr_t  xml_get_fileLen(const char * szFileName, int* nLen)
//{
//	if(szFileName == NULL || nLen == NULL)
//	{
//		return RET_PARAMER_ERR;
//	}
//
//	FILE *pFile= fopen(szFileName, "r");
//	if(NULL == pFile)
//	{
//		perror("open xml fail\n");
//		return RET_OPENFILE_FAILED;
//	}
//	fseek(pFile, 0, SEEK_END);
//	*nLen = ftell(pFile);
//	if(pFile)
//	{
//		fclose(pFile);
//	}
//	return RET_SUCCESS;
//}

Xml_DocPtr_t xml_get_file(const char* FileFullPath,const char *encoding, char* pszBuf)
{
	if(FileFullPath == NULL || encoding == NULL)
	{
		return NULL;
	}
	xmlDocPtr pdoc = xmlParseFile(FileFullPath);
	if(pdoc == NULL)
	{
		return NULL;
	}
	if(pszBuf != NULL)
	{
		unsigned char *OuBuf = NULL;
		int OutLen = 0;
		xmlKeepBlanksDefault(0);//当xmlKeepBlanksDefault(0)时，format设置为1才能生效
		xmlDocDumpFormatMemoryEnc(pdoc, &OuBuf,&OutLen, encoding, XMLFORMAT);
		if(OuBuf)
		{
			strcpy(pszBuf, (const char *)OuBuf);
			xmlFree(OuBuf);
		}
	}
	return pdoc;
}

RetErr_t xml_handleTo_str(Xml_DocPtr_t DoCHandle,const char *encoding, char** OutBuf)
{
	xmlDocPtr pdoc = (xmlDocPtr)DoCHandle;
	if(pdoc == NULL || encoding == NULL)
	{
		return RET_PARAMER_ERR;
	}
	int OutLen = 0;
	xmlKeepBlanksDefault(0);//当xmlKeepBlanksDefault(0)时，format设置为1才能生效
	xmlDocDumpFormatMemoryEnc(pdoc, (xmlChar**)OutBuf,&OutLen, encoding, XMLFORMAT);
	if(OutLen == 0)
	{
		return RET_UNKNOW_FAIL;
	}
	return RET_SUCCESS;
}

Xml_NodePtr_t xml_append_rootNode(Xml_DocPtr_t XmlDocHandle, const char* nodeChildName)
{
	xmlDocPtr pDocHandle = (xmlDocPtr)XmlDocHandle;
	if(pDocHandle == NULL)
	{
		return NULL;
	}
	xmlNodePtr pRootNode = xmlNewNode(NULL, BAD_CAST nodeChildName);
	if(NULL == pRootNode)
	{
		return NULL;
	}
	/*root node 加入到 doc文档中*/
	xmlDocSetRootElement(pDocHandle, pRootNode);
	return pRootNode;
}

Xml_NodePtr_t xml_append_childNode(Xml_NodePtr_t nodeParent,  const char* nodeChildName)
{
	xmlNodePtr pNodeParent =(xmlNodePtr)nodeParent;
	if(pNodeParent == NULL)
	{
		return NULL;
	}
	/*create Head node*/
	xmlNodePtr Child_node = xmlNewNode(NULL, BAD_CAST nodeChildName);
	if(NULL == Child_node){
		/*报警*/
		return NULL;
	}
	/*Head node 加入到 root node*/
	xmlAddChild(pNodeParent, Child_node);
	return Child_node;
}

BOOL xml_set_charNode(Xml_NodePtr_t nodeDest, const char* cszText)
{
	xmlNodePtr pCurNode =(xmlNodePtr)nodeDest;
	if(pCurNode == NULL)
	{
		return FALSE;
	}
	//change
	char *newChar = NULL;
	const char* oldChar= cszText;
	newChar = convert(oldChar,/*&new,*/(strlen(oldChar)+1)*6);
	xmlNodeSetContent(pCurNode,BAD_CAST newChar);
	if(newChar != oldChar)
	{
		free(newChar);
		newChar = NULL;
	}
	return TRUE;
}
BOOL xml_set_intNode(Xml_NodePtr_t nodeDest, int thevalue)
{
	char pValueBuf[16] = {0};
	sprintf(pValueBuf, "%d", thevalue);
	return xml_set_charNode(nodeDest, pValueBuf);
}
BOOL xml_set_uintNode(Xml_NodePtr_t nodeDest,unsigned int thevalue)
{
	char pValueBuf[16] = {0};
	sprintf(pValueBuf, "%u", thevalue);
	return xml_set_charNode(nodeDest, pValueBuf);
}
BOOL xml_set_doubleNode(Xml_NodePtr_t nodeDest, double thevalue)
{
	char pValueBuf[16] = {0};
	sprintf(pValueBuf, "%0.6lf", thevalue);
	return xml_set_charNode(nodeDest, pValueBuf);
}
BOOL xml_set_floatNode(Xml_NodePtr_t nodeDest, float thevalue)
{
	char pValueBuf[16] = {0};
	sprintf(pValueBuf, "%0.6f", thevalue);
	return xml_set_charNode(nodeDest, pValueBuf);
}
BOOL xml_set_timeNode(Xml_NodePtr_t nodeDest, ScSystemTime thevalue)
{
	char pValueBuf[32] = {0};
	if(thevalue.wYear < 1970)
	{
		sprintf(pValueBuf, "%02d:%02d:%02d", thevalue.wHour, thevalue.wMinute, thevalue.wSecond);
	}
	else
	{
		sprintf(pValueBuf, "%04d-%02d-%02d %02d:%02d:%02d",thevalue.wYear,thevalue.wMonth,thevalue.wDay,
				thevalue.wHour, thevalue.wMinute, thevalue.wSecond);
	}
	return xml_set_charNode(nodeDest, pValueBuf);
}

BOOL xml_get_charNode(Xml_NodePtr_t nodeDest, char* cszText, int nLen)
{
	xmlNodePtr curNode = (xmlNodePtr)nodeDest;
	if(curNode == NULL || nLen <= 0)
	{
		return FALSE;
	}
	xmlChar * szKey = xmlNodeGetContent(curNode);
	if(szKey == NULL)
	{
		return FALSE;
	}
	strncpy(cszText, (const char*)szKey, nLen);
	xmlFree(szKey);
	return TRUE;
}

BOOL xml_get_intNode(Xml_NodePtr_t nodeDest, int* thevalue)
{
	char pBuf[16] = {0};
	if(FALSE == xml_get_charNode(nodeDest, pBuf, sizeof(pBuf)))
	{
		return FALSE;
	}
	*thevalue = atoi(pBuf);
	return TRUE;
}
BOOL xml_get_doubleNode(Xml_NodePtr_t nodeDest, double* thevalue)
{
	char pBuf[16] = {0};
	if(FALSE == xml_get_charNode(nodeDest, pBuf, sizeof(pBuf)))
	{
		return FALSE;
	}
	*thevalue = atof(pBuf);
	return TRUE;
}
BOOL xml_get_floatNode(Xml_NodePtr_t nodeDest, float* thevalue)
{
	char pBuf[16] = {0};
	if(FALSE == xml_get_charNode(nodeDest, pBuf, sizeof(pBuf)))
	{
		return FALSE;
	}
	*thevalue = atof(pBuf);
	return TRUE;
}
static void StrToSysTime(const char *pBuf, ScSystemTime* thevalue)
{
	char pNumBuf[8] = {0};
	const char* pPointHour = pBuf;
	int nStep = 0;
	int nNumGet = 0;//取了几次值
	int nPos = 0;//记住上次取值的位置
	int nStrLen = 0;//字符串大小

	char *pLongDate = strchr((char *)pBuf, '-');

	if(pLongDate)//取出年月日
	{
		nStrLen = strlen(pPointHour);
		for(;nStep < nStrLen; nStep++)
		{
			if(pPointHour[nStep] == '-' || pPointHour[nStep] == ' ')
			{
				memset(pNumBuf, 0 ,sizeof(pNumBuf));
				strncpy(pNumBuf, &(pPointHour[nPos]), nStep - nPos);
				nPos = nStep + 1;
				nNumGet++;
				if(nNumGet == 1)
				{
					thevalue->wYear = atoi(pNumBuf);
				}
				else if(nNumGet == 2)
				{
					thevalue->wMonth = atoi(pNumBuf);
				}
				else if(nNumGet == 3)
				{
					thevalue->wDay = atoi(pNumBuf);
					pPointHour = &(pBuf[nStep+1]);
					break;
				}
			}
		}
		nNumGet = 0;
		nPos = 0;

	}
	nStrLen = strlen(pPointHour);
	for(nStep = 0; nStep < nStrLen; nStep++)
	{
		if(pPointHour[nStep] == ':' || nStep == nStrLen - 1)
		{
			if(nStep == nStrLen - 1)
			{
				nStep++;//最后一个指针偏移一位减去之前位置就是最后秒的数值
			}
			strncpy(pNumBuf, &(pPointHour[nPos]), nStep - nPos);
			nPos = nStep + 1;
			nNumGet++;
			if(nNumGet == 1)
			{
				thevalue->wHour = atoi(pNumBuf);
			}
			else if(nNumGet == 2)
			{
				thevalue->wMinute = atoi(pNumBuf);
			}
			else if(nNumGet == 3)
			{
				thevalue->wSecond = atoi(pNumBuf);
			}
		}
	}
}
BOOL xml_get_timeNode(Xml_NodePtr_t nodeDest, ScSystemTime* thevalue)
{
	char pBuf[32] = {0};
	if(FALSE == xml_get_charNode(nodeDest, pBuf, sizeof(pBuf)))
	{
		return FALSE;
	}
	memset(thevalue, 0 , sizeof(ScSystemTime));
	StrToSysTime(pBuf, thevalue);

	return TRUE;
}

Xml_NodePtr_t xml_remove_node(Xml_NodePtr_t nodeRemoving)
{
	xmlNodePtr pNode = (xmlNodePtr) nodeRemoving;
	if(NULL == pNode)
	{
		return NULL;
	}

	xmlNodePtr tempNode;
	tempNode = pNode->next;
	xmlUnlinkNode(pNode);
	xmlFreeNode(pNode);
	pNode = tempNode;
	return pNode;
}
RetErr_t xml_init_str(Xml_ParseStr_t *parse_xml, const char *xml)
{
	if(NULL == parse_xml || NULL == xml)
	{
		return RET_PARAMER_ERR;
	}

	parse_xml->pXmlHandle= xmlParseMemory((char *)xml , strlen((char *)xml));
	if(NULL == parse_xml->pXmlHandle)
	{
		return RET_PARSE_ERR;
	}

	// 获取 xml 文档对象的根节对象
	parse_xml->pNRootNode = xmlDocGetRootElement((xmlDocPtr)(parse_xml->pXmlHandle));
	if(NULL == parse_xml->pNRootNode)
	{
		if(parse_xml->pXmlHandle != NULL)
		{
			xmlFreeDoc(((xmlDocPtr)(parse_xml->pXmlHandle)));
			parse_xml->pXmlHandle = NULL;
		}
		return RET_NODE_ERR;
	}
	return RET_SUCCESS;
}

Xml_NodePtr_t xml_get_rootNode(Xml_DocPtr_t DocHandle)
{
	xmlDocPtr pRootNode = (xmlDocPtr)DocHandle;
	if(pRootNode == NULL)
	{
		return NULL;
	}
	return xmlDocGetRootElement(pRootNode);
}
Xml_NodePtr_t xml_get_firstChildNode(Xml_NodePtr_t nodeParent)
{
	xmlNodePtr pNodePar = (xmlNodePtr)nodeParent;
	if(pNodePar == NULL)
	{
		return NULL;
	}
	return xmlFirstElementChild(pNodePar);
}
Xml_NodePtr_t xml_get_prevSiblingNode(Xml_NodePtr_t nodeFrom)
{
	xmlNodePtr pNode = (xmlNodePtr)nodeFrom;
	if(pNode == NULL)
	{
		return NULL;
	}
	return xmlPreviousElementSibling(pNode);
}
Xml_NodePtr_t xml_get_nextSiblingNode(Xml_NodePtr_t nodeFrom)
{
	xmlNodePtr pNode = (xmlNodePtr)nodeFrom;
	if(pNode == NULL)
	{
		return NULL;
	}
	return  xmlNextElementSibling(pNode);
}
Xml_NodePtr_t xml_get_childNode(Xml_NodePtr_t nodeFrom, const char *key)
{

	xmlNodePtr curNode =(xmlNodePtr)nodeFrom;
	if(NULL == curNode || NULL == key)
	{
		return NULL;
	}

	xmlNodePtr pcur = curNode->xmlChildrenNode;

	while(pcur != NULL)
	{
		if(!xmlStrcmp(pcur->name, (const xmlChar*)key))
		{
			return pcur;
		}

		pcur = pcur->next;
	}

	return NULL;
}


Xml_NodePtr_t xml_get_sameNameBroNode(Xml_NodePtr_t nodecur, const char *key)
{

	xmlNodePtr curNode =(xmlNodePtr)nodecur;
	if(NULL == curNode || NULL == key)
	{
		return NULL;
	}

	xmlNodePtr pcur = (xmlNodePtr)xml_get_nextSiblingNode((xmlNodePtr)curNode);

	while(pcur != NULL)
	{
		if(!xmlStrcmp(pcur->name, (const xmlChar*)key))
		{
			return pcur;
		}

		pcur = (xmlNodePtr)xml_get_nextSiblingNode((xmlNodePtr)pcur);
	}

	return NULL;
}


char * xml_get_nodeName(Xml_NodePtr_t node)
{
	xmlNodePtr curNode =(xmlNodePtr)node;
	if(NULL == curNode)
	{
		return NULL;
	}
	char *pName = (char *)curNode->name;
	return pName;
}
char * xml_set_nodeName(Xml_NodePtr_t node, const char * name)
{
	xmlNodePtr curNode =(xmlNodePtr)node;
	if(NULL == curNode || name == NULL)
	{
		return NULL;
	}
	xmlNodeSetName(curNode, (const xmlChar*)name);
	char *pName = (char *)curNode->name;
	return pName;
}

BOOL xml_free_docHandle(Xml_DocPtr_t DoCHandle)
{
	xmlDocPtr pHandle = (xmlDocPtr)DoCHandle;
	if(pHandle == NULL)
	{
		return FALSE;
	}
	xmlFreeDoc(pHandle);
	return TRUE;
}
BOOL xml_free_baseBuf(char *pBuf)
{
	if(pBuf == NULL)
	{
		return FALSE;
	}
	xmlFree(pBuf);
	return TRUE;
}


xmlNodePtr xml_nodeName_Node(const char * nodeName, Xml_ParseStr_t  *parse_xml, int index)
{
	char *childNode = NULL;
	char charNodeName[64] = {0};
	const char *str = NULL;
	Xml_NodePtr_t pChildNode = NULL;
	Xml_NodePtr_t parentNode = NULL;
	int root = FALSE;
	int i = 0;
	int find = 0;
	childNode = strstr(nodeName, "/");
	str = nodeName;
	if(childNode == NULL || parse_xml == NULL)
	{
		return NULL;
	}

	str++;
	for(; *str != '\0'; str++)
	{
		i++;
		if(*str == '/')
		{
			memset(charNodeName, 0, sizeof(charNodeName));
			memcpy(charNodeName, nodeName + 1, i - 1);
			//printf("%s\n", charNodeName);

			if(root == FALSE)
			{
				pChildNode = parse_xml->pNRootNode;
				root = TRUE;
			}
			else
			{
				while (parentNode != NULL)
				{
					pChildNode = xml_get_childNode(parentNode, charNodeName);
					if (pChildNode == NULL)
					{

						parentNode = xml_get_sameNameBroNode(parentNode, xml_get_nodeName(parentNode));
					}
					else
					{
						parentNode = xml_get_sameNameBroNode(parentNode, xml_get_nodeName(parentNode));
						if (parentNode != NULL)
						{
							if (find++ >= index)
							{
								break;
							}
						}



					}
				}
				if (pChildNode == NULL)
				{
					break;
				}



			}

			nodeName = str;
			parentNode = pChildNode;
			i = 0;
		}
	}

	return (xmlNodePtr)pChildNode;
}



BOOL xml_get_charNode1(const char * nodeName, char* cszText, char *xmlBuf, int nLen)
{
	xmlNodePtr curNode = NULL;
	Xml_ParseStr_t parse_xml;
	parse_xml.pXmlHandle=NULL;
	parse_xml.pNRootNode=NULL;
	BOOL ret = TRUE;
	xmlChar * szKey = NULL;
	if(nodeName == NULL || cszText == NULL || nLen <= 0)
	{
		return FALSE;

	}
	ret = xml_init_str(&parse_xml, xmlBuf);
	if(ret != RET_SUCCESS)
	{
		ret = FALSE;
		goto EXIT;
	}

	curNode = xml_nodeName_Node(nodeName, &parse_xml, 0);
	if(curNode == NULL)
	{
		ret = FALSE;
		goto EXIT;
	}

	szKey = xmlNodeGetContent(curNode);
	if(szKey == NULL)
	{
		ret = FALSE;
		goto EXIT;
	}
	ret = TRUE;
	strncpy(cszText, (const char*)szKey, nLen);
	xmlFree(szKey);
EXIT:
	if(parse_xml.pXmlHandle)
	{
		xml_free_docHandle(parse_xml.pXmlHandle);//将句柄销毁
	}
	else
	{
		ret = FALSE;
	}
	return ret;
}

BOOL xml_get_intNode1(const char * nodeName, int* thevalue, char *xmlBuf)
{
	char pBuf[16] = {0};
	if(FALSE == xml_get_charNode1(nodeName, pBuf, xmlBuf, sizeof(pBuf)))
	{
		return FALSE;
	}
	//*thevalue = atoi(pBuf);
	sscanf(pBuf, "%d",thevalue);
	return TRUE;
}
BOOL xml_get_shortNode1(const char * nodeName, short* thevalue, char *xmlBuf)
{
	char pBuf[16] = { 0 };
	if (FALSE == xml_get_charNode1(nodeName, pBuf, xmlBuf, sizeof(pBuf)))
	{
		return FALSE;
	}
	*thevalue = atoi(pBuf);
	return TRUE;
}
BOOL xml_get_doubleNode1(const char * nodeName, double* thevalue, char *xmlBuf)
{
	char pBuf[16] = {0};
	if(FALSE == xml_get_charNode1(nodeName, pBuf, xmlBuf, sizeof(pBuf)))
	{
		return FALSE;
	}
	*thevalue = atof(pBuf);
	return TRUE;
}
BOOL xml_get_floatNode1(const char * nodeName, float* thevalue, char *xmlBuf)
{
	char pBuf[16] = {0};
	if(FALSE == xml_get_charNode1(nodeName, pBuf, xmlBuf, sizeof(pBuf)))
	{
		return FALSE;
	}
	*thevalue = atof(pBuf);
	return TRUE;
}
BOOL xml_get_timeNode1(const char * nodeName, ScSystemTime* thevalue,  char *xmlBuf)
{
	char pBuf[32] = {0};
	if(FALSE == xml_get_charNode1(nodeName, pBuf, xmlBuf, sizeof(pBuf)))
	{
		return FALSE;
	}
	memset(thevalue, 0 , sizeof(ScSystemTime));
	StrToSysTime(pBuf, thevalue);

	return TRUE;
}





BOOL xml_get_charNode2(const char * nodeName, char* cszText, char *fileName, int nLen)
{
	xmlNodePtr curNode = NULL;
	Xml_ParseStr_t parse_xml;
	int ret = TRUE;
	xmlChar * szKey = NULL;
	if(nodeName == NULL || fileName == NULL || nLen <= 0)
	{
		return FALSE;
	}
	parse_xml.pXmlHandle = xml_read_file(fileName, NULL);
	if(parse_xml.pXmlHandle == NULL)
	{
		ret = FALSE;
		goto EXIT;
	}
	parse_xml.pNRootNode = xmlDocGetRootElement((xmlDocPtr)(parse_xml.pXmlHandle));
	if(parse_xml.pNRootNode == NULL)
	{
		ret = FALSE;
		goto EXIT;
	}
	curNode = xml_nodeName_Node(nodeName, &parse_xml, 0);
	if(curNode == NULL)
	{
		ret = FALSE;
		goto EXIT;
	}

	szKey = xmlNodeGetContent(curNode);
	if(szKey == NULL)
	{
		ret = FALSE;
		goto EXIT;
	}
	ret = TRUE;

	//char *getTest = NULL;
	//strncpy(getTest, (const char*)szKey, nLen);
	//convert(getTest,cszText);
	strncpy(cszText, (const char*)szKey, nLen);
	xmlFree(szKey);
EXIT:
	if(parse_xml.pXmlHandle)
	{
		xml_free_docHandle(parse_xml.pXmlHandle);//将句柄销毁
	}
	else
	{
		ret = FALSE;
	}
	return ret;
}

BOOL xml_get_intNode2(const char * nodeName, int* thevalue, char *fileName)
{
	char pBuf[16] = {0};
	if(FALSE == xml_get_charNode2(nodeName, pBuf, fileName, sizeof(pBuf)))
	{
		return FALSE;
	}
	*thevalue = atoi(pBuf);
	return TRUE;
}
BOOL xml_get_doubleNode2(const char * nodeName, double* thevalue, char *fileName)
{
	char pBuf[16] = {0};
	if(FALSE == xml_get_charNode2(nodeName, pBuf, fileName, sizeof(pBuf)))
	{
		return FALSE;
	}
	*thevalue = atof(pBuf);
	return TRUE;
}
BOOL xml_get_floatNode2(const char * nodeName, float* thevalue, char *fileName)
{
	char pBuf[16] = {0};
	if(FALSE == xml_get_charNode2(nodeName, pBuf, fileName, sizeof(pBuf)))
	{
		return FALSE;
	}
	*thevalue = atof(pBuf);
	return TRUE;
}
BOOL xml_get_timeNode2(const char * nodeName, ScSystemTime* thevalue,  char *fileName)
{
	char pBuf[32] = {0};
	if(FALSE == xml_get_charNode2(nodeName, pBuf, fileName, sizeof(pBuf)))
	{
		return FALSE;
	}
	memset(thevalue, 0 , sizeof(ScSystemTime));
	StrToSysTime(pBuf, thevalue);

	return TRUE;
}



BOOL xml_get_charNode3(const char * nodeName, char* cszText, char *xmlBuf, int nLen, int index)
{
	xmlNodePtr curNode = NULL;
	Xml_ParseStr_t parse_xml;
	xmlChar * szKey = NULL;
	BOOL ret = TRUE;
	parse_xml.pXmlHandle = NULL;
	parse_xml.pNRootNode = NULL;
	if (nodeName == NULL || nLen <= 0)
	{
		return FALSE;

	}
	ret = xml_init_str(&parse_xml, xmlBuf);
	if (ret != RET_SUCCESS)
	{
		ret = FALSE;
		goto EXIT;
	}

	curNode = xml_nodeName_Node(nodeName, &parse_xml, index);
	if (curNode == NULL)
	{
		ret = FALSE;
		goto EXIT;
	}

	szKey = xmlNodeGetContent(curNode);
	if (szKey == NULL)
	{
		ret = FALSE;
		goto EXIT;
	}
	ret = TRUE;
	strncpy(cszText, (const char*)szKey, nLen);
	xmlFree(szKey);
EXIT:
	if (parse_xml.pXmlHandle)
	{
		xml_free_docHandle(parse_xml.pXmlHandle);//将句柄销毁
	}
	else
	{
		ret = FALSE;
	}
	return ret;
}

BOOL xml_get_intNode3(const char * nodeName, int* thevalue, char *xmlBuf, int index)
{
	char pBuf[16] = { 0 };
	if (FALSE == xml_get_charNode3(nodeName, pBuf, xmlBuf, sizeof(pBuf) ,index))
	{
		return FALSE;
	}
	*thevalue = atoi(pBuf);
	return TRUE;
}
BOOL xml_get_shortNode3(const char * nodeName, short* thevalue, char *xmlBuf, int index)
{
	char pBuf[16] = { 0 };
	if (FALSE == xml_get_charNode3(nodeName, pBuf, xmlBuf, sizeof(pBuf), index))
	{
		return FALSE;
	}
	*thevalue = atoi(pBuf);
	return TRUE;
}
BOOL xml_get_doubleNode3(const char * nodeName, double* thevalue, char *xmlBuf, int index)
{
	char pBuf[16] = { 0 };
	if (FALSE == xml_get_charNode3(nodeName, pBuf, xmlBuf, sizeof(pBuf), index))
	{
		return FALSE;
	}
	*thevalue = atof(pBuf);
	return TRUE;
}
BOOL xml_get_floatNode3(const char * nodeName, float* thevalue, char *xmlBuf, int index)
{
	char pBuf[16] = { 0 };
	if (FALSE == xml_get_charNode3(nodeName, pBuf, xmlBuf, sizeof(pBuf), index))
	{
		return FALSE;
	}
	*thevalue = atof(pBuf);
	return TRUE;
}
BOOL xml_get_timeNode3(const char * nodeName, ScSystemTime* thevalue, char *xmlBuf, int index)
{
	char pBuf[32] = { 0 };
	if (FALSE == xml_get_charNode3(nodeName, pBuf, xmlBuf,sizeof(pBuf), index))
	{
		return FALSE;
	}
	memset(thevalue, 0, sizeof(ScSystemTime));
	StrToSysTime(pBuf, thevalue);

	return TRUE;
}


xmlNodePtr xml_get_NodePtr(const char * nodeName, const char* cszText, Xml_DocPtr_t pHandle)
{
	char *childNode = NULL;
	char charNodeName[64] = {0};
	const char *str = NULL;
	int root = FALSE;
	Xml_NodePtr_t parentNode = NULL;
	Xml_NodePtr_t pChildNode = NULL;
	int i = 0;
	childNode = strstr(nodeName, "/");
	str = nodeName;
	if(childNode == NULL || pHandle == NULL)
	{
		return NULL;
	}



	str++;
	for(; *str != '\0'; str++)
	{
		i++;
		if(*str == '/')
		{
			memset(charNodeName, 0, sizeof(charNodeName));
			memcpy(charNodeName, nodeName + 1, i - 1);
			//printf("%s\n", charNodeName);

			if(root == FALSE)
			{
				root = TRUE;
				pChildNode = xmlDocGetRootElement((xmlDocPtr)pHandle);
				if(pChildNode == NULL)
				{
					pChildNode = xml_append_rootNode(pHandle, charNodeName);
					if(pChildNode == NULL)
					{
						return FALSE;
					}
				}
			}
			else
			{
				pChildNode = xml_get_childNode(parentNode, charNodeName);
				if(pChildNode == NULL)
				{
					//printf("*****charNodeName:%s\n\n", charNodeName);
					pChildNode = xml_append_childNode(parentNode,  charNodeName);
					if(pChildNode == NULL)
					{
						break;
					}
				}
			}

			nodeName = str;
			parentNode = pChildNode;
			i = 0;
		}
	}

	return (xmlNodePtr)pChildNode;
}


BOOL xml_set_charNode1(const char * nodeName, const char* cszText, Xml_DocPtr_t pHandle)
{

	if(cszText == NULL || nodeName == NULL || pHandle == NULL)
	{
		return FALSE;
	}
	xmlNodePtr pCurNode = xml_get_NodePtr(nodeName, cszText, pHandle);
	if(pCurNode == NULL)
	{
		return FALSE;
	}
	char *newChar = NULL;
	const char* oldChar= cszText;
	newChar = convert(oldChar,/*&new,*/(strlen(oldChar)+1)*6);
	xmlNodeSetContent(pCurNode,BAD_CAST newChar);
	if(newChar != oldChar)
	{
		free(newChar);
		newChar = NULL;
	}
	//xmlNodePtr content = xmlNewText(BAD_CAST cszText);
	//xmlAddChild(pCurNode,content);
	return TRUE;
}
BOOL xml_set_intNode1(const char * nodeName, int thevalue, Xml_DocPtr_t pHandle)
{
	char pValueBuf[16] = {0};
	sprintf(pValueBuf, "%d", thevalue);
	return xml_set_charNode1(nodeName, pValueBuf, pHandle);
}
BOOL xml_set_uintNode1(const char * nodeName, unsigned int thevalue, Xml_DocPtr_t pHandle)
{
	char pValueBuf[16] = {0};
	sprintf(pValueBuf, "%u", thevalue);
	return xml_set_charNode1(nodeName, pValueBuf, pHandle);
}
BOOL xml_set_shortNode1(const char * nodeName, short thevalue, Xml_DocPtr_t pHandle)
{
	char pValueBuf[16] = { 0 };
	sprintf(pValueBuf, "%d", thevalue);
	return xml_set_charNode1(nodeName, pValueBuf, pHandle);
}
BOOL xml_set_doubleNode1(const char * nodeName, double thevalue, Xml_DocPtr_t pHandle)
{
	char pValueBuf[16] = {0};
	sprintf(pValueBuf, "%0.6lf", thevalue);
	return xml_set_charNode1(nodeName, pValueBuf, pHandle);
}
BOOL xml_set_floatNode1(const char * nodeName, float thevalue, Xml_DocPtr_t pHandle)
{
	char pValueBuf[16] = {0};
	sprintf(pValueBuf, "%0.6f", thevalue);
	return xml_set_charNode1(nodeName, pValueBuf, pHandle);
}
BOOL xml_set_timeNode1(const char * nodeName, ScSystemTime thevalue, Xml_DocPtr_t pHandle)
{
	char pValueBuf[32] = {0};
	if(thevalue.wYear < 1970)
	{
		sprintf(pValueBuf, "%02d:%02d:%02d", thevalue.wHour, thevalue.wMinute, thevalue.wSecond);
	}
	else
	{
		sprintf(pValueBuf, "%04d-%02d-%02d %02d:%02d:%02d",thevalue.wYear,thevalue.wMonth,thevalue.wDay,
				thevalue.wHour, thevalue.wMinute, thevalue.wSecond);
	}
	return xml_set_charNode1(nodeName, pValueBuf, pHandle);
}







BOOL xml_set_charNode2(const char * nodeName, const char* cszText, char* filename)
{
	Xml_DocPtr_t pHandle = xml_read_file(filename ,  "UTF-8");
	if(pHandle == NULL)
	{
		pHandle =  xml_init_doc();
		if(pHandle == NULL)
			return FALSE;
	}
	if(cszText == NULL || nodeName == NULL || pHandle == NULL)
	{
		xml_free_docHandle(pHandle);
		return FALSE;
	}
	xmlNodePtr pCurNode = xml_get_NodePtr(nodeName, cszText, pHandle);
	if(pCurNode == NULL)
	{
		xml_free_docHandle(pHandle);
		return FALSE;
	}
	xmlNodeSetContent(pCurNode,BAD_CAST cszText);
	xml_save_handleToFile(filename, pHandle, "UTF-8");
	//xmlNodePtr content = xmlNewText(BAD_CAST cszText);
	//xmlAddChild(pCurNode,content);
	xml_free_docHandle(pHandle);
	return TRUE;
}
BOOL xml_set_intNode2(const char * nodeName, int thevalue, char* filename)
{
	char pValueBuf[16] = {0};
	sprintf(pValueBuf, "%d", thevalue);
	return xml_set_charNode2(nodeName, pValueBuf, filename);
}

BOOL xml_set_doubleNode2(const char * nodeName, double thevalue, char* filename)
{
	char pValueBuf[16] = {0};
	sprintf(pValueBuf, "%0.6lf", thevalue);
	return xml_set_charNode2(nodeName, pValueBuf, filename);
}
BOOL xml_set_floatNode2(const char * nodeName, float thevalue, char* filename)
{
	char pValueBuf[16] = {0};
	sprintf(pValueBuf, "%0.6f", thevalue);
	return xml_set_charNode2(nodeName, pValueBuf, filename);
}
BOOL xml_set_timeNode2(const char * nodeName, ScSystemTime thevalue, char* filename)
{
	char pValueBuf[32] = {0};
	if(thevalue.wYear < 1970)
	{
		sprintf(pValueBuf, "%02d:%02d:%02d", thevalue.wHour, thevalue.wMinute, thevalue.wSecond);
	}
	else
	{
		sprintf(pValueBuf, "%04d-%02d-%02d %02d:%02d:%02d",thevalue.wYear,thevalue.wMonth,thevalue.wDay,
				thevalue.wHour, thevalue.wMinute, thevalue.wSecond);
	}
	return xml_set_charNode2(nodeName, pValueBuf, filename);
}


BOOL xml_remove_node1(const char * nodeName, char *fileName)
{

	Xml_DocPtr_t pHandle = xml_read_file(fileName ,  "UTF-8");
	if(pHandle == NULL)
	{
		return FALSE;
	}

	if(nodeName == NULL || pHandle == NULL)
	{
		xml_free_docHandle(pHandle);
		return FALSE;
	}
	xmlNodePtr pCurNode = xml_get_NodePtr(nodeName, NULL, pHandle);
	if(pCurNode == NULL)
	{
		xml_free_docHandle(pHandle);
		return FALSE;
	}
	xml_remove_node(pCurNode);
	xml_save_handleToFile(fileName, pHandle, "UTF-8");
	//xmlNodePtr content = xmlNewText(BAD_CAST cszText);
	//xmlAddChild(pCurNode,content);
	xml_free_docHandle(pHandle);
	return TRUE;
}
int parseUploadTime(int select,char *xmlPath)
{
	char fromTimeRes[255] = {0};
	char toTimeRes[255] = {0};
	int TimeFlags = 0;
	int EnableStream = 0;	
	char fromHour[10] = {0};
	char toHour[10] = {0};
	char fromMinute[10] = {0};
	char toMinute[10] = {0};

	struct tm *ptm; 
	long ts; 
	int hourCur,minCur; 

	xml_get_intNode2("/root/TimeFlags/",&TimeFlags,xmlPath);
	xml_get_intNode2("/root/EnableStream/",&EnableStream,xmlPath);
		
	ts = time(NULL); 
	ptm = localtime(&ts); 
	hourCur = ptm-> tm_hour;               //时 
	minCur = ptm-> tm_min;                 //分 
	if(select == 1)  //1 FTP平台
	{
		if(1 != TimeFlags)
		{
			return 0;	//未开启定时
		}

		if( xml_get_charNode2("/root/TimeFrom/",fromTimeRes,xmlPath,sizeof(fromTimeRes)) == FALSE ||
				xml_get_charNode2("/root/TimeTo/",toTimeRes,xmlPath,sizeof(toTimeRes)) == FALSE)
		{
			return 0;
		}
	//	DEBUG_INFO("TimeFrom = %d,TimeTo = %d\n\n",strlen(fromTimeRes),strlen(toTimeRes));
		if((strlen(fromTimeRes) < 2) || (strlen(toTimeRes) < 2))
				return 0;
			
	}
	if(select == 2)
	{
		if(1 != TimeFlags)
		{
			return 0;	//未开启定时
		}

		//FTP服务器
		if(xml_get_charNode2("/root/TimeFrom/",fromTimeRes,xmlPath,sizeof(fromTimeRes)) == FALSE ||
		xml_get_charNode2("/root/TimeTo/",toTimeRes,xmlPath,sizeof(toTimeRes)) == FALSE)
		{
			return 0;
		}
		//DEBUG_INFO("TimeFrom = %d,TimeTo = %d\n\n",strlen(fromTimeRes),strlen(toTimeRes));
		if((strlen(fromTimeRes) < 2) || (strlen(toTimeRes) < 2))
			return 0;

	}
	if(select == 3)
	{
		if(1 != EnableStream)
		{
			return 0;	//未开启定时
		}

		//推流定时     1 不推流
		if(xml_get_charNode2("/root/TimeFromStream/",fromTimeRes,xmlPath,sizeof(fromTimeRes)) == FALSE ||
		xml_get_charNode2("/root/TimeToStream/",toTimeRes,xmlPath,sizeof(toTimeRes)) == FALSE)
		{
			return 0;
		}
		//DEBUG_INFO("TimeFrom = %d,TimeTo = %d\n\n",strlen(fromTimeRes),strlen(toTimeRes));
		if((strlen(fromTimeRes) < 1) || (strlen(toTimeRes) < 1))
		{
			return 0;
		}
	}

	sscanf(fromTimeRes,"%2s",fromHour);
	sscanf(fromTimeRes,"%*[^:]:%s",fromMinute);

	sscanf(toTimeRes,"%2s",toHour);
	sscanf(toTimeRes,"%*[^:]:%s",toMinute);

	if( ((atoi(fromHour)*60+atoi(fromMinute)) <= (hourCur*60+minCur) ) && ((atoi(toHour)*60+atoi(toMinute)-1) >= (hourCur*60+minCur))) //20:00 - 20:30  实际是20:00-20:29
	{
	//	DEBUG_INFO("******yes parseUploadTime  from : %s  to :%s******\n\n",fromTimeRes,toTimeRes);
	//	nslog(NS_INFO,"******yes parseUploadTime  from : %s  to :%s******\n\n",fromTimeRes,toTimeRes);
		return 0;
	}

//	DEBUG_INFO("******no parseUploadTime  from : %s  to :%s******\n\n",fromTimeRes,toTimeRes);
//	nslog(NS_INFO,"****** no parseUploadTime  from : %s  to :%s******\n\n",fromTimeRes,toTimeRes);
	return 1;
}


int stream_getDirFile(char *dirPath,char** fileName)
{		
		int index = 0;
		char indexBuf[64] = {0};
		DIR *dp = opendir(dirPath);
		if(dp == NULL)
		{
			perror("open dir failed\n");
			return -1;
		} 
		Xml_DocPtr_t pDocHandle = xml_init_doc();
		Xml_NodePtr_t pRootNode = xml_append_rootNode(pDocHandle, "root");
		Xml_NodePtr_t pBodyNode = xml_append_childNode(pRootNode, "MsgBody");
		Xml_NodePtr_t pChildNode = NULL;
		if((pDocHandle == NULL) ||(pRootNode ==NULL) || (pBodyNode == NULL))
		{
			goto EXIT;
		}

		while(1)
		{ 
	 		struct dirent *sdp = readdir(dp);
			if(sdp == NULL)
				break; 
			if(strcmp(sdp->d_name,".") == 0 || strcmp(sdp->d_name,"..") == 0)	
					continue;
			memset(indexBuf,0,sizeof(indexBuf));
			sprintf(indexBuf,"FileName%d",index);
			pChildNode = xml_append_childNode(pBodyNode,indexBuf);		
			xml_set_charNode(pChildNode,sdp->d_name);
			index++;
		}
		pChildNode = xml_append_childNode(pBodyNode,"Total");		
		xml_set_intNode(pChildNode,index);

		if(RET_SUCCESS != xml_handleTo_str(pDocHandle,"UTF-8",fileName))
		{
			goto EXIT;
		}
		
EXIT:
		if(pDocHandle != NULL)
		{
			xml_free_docHandle(pDocHandle);
			pDocHandle = NULL;
		} 
		closedir(dp); 
		return index;
} 



RetErr_t  xml_get_fileLen(const char * szFileName, int* nLen)
{
	if(szFileName == NULL || nLen == NULL)
	{
		return RET_PARAMER_ERR;
	}

	FILE *pFile= fopen(szFileName, "r");
	if(NULL == pFile)
	{
		perror("open xml fail\n");
		return RET_OPENFILE_FAILED;
	}
	fseek(pFile, 0, SEEK_END);
	*nLen = ftell(pFile);
	if(pFile)
	{
		fclose(pFile);
	}
	return RET_SUCCESS;
}





