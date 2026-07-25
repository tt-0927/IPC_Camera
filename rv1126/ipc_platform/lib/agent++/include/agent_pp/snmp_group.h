/*_############################################################################
  _## 
  _##  AGENT++ 4.5 - snmp_group.h  
  _## 
  _##  Copyright (C) 2000-2021  Frank Fock and Jochen Katz (agentpp.com)
  _##  
  _##  Licensed under the Apache License, Version 2.0 (the "License");
  _##  you may not use this file except in compliance with the License.
  _##  You may obtain a copy of the License at
  _##  
  _##      http://www.apache.org/licenses/LICENSE-2.0
  _##  
  _##  Unless required by applicable law or agreed to in writing, software
  _##  distributed under the License is distributed on an "AS IS" BASIS,
  _##  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  _##  See the License for the specific language governing permissions and
  _##  limitations under the License.
  _##  
  _##########################################################################*/


#ifndef snmp_group_h_
#define snmp_group_h_


#include <string.h>

#include <agent_pp/agent++.h>
#include <agent_pp/mib.h>


#define oidSnmpGroup			"1.3.6.1.2.1.11"
#define oidSnmpInPkts			"1.3.6.1.2.1.11.1.0"
#define oidSnmpOutPkts			"1.3.6.1.2.1.11.2.0"
#define oidSnmpInBadVersions		"1.3.6.1.2.1.11.3.0"
#define oidSnmpInBadCommunityNames 	"1.3.6.1.2.1.11.4.0"
#define oidSnmpInBadCommunityUses  	"1.3.6.1.2.1.11.5.0"
#define oidSnmpInASNParseErrs      	"1.3.6.1.2.1.11.6.0"
#define oidSnmpInTooBigs	       	"1.3.6.1.2.1.11.8.0"
#define oidSnmpInNoSuchNames       	"1.3.6.1.2.1.11.9.0"
#define oidSnmpInBadValues	       	"1.3.6.1.2.1.11.10.0"
#define oidSnmpInReadOnlys	       	"1.3.6.1.2.1.11.11.0"
#define oidSnmpInGenErrs	       	"1.3.6.1.2.1.11.12.0"
#define oidSnmpInTotalReqVars      	"1.3.6.1.2.1.11.13.0"
#define oidSnmpInTotalSetVars      	"1.3.6.1.2.1.11.14.0"
#define oidSnmpInGetRequests       	"1.3.6.1.2.1.11.15.0"
#define oidSnmpInGetNexts	       	"1.3.6.1.2.1.11.16.0"
#define oidSnmpInSetRequests       	"1.3.6.1.2.1.11.17.0"
#define oidSnmpInGetResponses      	"1.3.6.1.2.1.11.18.0"
#define oidSnmpInTraps	       	        "1.3.6.1.2.1.11.19.0"
#define oidSnmpOutTooBigs	       	"1.3.6.1.2.1.11.20.0"
#define oidSnmpOutNoSuchNames      	"1.3.6.1.2.1.11.21.0"
#define oidSnmpOutBadValues		"1.3.6.1.2.1.11.22.0"
#define oidSnmpOutGenErrs	       	"1.3.6.1.2.1.11.24.0"
#define oidSnmpOutGetRequests      	"1.3.6.1.2.1.11.25.0"
#define oidSnmpOutGetNexts	       	"1.3.6.1.2.1.11.26.0"
#define oidSnmpOutSetRequests      	"1.3.6.1.2.1.11.27.0"
#define oidSnmpOutGetResponses     	"1.3.6.1.2.1.11.28.0"
#define oidSnmpOutTraps			"1.3.6.1.2.1.11.29.0"
#define oidSnmpEnableAuthenTraps	"1.3.6.1.2.1.11.30.0"
#define oidSnmpSilentDrops	       	"1.3.6.1.2.1.11.31.0"
#define oidSnmpProxyDrops		"1.3.6.1.2.1.11.32.0"

#define enableAuthTraps			1
#define disableAuthTraps	        2

#ifdef AGENTPP_NAMESPACE
namespace Agentpp {
    using namespace Snmp_pp;
#endif


class AGENTPP_DECL snmpInPkts: public Counter32MibLeaf {

public:
	snmpInPkts(): Counter32MibLeaf(oidSnmpInPkts) { }
};


class AGENTPP_DECL snmpOutPkts: public Counter32MibLeaf {

public:
	snmpOutPkts(): Counter32MibLeaf(oidSnmpOutPkts) { }
};


class AGENTPP_DECL snmpInBadVersions: public Counter32MibLeaf {

public:
	snmpInBadVersions(): Counter32MibLeaf(oidSnmpInBadVersions) { }
};


class AGENTPP_DECL snmpInBadCommunityNames: public Counter32MibLeaf {

public:
	snmpInBadCommunityNames(): Counter32MibLeaf(oidSnmpInBadCommunityNames) { }
};


class AGENTPP_DECL snmpInBadCommunityUses: public Counter32MibLeaf {

public:
	snmpInBadCommunityUses(): Counter32MibLeaf(oidSnmpInBadCommunityUses) { }
};


class AGENTPP_DECL snmpInASNParseErrs: public Counter32MibLeaf {

public:
	snmpInASNParseErrs(): Counter32MibLeaf(oidSnmpInASNParseErrs) { }
};


class AGENTPP_DECL snmpInTooBigs: public Counter32MibLeaf {

public:
	snmpInTooBigs(): Counter32MibLeaf(oidSnmpInTooBigs) { }
};


class AGENTPP_DECL snmpInNoSuchNames: public Counter32MibLeaf {

public:
	snmpInNoSuchNames(): Counter32MibLeaf(oidSnmpInNoSuchNames) { }
};


class AGENTPP_DECL snmpInBadValues: public Counter32MibLeaf {

public:
	snmpInBadValues(): Counter32MibLeaf(oidSnmpInBadValues) { }
};


class AGENTPP_DECL snmpInReadOnlys: public Counter32MibLeaf {

public:
	snmpInReadOnlys(): Counter32MibLeaf(oidSnmpInReadOnlys) { }
};


class AGENTPP_DECL snmpInGenErrs: public Counter32MibLeaf {

public:
	snmpInGenErrs(): Counter32MibLeaf(oidSnmpInGenErrs) { }
};


class AGENTPP_DECL snmpInTotalReqVars: public Counter32MibLeaf {

public:
	snmpInTotalReqVars(): Counter32MibLeaf(oidSnmpInTotalReqVars) { }
};


class AGENTPP_DECL snmpInTotalSetVars: public Counter32MibLeaf {

public:
	snmpInTotalSetVars(): Counter32MibLeaf(oidSnmpInTotalSetVars) { }
};


class AGENTPP_DECL snmpInGetRequests: public Counter32MibLeaf {

public:
	snmpInGetRequests(): Counter32MibLeaf(oidSnmpInGetRequests) { }
};


class AGENTPP_DECL snmpInGetNexts: public Counter32MibLeaf {

public:
	snmpInGetNexts(): Counter32MibLeaf(oidSnmpInGetNexts) { }
};


class AGENTPP_DECL snmpInSetRequests: public Counter32MibLeaf {

public:
	snmpInSetRequests(): Counter32MibLeaf(oidSnmpInSetRequests) { }
};


class AGENTPP_DECL snmpInGetResponses: public Counter32MibLeaf {

public:
	snmpInGetResponses(): Counter32MibLeaf(oidSnmpInGetResponses) { }
};


class AGENTPP_DECL snmpInTraps: public Counter32MibLeaf {

public:
	snmpInTraps(): Counter32MibLeaf(oidSnmpInTraps) { }
};


class AGENTPP_DECL snmpOutTooBigs: public Counter32MibLeaf {

public:
	snmpOutTooBigs(): Counter32MibLeaf(oidSnmpOutTooBigs) { }
};


class AGENTPP_DECL snmpOutNoSuchNames: public Counter32MibLeaf {

public:
	snmpOutNoSuchNames(): Counter32MibLeaf(oidSnmpOutNoSuchNames) { }
};


class AGENTPP_DECL snmpOutBadValues: public Counter32MibLeaf {

public:
	snmpOutBadValues(): Counter32MibLeaf(oidSnmpOutBadValues) { }
};


class AGENTPP_DECL snmpOutGenErrs: public Counter32MibLeaf {

public:
	snmpOutGenErrs(): Counter32MibLeaf(oidSnmpOutGenErrs) { }
};


class AGENTPP_DECL snmpOutGetRequests: public Counter32MibLeaf {

public:
	snmpOutGetRequests(): Counter32MibLeaf(oidSnmpOutGetRequests) { }
};


class AGENTPP_DECL snmpOutGetNexts: public Counter32MibLeaf {

public:
	snmpOutGetNexts(): Counter32MibLeaf(oidSnmpOutGetNexts) { }
};


class AGENTPP_DECL snmpOutSetRequests: public Counter32MibLeaf {

public:
	snmpOutSetRequests(): Counter32MibLeaf(oidSnmpOutSetRequests) { }
};


class AGENTPP_DECL snmpOutGetResponses: public Counter32MibLeaf {

public:
	snmpOutGetResponses(): Counter32MibLeaf(oidSnmpOutGetResponses) { }
};


class AGENTPP_DECL snmpOutTraps: public Counter32MibLeaf {

public:
	snmpOutTraps(): Counter32MibLeaf(oidSnmpOutTraps) { }
};

class AGENTPP_DECL snmpSilentDrops: public Counter32MibLeaf {

public:
	snmpSilentDrops(): Counter32MibLeaf(oidSnmpSilentDrops) { }
};


class AGENTPP_DECL snmpProxyDrops: public Counter32MibLeaf {

public:
	snmpProxyDrops(): Counter32MibLeaf(oidSnmpProxyDrops) { }
};


/**
 *  snmpEnableAuthenTraps
 *
"Indicates whether the SNMP entity is permitted to generate
 authenticationFailure traps. The value of this object
 overrides any configuration information; as such, it
 provides a means whereby all authenticationFailure traps may
 be disabled.

 Note that it is strongly recommended that this object be
 stored in non-volatile memory so that it remains constant
 across re-initializations of the network management system."
 */


class AGENTPP_DECL snmpEnableAuthenTraps: public MibLeaf {

public:
	snmpEnableAuthenTraps();
	virtual ~snmpEnableAuthenTraps();

	static snmpEnableAuthenTraps* instance;
        /**
         * Get the pointer to the snmpEnableAuthenTraps associated with the
         * provided Mib instance. 
         * @param mib a Mib reference
         * @return 
         *    a pointer to the entry if available in the Mib or the static 
         *    instance pointer as fallback.
         * @since 4.3.0
         */
        static snmpEnableAuthenTraps* get_instance(Mib* mib) {
            Oidx oid(oidSnmpEnableAuthenTraps);
            snmpEnableAuthenTraps* entry = (snmpEnableAuthenTraps*)mib->get(oid);
            return (entry) ? entry : instance;
        }
        
        
	long			get_state();
	virtual bool    	value_ok(const Vbx&);
};


/**********************************************************************
 * 
 *  class snmpGroup
 *
 **********************************************************************/


class AGENTPP_DECL snmpGroup: public MibGroup {

public:
	snmpGroup();
};
#ifdef AGENTPP_NAMESPACE
}
#endif

#endif
