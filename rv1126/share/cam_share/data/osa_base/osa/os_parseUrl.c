

#include "os_parseUrl.h"
#include "os.h"


int os_parsing_RTSPURL(char const* url, char* username, char* password, char* address,int* portNum, char* path)
{
    //Parse the URL as "rtsp://[<username>[:<password>]@]<server-address-or-name>[:<port>][/<path>]"
	Uint32 const prefixLength = 7;
    char const* from = &url[prefixLength];
    char const* tmpPos;

    if ((tmpPos = strchr(from, '@')) != NULL)
	{
	// We found <username> (and perhaps <password>).
        char const* usernameStart = from;
        char const* passwordStart = NULL;
        char const* p = tmpPos;

        if ((tmpPos = strchr(from, ':')) != NULL && tmpPos < p)
		{
            passwordStart = tmpPos;
            Uint32 passwordLen = p - passwordStart;
            strncpy(password, passwordStart, passwordLen);
            password[passwordLen] = '\0'; //Set the ending character.
        }

        Uint32 usernameLen = 0;
        if (passwordStart != NULL)
        {
            usernameLen = tmpPos - usernameStart;
        } else
        {
            usernameLen = p - usernameStart;
        }
        strncpy(username, usernameStart, usernameLen);
        username[usernameLen] = '\0';  //Set the ending character.

        from = p + 1; // skip the '@'
    }

    const char* pathStart = NULL;
    if ((tmpPos = strchr(from, '/')) != NULL)
    {
    	Uint32 pathLen = strlen(tmpPos + 1);  //Skip '/'
        memcpy(path, tmpPos + 1, pathLen + 1);
//        strncpy(path, tmpPos + 1, pathLen + 1);
        pathStart = tmpPos;
    }

    // Next, will parse the address and port.
    tmpPos = strchr(from, ':');
    if (tmpPos == NULL)
    {
        if (pathStart == NULL)
        {
        	Uint32 addressLen = strlen(from);
        	memcpy(address, from, addressLen + 1);
//          strncpy(address, from, addressLen + 1);  //Already include '\0'
        } else
        {
        	Uint32 addressLen = pathStart - from;
            strncpy(address, from, addressLen);
            address[addressLen] = '\0';   //Set the ending character.
        }
        *portNum = 554; // Has not the specified port, and will use the default value

    } else if (tmpPos != NULL)
    {
    	Uint32 addressLen = tmpPos - from;
        strncpy(address, from, addressLen);
        address[addressLen] = '\0';  //Set the ending character.
        *portNum = strtoul(tmpPos + 1, NULL, 10);
    }

    return 1;
}



