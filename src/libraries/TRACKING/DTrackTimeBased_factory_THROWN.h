// $Id$
//
//    File: DTrackTimeBased_factory_THROWN.h
// Created: MWed Nov 18 06:25:19 EST 2009
// Creator: davidl (on Darwin Amelia.local 9.8.0 i386)
//

#ifndef _DTrackTimeBased_factory_THROWN_
#define _DTrackTimeBased_factory_THROWN_

#include <JANA/JFactory.h>
#include <TRACKING/DReferenceTrajectory.h>
#include <HDGEOMETRY/DRootGeom.h>
#include <HDGEOMETRY/DGeometry.h>
#include <HDGEOMETRY/DMagneticFieldMap.h>
#include "DTrackTimeBased.h"
#include "PID/DParticleID.h"

class DTrackFitter;
class DTrackHitSelector;
class DParticleID;

class DTrackTimeBased_factory_THROWN:public JFactory<DTrackTimeBased>{
	public:
		DTrackTimeBased_factory_THROWN();
		~DTrackTimeBased_factory_THROWN(){};
		const char* Tag(void){return "THROWN";}

	private:
		//jerror_t init(void);						///< Called once at program start.
		jerror_t brun(JEventLoop *eventLoop, int32_t runnumber);	///< Called everytime a new run number is detected.
		jerror_t evnt(JEventLoop *eventLoop, uint64_t eventnumber);	///< Called every event.
		//jerror_t erun(void);						///< Called everytime run number changes, provided brun has been called.
		//jerror_t fini(void);						///< Called after last event of last event source has been processed.
		
		DTrackFitter *fitter;
		const DTrackHitSelector *hitselector;
		const DParticleID* dParticleID;
		vector<DReferenceTrajectory*> rt_pool;
		
		DRootGeom *RootGeom;
		DGeometry *geom;
		DMagneticFieldMap *bfield;
		
		vector<vector<DCDCWire *> > cdcwires;
	  	vector<double> cdc_rmid;
	  	double cdc_endplate_z;

  		// FDC wire info
  		vector<double>fdc_z_wires;
  		double fdc_package_size;
  		double fdc_rmax;
  		vector<double> fdc_rmin_packages;

};

#endif // _DTrackTimeBased_factory_THROWN_

