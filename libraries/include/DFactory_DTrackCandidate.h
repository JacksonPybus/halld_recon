// $Id$
//
//    File: DFactory_DTrackCandidate.h
// Created: Mon Jul 18 15:23:04 EDT 2005
// Creator: davidl (on Darwin wire129.jlab.org 7.8.0 powerpc)
//

#ifndef _DFactory_DTrackCandidate_
#define _DFactory_DTrackCandidate_

#include <TH1.h>

#include "DFactory.h"
#include "DQuickFit.h"
#include "DTrackCandidate.h"
#include "../TRACKING/Dtrk_hit.h"

class DGeometry;
class DMagneticFieldMap;

class DFactory_DTrackCandidate:public DFactory<DTrackCandidate>{
	public:
		DFactory_DTrackCandidate();
		~DFactory_DTrackCandidate(){};
		const string toString(void);
		static void Fill_phi_circle(vector<Dtrk_hit*> hits, float x0, float y0);
		
		vector<Dtrk_hit*>& Get_trkhits(void){return trkhits;}
		vector<vector<Dtrk_hit*> >& Get_dbg_in_seed(void){return dbg_in_seed;}
		vector<vector<Dtrk_hit*> >& Get_dbg_hoc(void){return dbg_hoc;}
		vector<vector<Dtrk_hit*> >& Get_dbg_hol(void){return dbg_hol;}
		vector<vector<Dtrk_hit*> >& Get_dbg_hot(void){return dbg_hot;}
		vector<DQuickFit*>& Get_dbg_seed_fit(void){return dbg_seed_fit;}
		vector<DQuickFit*>& Get_dbg_track_fit(void){return dbg_track_fit;}
		vector<int>& Get_dbg_seed_index(void){return dbg_seed_index;}
		vector<TH1F*>& Get_dbg_phiz_hist(void){return dbg_phiz_hist;}
		vector<int>& Get_dbg_phiz_hist_seed(void){return dbg_phiz_hist_seed;}
		vector<TH1F*>& Get_dbg_zvertex_hist(void){return dbg_zvertex_hist;}
		vector<int>& Get_dbg_zvertex_hist_seed(void){return dbg_zvertex_hist_seed;}
		vector<float>& Get_dbg_z_vertex(void){return dbg_z_vertex;}
		vector<float>& Get_dbg_phizangle(void){return dbg_phizangle;}

	private:
		derror_t brun(DEventLoop *loop, int runnumber);
		derror_t evnt(DEventLoop *loop, int eventnumber);	///< Invoked via DEventProcessor virtual method
		derror_t fini(void);	///< Invoked via DEventProcessor virtual method
		void ClearEvent(void);
		void GetTrkHits(DEventLoop *loop);
		int FindSeed(void);
		int TraceSeed(Dtrk_hit *hit);
		Dtrk_hit* FindClosestXY(Dtrk_hit *hit);
		int FitSeed(void);
		int FindLineHits(void);
		int FindPhiZAngle(void);
		int FindZvertex(void);
		int FitTrack(void);
		int MarkTrackHits(DTrackCandidate* trackcandidate, DQuickFit *fit);
		inline void ChopSeed(void){if(hits_in_seed.size()>0)hits_in_seed[0]->flags |= Dtrk_hit::IGNORE;}


		void DumpHits(int current_seed_number, string stage);
		
		const DGeometry* dgeom;
		const DMagneticFieldMap *bfield;

		vector<Dtrk_hit*> trkhits; // sorted by z
		vector<Dtrk_hit*> trkhits_r_sorted; // sorted by dist. from beam line
		vector<Dtrk_hit*> hits_in_seed;
		vector<Dtrk_hit*> hits_on_circle;
		vector<Dtrk_hit*> hits_on_line;
		vector<Dtrk_hit*> hits_on_track;
		vector<vector<Dtrk_hit*> > dbg_in_seed;
		vector<vector<Dtrk_hit*> > dbg_hoc;
		vector<vector<Dtrk_hit*> > dbg_hol;
		vector<vector<Dtrk_hit*> > dbg_hot;
		vector<DQuickFit*> dbg_seed_fit;
		vector<DQuickFit*> dbg_track_fit;
		vector<int> dbg_seed_index;
		vector<TH1F*> dbg_phiz_hist;
		vector<int> dbg_phiz_hist_seed;
		vector<TH1F*> dbg_zvertex_hist;
		vector<int> dbg_zvertex_hist_seed;
		vector<float> dbg_phizangle;
		vector<float> dbg_z_vertex;
		int runnumber;
		int eventnumber;
		float MAX_SEED_DIST;
		float MAX_SEED_DIST2;
		float XY_NOISE_CUT;
		float XY_NOISE_CUT2;
		unsigned int MAX_SEED_HITS;
		float MAX_CIRCLE_DIST;
		float MAX_PHI_Z_DIST;
		unsigned int MAX_DEBUG_BUFFERS;
		float TARGET_Z_MIN;
		float TARGET_Z_MAX;
		float phizangle_bin_size;
		float z_vertex_bin_size;
		float x0,y0,r0;
		float phizangle, z_vertex;
		float phizangle_min, phizangle_max;
		string TRACKHIT_SOURCE;
		float MIN_HIT_Z, MAX_HIT_Z;
		bool EXCLUDE_STEREO;
		
		TH1F *phizangle_hist, *zvertex_hist, *phi_relative;
		
};

#endif // _DFactory_DTrackCandidate_

