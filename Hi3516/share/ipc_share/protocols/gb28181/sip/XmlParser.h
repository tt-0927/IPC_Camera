#pragma once
#include "SipType.h"
#include "pugixml.hpp"
namespace SIP
{
	namespace XmlParser
	{
		bool Parse(const char *data, int len, pugi::xml_document &doc);
		bool ParseHeader(manscdp_msgbody_header_t &header, pugi::xml_document &doc);
	}

}