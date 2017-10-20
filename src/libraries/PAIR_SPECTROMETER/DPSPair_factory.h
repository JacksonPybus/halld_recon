// $Id$
//
//    File: DPSPair_factory.h
// Created: Fri Mar 20 07:51:31 EDT 2015
// Creator: nsparks (on Linux cua2.jlab.org 2.6.32-431.5.1.el6.x86_64 x86_64)
//

#ifndef _DPSPair_factory_
#define _DPSPair_factory_

#include <JANA/JFactory.h>
#include "DPSPair.h"

class DPSPair_factory:public jana::JFactory<DPSPair>{
 public:
  DPSPair_factory(){};
  ~DPSPair_factory(){};

  double DELTA_T_CLUST_MAX;
  double DELTA_T_PAIR_MAX;


  typedef struct {
    int     column;
    double  energy;

    double  integral;
    double  pulse_peak;
    double  time;
    int     used;       
  } tile;

  typedef struct {

    int ntiles;

    vector<int> hit_index;

    double  energy;
    double  time;

    int     column;
    double  integral;
    double  pulse_peak;
    double  time_tile;

  } clust;


  vector<tile> tiles_left;
  vector<tile> tiles_right;

  vector<clust> clust_left;
  vector<clust> clust_right;


  static bool SortByTile(const tile &tile1, const tile &tile2);

 private:
  jerror_t init(void);					  	    ///< Called once at program start.
  jerror_t brun(jana::JEventLoop *eventLoop, int32_t runnumber);    ///< Called everytime a new run number is detected.
  jerror_t evnt(jana::JEventLoop *eventLoop, uint64_t eventnumber);	///< Called every event.
  jerror_t erun(void);						///< Called everytime run number changes, provided brun has been called.
  jerror_t fini(void);						///< Called after last event of last event source has been processed.
};

#endif // _DPSPair_factory_

