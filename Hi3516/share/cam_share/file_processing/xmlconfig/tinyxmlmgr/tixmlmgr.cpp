#ifndef __cplusplus
#define __cplusplus
#endif /* __cplusplus */

//#define _DEBUG

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <fcntl.h>

#include "tixmlconfig.h"
#include "../tinyxml/tinyxml.h"

#define BUFFSIZE	128 //8192
#define NAME_SIZE	64
#define VALUE_SIZE	64
#define LEAST_VALUE_SIZE	64

#define BAK_POSTFIX	"_bak"
SCODE Elem_SetValue(TiXmlElement *ptElem, const CHAR *szValue);

void xml_set_string(const char *szConfValue, void *pObj)
{
	if (NULL == szConfValue)
	{
		return;
	}
	sprintf((char *) pObj, "%s", szConfValue);
	return;
}

void xml_set_long(const char *szConfValue, void *pObj)
{
	if (NULL == szConfValue)
	{
		return;
	}
	
	char *pcEnd = NULL;
	//*(int *) pObj = atoi(szConfValue);
	*(long *) pObj = (long) strtol(szConfValue, &pcEnd, 10);
	return;
}

void xml_set_int(const char *szConfValue, void *pObj)
{
	if (NULL == szConfValue)
	{
		return;
	}
	
	char *pcEnd = NULL;
	*(int *) pObj = atoi(szConfValue);
	return;
}

void xml_set_short(const char *szConfValue, void *pObj)
{
	if (NULL == szConfValue)
	{
		return;
	}
	
	char *pcEnd = NULL;
	//*(int *) pObj = atoi(szConfValue);
	*(short *) pObj = (short) strtol(szConfValue, &pcEnd, 10);
	return;
}


SCODE TiXmlMgr_SafeReadFile(const char *szFileName)
{
	TiXmlDocument doc(szFileName);
	TiXmlDocument *pThis = &doc;
    TiXmlElement *pElement = NULL;
	
    bool loadOkay = pThis->LoadFile(szFileName);
    if ( loadOkay )
    {
		pElement = pThis->RootElement();
		pElement = pElement->FirstChildElement("write_flag");
    }
	
	if (loadOkay
		&& pElement != NULL
		&& 0 == strcmp(pElement->GetText(), "end"))
	{
		;
	}
	else
	{	
		fprintf(stderr, "%s:%d %s is destroyed,use backup file!\n", __FILE__, __LINE__, szFileName);
		
		const char *pBakFilePostfix = BAK_POSTFIX;
		char *szBakFileName = (char *)malloc(strlen(szFileName)+strlen(pBakFilePostfix)-1);
		if (!szBakFileName)
		{
			return S_FAIL;
		}
		
		strcpy(szBakFileName,szFileName);
		strcat(szBakFileName,pBakFilePostfix);

		if ( pThis->LoadFile(szBakFileName)
			&& pThis->SaveFile(szFileName)
			&& pThis->LoadFile(szFileName))
		{
			;
		}
		else
		{
			free(szBakFileName);
			fprintf(stderr, "%s:%d %s is destroyed and not be repaired!\n", __FILE__, __LINE__, szFileName);
			return S_FAIL;
		}
		free(szBakFileName);
	}

	return S_OK;
}

// just for sysmgr,use this to replace file with a temp file
SCODE TiXmlMgr_ReplaceFile(const char *szFileSrc, const char *szFileDst)
{
	// 1.BackupFile
	TiXmlDocument docDst(szFileDst);
	bool loadOkay = docDst.LoadFile();
	const char *pBakFilePostfix = BAK_POSTFIX;
	char *szBakFileName = (char *)malloc(strlen(szFileDst)+strlen(pBakFilePostfix)+1);
	if (!szBakFileName)
	{
		return S_FAIL;
	}
	
	strcpy(szBakFileName,szFileDst);
	strcat(szBakFileName,pBakFilePostfix);
	
	bool writeOkay = docDst.SaveFile(szBakFileName);
	//printf( "Could not load file '%s'. Error='%s'. Exiting.\n", szBakFileName, docDst.ErrorDesc() );
		
	free(szBakFileName);
	if ( !writeOkay )
	{
		return S_FAIL;
	}
	sync();

	// 2. ReplaceFile
	TiXmlDocument docSrc(szFileSrc);
	loadOkay = docSrc.LoadFile();
	
	if ( !loadOkay )
	{
		//printf( "Could not load file '%s'. Error='%s'. Exiting.\n", szFileName, old.ErrorDesc() );
		return S_FAIL;
	}
    
    if ( !docSrc.SaveFile(szFileDst) )
    {
        return S_FAIL;
    }

	return S_OK;

}

static TiXmlElement* GetElementNoAttr(TiXmlDocument *pThis, const char *szConfName)
{	
	char *p;
	char *szTmpName;

	DBPRINT3("%s:%s %s\n", __FILE__, __func__, szConfName)

	szTmpName = strdup(szConfName);
	if (szTmpName == NULL)
	{
		return NULL;
	}

	// get root
	TiXmlElement *pElement = pThis->RootElement();
	p = strtok(szTmpName, "/");
	if (strcmp(pElement->Value(), p))
	{
		fprintf(stderr, "%s:%d %s mismatch!\n", __FILE__, __LINE__, szConfName);
		free(szTmpName);
		return NULL;
	}	
	p = strtok(NULL, "/");
	while (p != NULL)
	{
        pElement = pElement->FirstChildElement(p);
		if (pElement == NULL)
		{
			fprintf(stderr, "%s:%d %s mismatch!\n", __FILE__, __LINE__, szConfName);
			free(szTmpName);
			return NULL;
		}
		p = strtok(NULL, "/");
	}
	free(szTmpName);
	return pElement;
}

/* Get ConfValue by Attribute in start element */
static TiXmlElement* GetElementbyAttr(TiXmlDocument *pThis, const char *szConfName)
{	
    
	char *p;
	char *szTmpName;

	/* attr var */	
	CHAR *pcTmp;
	CHAR szAttrTmp[NAME_SIZE];		
	CHAR *pcTmpAttrName;			//attr name
	CHAR *pcTmpAttrValue;			//attr value
	CHAR *pcNodeName;
	BOOL bAttrFound	= TRUE;	

	DBPRINT3("%s:%d GetConfAttr %s\n", __FILE__, __LINE__, szConfName)
	
	szTmpName = strdup(szConfName);
	if (szTmpName == NULL)
	{
		return NULL;
	}
	
	// get root
	p = strtok(szTmpName, "/");
	TiXmlElement *pElement = pThis->RootElement();
	if (strcmp(pElement->Value(), p))
	{
		fprintf(stderr, "%s:%d %s mismatch!\n", __FILE__, __LINE__, szConfName);
		free(szTmpName);
		return NULL;
	}	
	p = strtok(NULL, "/");
	
	while (p != NULL)
	{
		//attribute is not specified
		if (strchr(p, '@') == NULL)
		{
			pElement = pElement->FirstChildElement(p);		
		}
		//find attribute
		else 
		{
			//copy attr to new string
			memset(szAttrTmp, 0, NAME_SIZE);
			strcpy(szAttrTmp, p);
			DBPRINT3("%s:%d find %s\n", __FILE__, __LINE__, szAttrTmp)
			
			//new path string to be parsed
			pcTmp = p + strlen(p) -1;
			pcNodeName = strtok(szAttrTmp, " [");

			pElement = pElement->FirstChildElement(pcNodeName);
			
			//find sibling
			while (pElement != NULL)
			{	
				pcTmpAttrName = strtok(NULL, " @=");			
				while (pcTmpAttrName)
				{
					pcTmpAttrValue = strtok(NULL, " ']");
					bAttrFound = TRUE;
					DBPRINT4("%s:%d start to find %s=%s\n", __FILE__, __LINE__, pcTmpAttrName, pcTmpAttrValue)
					if(strcmp(pElement->Attribute(pcTmpAttrName),pcTmpAttrValue))
					{
						bAttrFound = FALSE;
						break;
					}
					pcTmpAttrName = strtok(NULL, " =]");
				}				
				if (!bAttrFound)
				{
					// reload to find sibling
					memset(szAttrTmp, 0, NAME_SIZE);
					strcpy(szAttrTmp, p);
					
					//new path string to be parsed
					pcNodeName = strtok(szAttrTmp, " [");
					pElement = pElement->NextSiblingElement(pcNodeName);
				}
				else
				{
					break;
				}
			}//while sibling
			
/*			//if specify attrnum > attrnum in ndoe , show err
			if (!pElement && pcTmpAttrName)
			{					
				fprintf(stderr, "%s:%d <%s> specified attribute number exceed!!\n", __FILE__, __LINE__, pcNodeName);
				free(szTmpName);
				return NULL;
			}
	*/		
			if (bAttrFound && pElement->FirstChildElement())
			{					
				//restart strtok string parsing, "f/" used to be fake parsed once
				pcTmp[0] = 'f';
				pcTmp[1] = '/';
				DBPRINT3("%s:%d start Newstr %s!!\n", __FILE__, __LINE__, p)
				p = strtok(pcTmp, "/");
			}
		}//else with attribute
		if (pElement == NULL)
		{
			fprintf(stderr, "%s:%d %s mismatch!\n", __FILE__, __LINE__, szConfName);
			free(szTmpName);
			return NULL;
		}
		p = strtok(NULL, "/");		
	}//while path	
	free(szTmpName);
	return pElement;
}//TiXmlMgr_GetConfValuebyAttr

const char* GetConfValueByHandle(TiXmlDocument *hObject, const char *szConfName)
{	
	
	TiXmlElement *ptElem = NULL;
	/*if there doesn't exit attr, call GetConfNoAttr*/
	if (! strchr(szConfName, '@'))
	{		
		ptElem = GetElementNoAttr(hObject, szConfName);
	}
	/*if there exits attr, call GetConfbyAttr*/
	else
	{
		ptElem = GetElementbyAttr(hObject, szConfName);
	}

	
	if (ptElem == NULL)
	{
		return NULL;
	}
	else	// do things after it's found
	{
		return ptElem->GetText();
	}
}

SCODE TiXmlMgr_GetConfValue(const char *szFileName, const char *szConfName, char *szConfValue)
{	
	TiXmlDocument doc(szFileName);
	bool loadOkay = doc.LoadFile();
	
	if ( !loadOkay )
	{
		//printf( "Could not load file '%s'. Error='%s'. Exiting.\n", szFileName, old.ErrorDesc() );
		return S_FAIL;
	}

	const char *szValue = GetConfValueByHandle(&doc, szConfName);
	if (szValue != NULL)
		strcpy(szConfValue, szValue);
	else
		return S_FAIL;
	
	return S_OK;
}

SCODE TiXmlMgr_GetMultiConfValue(const char *szFileName, TTiXmlMgrParsingMap *ptParsingMap)
{
	const char *szConfValue;
	
	TiXmlDocument doc(szFileName);
	bool loadOkay = doc.LoadFile();
	
	if ( !loadOkay )
	{
	//	printf( "Could not load file '%s'. Error='%s'. Exiting.\n", szFileName, old.ErrorDesc() );
		return S_FAIL;
	}

	while (ptParsingMap->szName != NULL)
	{
		if ((szConfValue = GetConfValueByHandle(&doc, ptParsingMap->szName)) != NULL
			&& ptParsingMap->pfnParsingAction != NULL)
		{
			
			ptParsingMap->pfnParsingAction (szConfValue, ptParsingMap->pObject);
		}
		ptParsingMap++;
	}
	return S_OK;
}

SCODE Elem_SetValue(TiXmlElement *ptElem, const CHAR *szValue)
{
	assert(ptElem);
	DBPRINT4("%s:%d SetValue %s %s\n", __FILE__, __LINE__, ptElem->Value(),szValue);
	
	TiXmlText newText(szValue);

	TiXmlNode * node = ptElem->FirstChild();
	if (node)
	{
		ptElem->ReplaceChild( node, newText );
	}
	else
	{
		//DBPRINT0("node == NULL when Element has no text.\n");
		ptElem->InsertEndChild( newText );	
	}
	return S_OK;
}

SCODE SetConfValueBegin(TiXmlDocument *pThis, const char *szFileName)
{	
	// 1.BackupFile
	const char *pBakFilePostfix = BAK_POSTFIX;
	char *szBakFileName = (char *)malloc(strlen(szFileName)+strlen(pBakFilePostfix)+1);
	if (!szBakFileName)
	{
		return S_FAIL;
	}
	
	strcpy(szBakFileName,szFileName);
	strcat(szBakFileName,pBakFilePostfix);
	
	bool writeOkay = pThis->SaveFile(szBakFileName);
	free(szBakFileName);
	if ( !writeOkay )
	{
		return S_FAIL;
	}
	sync();
	
	// 2.write_flag->begin
/*	TiXmlElement *pElemFlg = pThis->RootElement();
	pElemFlg = pElemFlg->FirstChildElement("write_flag");
	if (pElemFlg == NULL
		|| S_OK != Elem_SetValue(pElemFlg, "begin")
		|| !pThis->SaveFile()
		)
	{
		
		fprintf(stderr, "%s:%d fail to set write_flag!\n", __FILE__, __LINE__);
		return S_FAIL;
	}
	sync();
*/
	return S_OK;		
}

SCODE SetConfValueEnd(TiXmlDocument *pThis)
{	
	TiXmlElement *pElemFlg = pThis->RootElement();
/*	pElemFlg = pElemFlg->FirstChildElement("write_flag");
	if (pElemFlg == NULL)
	{
		
		fprintf(stderr, "%s:%d fail to set write_flag!\n", __FILE__, __LINE__);
		return S_FAIL;
	}

	// 4.write_flag->end
	if (S_OK != Elem_SetValue(pElemFlg, "end")
		|| !pThis->SaveFile())
	{
		
		fprintf(stderr, "%s:%d fail to set write_flag!\n", __FILE__, __LINE__);
		return S_FAIL;
	}
	sync();
*/
	if (!pThis->SaveFile())
	{
		
		fprintf(stderr, "%s:%d fail to set write_flag!\n", __FILE__, __LINE__);
		return S_FAIL;
	}

	return S_OK;		
}

SCODE SetConfValueByHandle(TiXmlDocument *pThis, const char *szConfName, const char *szConfValue)
{	
	TiXmlElement *ptElem;
	if (strchr(szConfName, '@') == NULL)
	{
		ptElem = GetElementNoAttr(pThis, szConfName);
	}
	else
	{
		ptElem = GetElementbyAttr(pThis, szConfName);
	}
	
	if (ptElem == NULL)
	{
		return S_FAIL;
	}
	else
	{
		// 3.SetConfValue
		if (S_OK != Elem_SetValue(ptElem, szConfValue))
		{
			return S_FAIL;
		}
	}
	

	return S_OK;		
}

SCODE TiXmlMgr_SetConfValue(const char *szFileName, const char *szConfName, const char *szConfValue)
{	
	TiXmlDocument doc(szFileName);
	bool loadOkay = doc.LoadFile();
	
	if ( !loadOkay )
	{
		//printf( "Could not load file '%s'. Error='%s'. Exiting.\n", szFileName, old.ErrorDesc() );
		return S_FAIL;
	}

	if(SetConfValueBegin(&doc, szFileName) != S_OK
		|| SetConfValueByHandle(&doc, szConfName, szConfValue) != S_OK
		|| SetConfValueEnd(&doc) != S_OK)
	{
		return S_FAIL;
	}
	
	return S_OK;

}//TiXmlMgr_SetConfValue

SCODE TiXmlMgr_SetMultiConfValue(const char *szFileName, int iConfNum, const char *szConfNames[], const char *szConfValues[])
{
	
	TiXmlDocument doc(szFileName);
	bool loadOkay = doc.LoadFile();
	
	if ( !loadOkay )
	{
		//printf( "Could not load file '%s'. Error='%s'. Exiting.\n", szFileName, old.ErrorDesc() );
		return S_FAIL;
	}

	if(SetConfValueBegin(&doc, szFileName) != S_OK)
	{
		return S_FAIL;
	}
	
	int i;
	// TODO : a more efficient method
	for (i=0; i<iConfNum; i++)
	{
		if (szConfNames[i] && szConfValues[i])
		{
			// printf("%s %d : :%s: :%s:\n", __FILE__, __LINE__, szConfNames[i], szConfValues[i]);
			if (SetConfValueByHandle(&doc, szConfNames[i], szConfValues[i]) != S_OK)
			{
				return S_FAIL;
			}
		}
	}

	if(SetConfValueEnd(&doc) != S_OK)
	{
		return S_FAIL;
	}

	return S_OK;
}

//  return a pointer to a vector of char pointers, each attribute are name and value pairs. 
const char** GetConfAttr(TiXmlElement *pElement)
{	
	const char** ppAtts = NULL;

	TiXmlAttribute* ptAttr = pElement->FirstAttribute();
	if (ptAttr != NULL)
	{
		TiXmlAttribute* ptAttrTemp = ptAttr;
		int iAttrNum = 0;
		for (; ptAttrTemp!=NULL; ptAttrTemp=ptAttrTemp->Next())
		{
			iAttrNum++;
		}
//		DBPRINT3("%s:%d %d Attribute found!!\n", __FILE__, __LINE__, iAttrNum);

		ppAtts = new const char*[iAttrNum*2 + 1];

		ptAttrTemp = ptAttr;
		iAttrNum = 0;
		for (; ptAttrTemp!=NULL; ptAttrTemp=ptAttrTemp->Next())
		{
			ppAtts[iAttrNum] = ptAttrTemp->Name();
			ppAtts[iAttrNum+1] = ptAttrTemp->Value();

//			DBPRINT4("%s:%d Attribute: %s=%s\n", __FILE__, __LINE__, ppAtts[iAttrNum],ppAtts[iAttrNum+1]);
			
			iAttrNum += 2;
		}
		ppAtts[iAttrNum] = NULL;	// end
	}

	return ppAtts;
}

void Elem_CallBack(TiXmlElement *pElement, const char *szAbsoluteName, HANDLE hUsrData, TTiXmlMgrTreeMap *ptTreeMap)
{
    while (ptTreeMap->szElemName != NULL)
    {
		if (strcmp(ptTreeMap->szElemName, szAbsoluteName) == 0)
		{
//			DBPRINT3("%s:%d TiXmlMgr_Elem_CallBack %s\n", __FILE__, __LINE__, szAbsoluteName);

	        if (ptTreeMap->pfStartHdl != NULL)
	        {
				const char** ppAtts = GetConfAttr(pElement);
	            ptTreeMap->pfStartHdl(hUsrData, NULL, ppAtts);
				if (ppAtts != NULL)
					delete[] ppAtts;
	        }

	        if (ptTreeMap->pfChDataHdl != NULL)
	        {
				const char *element_vlue = pElement->GetText();
				unsigned int element_len = (NULL == element_vlue) ? 0 : strlen(element_vlue);
	            ptTreeMap->pfChDataHdl(hUsrData, element_vlue, element_len);
	        }

	        if (ptTreeMap->pfEndHdl != NULL)
	            ptTreeMap->pfEndHdl(hUsrData, NULL);
		}

		ptTreeMap++;
    }
}

SCODE TreeTraverse(TiXmlElement *pElement, const char *szParentAbsoluteName, HANDLE hUsrData, TTiXmlMgrTreeMap *ptTreeMap)
{
	const char* szElementName = pElement->Value();
	char *szSubAbsoluteName = new char[2 + strlen(szParentAbsoluteName) + strlen(szElementName)];
	strcpy(szSubAbsoluteName, szParentAbsoluteName);
	strcat(szSubAbsoluteName, szElementName);
	
	DBPRINT4("%s:%d TiXmlMgr_TreeTraverse: %s %s\n", __FILE__, __LINE__, szParentAbsoluteName, szSubAbsoluteName);
	
	Elem_CallBack(pElement, szSubAbsoluteName, hUsrData, ptTreeMap);
	
	// Child
	if ((pElement = pElement->FirstChildElement()) != NULL)
	{		
		strcat(szSubAbsoluteName, "/");
		TreeTraverse(pElement, szSubAbsoluteName, hUsrData, ptTreeMap);
		
		// Child's brother
		while ((pElement = pElement->NextSiblingElement()) != NULL)
		{
			TreeTraverse(pElement, szSubAbsoluteName, hUsrData, ptTreeMap);
		}
	}
	delete[] szSubAbsoluteName;
	
	return S_OK;
}

SCODE TiXmlMgr_UsrDefFunc(const char *szFileName, HANDLE hUsrData, TTiXmlMgrTreeMap*ptTreeMap)
{	
	TiXmlDocument doc(szFileName);
	bool loadOkay = doc.LoadFile();
	
	if ( !loadOkay )
	{
		//printf( "Could not load file '%s'. Error='%s'. Exiting.\n", szFileName, old.ErrorDesc() );
		return S_FAIL;
	}
	
	// get root
	TiXmlElement *pElement = doc.RootElement();
	if (pElement == NULL)
	{
		return S_FAIL;
	}

	// ptTreeMap start with "root/..." not "/root/..."
	TreeTraverse(pElement, "", hUsrData, ptTreeMap);

	return S_OK;
}

