// $Id$
//
//    File: Dtrk_hit.h
// Created: Wed Jul 20 13:43:55 EDT 2005
// Creator: davidl (on Darwin wire129.jlab.org 7.8.0 powerpc)
//

#ifndef _Dtrk_hit_
#define _Dtrk_hit_

#include <math.h>

#include "DTrackHit.h"
#include "DMCTrackHit.h"
#include "JANA/jerror.h"
#include "GlueX.h"

class Dtrk_hit:public DTrackHit{
	public:
		Dtrk_hit(const DMCTrackHit* hit);
		virtual ~Dtrk_hit();
		// Note: className not defined so factory mechanism treats us
		// like DTrackHit objects
		//virtual const char* className(void){return static_className();}
		static const char* static_className(void){return "Dtrk_hit";}
		
		enum Dtrk_hit_Flags_t{
			USED				= 0x0001,
			IN_SEED			= 0x0002,
			ON_CIRCLE		= 0x0004,
			ON_LINE			= 0x0008,
			ON_TRACK			= 0x0010,
			IGNORE			= 0x0020,
		};
		unsigned int flags;
		float phi_circle;
		
		inline float DistXY(Dtrk_hit *hit){return sqrt(DistXY2(hit));}
		inline float DistXY2(Dtrk_hit *hit){
			float dx = x - hit->x;
			float dy = y - hit->y;
			return dx*dx + dy*dy;
		}
		inline float CosPhiXY(Dtrk_hit *a, Dtrk_hit *b){
			float adx = a->x - x;
			float ady = a->y - y;
			float bdx = b->x - x;
			float bdy = b->y - y;
			return (adx*bdx + ady*bdy)/sqrt((adx*adx+ady*ady)*(bdx*bdx+bdy*bdy));
		}
		
	protected:
	
	
	private:

};

#endif // _Dtrk_hit_

