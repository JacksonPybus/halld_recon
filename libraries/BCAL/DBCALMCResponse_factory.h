// $Id$
//
//    File: DBCALMCResponse_factory.h
// Created: Thu Nov 17 09:56:05 CST 2005
// Creator: gluexuser (on Linux hydra.phys.uregina.ca 2.4.20-8smp i686)
//

#ifndef _DBCALMCResponse_factory_
#define _DBCALMCResponse_factory_

#include "JANA/JFactory.h"
#include "JANA/JEventLoop.h"
#include "DBCALMCResponse.h"

class DBCALMCResponse_factory:public JFactory<DBCALMCResponse>{
	public:
		DBCALMCResponse_factory(){};
		~DBCALMCResponse_factory(){};
		const string toString(void);


	private:
		jerror_t evnt(JEventLoop *loop, int eventnumber);	///< Invoked via JEventProcessor virtual method
};

#endif // _DBCALMCResponse_factory_

