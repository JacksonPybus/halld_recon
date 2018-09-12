// $Id: DFCALGeometry.h 19049 2015-07-16 18:31:31Z staylor $
//
//    File: DFCALGeometry.h
// Created: Wed Aug 24 10:09:27 EST 2005
// Creator: shepherd (on Darwin 129-79-159-16.dhcp-bl.indiana.edu 8.2.0 powerpc)
//

#ifndef _DFCALGeometry_
#define _DFCALGeometry_

#include <JANA/JFactory.h>
#include <JANA/JObject.h>
using namespace jana;

#include "DVector2.h"
#include "units.h"

class DFCALGeometry : public JObject {

  //#define kBlocksWide 59
  //#define kBlocksTall 59
#define kInnerBlocksWide 30
#define kInnerBlocksTall 30
  //#define kMaxChannels kBlocksWide * kBlocksTall * 2
// Do not forget to adjust below formula if number of blocks chage in any direction:
//   this is now used to convert from row/col to coordiantes y/x and back - MK
//#define kMidBlock (kBlocksWide-1)/2
#define kInnerMidBlock 15                     			
  //#define kBeamHoleSize 3

public:
	JOBJECT_PUBLIC(DFCALGeometry);

	DFCALGeometry();
	~DFCALGeometry(){}

	// these numbers are fixed for the FCAL as constructed
	// it probably doesn't make sense to retrieve them
	// from a database as they aren't going to change unless
	// the detector is reconstructed

	enum { kBlocksWide = 59 };
	enum { kBlocksTall = 59 };
	enum { kMaxChannels = kBlocksWide * kBlocksTall + kInnerBlocksWide*kInnerBlocksTall};
	enum { kMidBlock = ( kBlocksWide - 1 ) / 2 };
	enum { kBeamHoleSize = 3 };

	static double blockSize(int calor)  { 
	  if (calor==1) return 2.0*k_cm;
	  return 4.0157*k_cm; 
	}
	static double radius()  { return 1.20471*k_m; }
	static double blockLength(int calor)  { 
	  if (calor==1) return 18.0*k_cm;
	  return 45.0*k_cm; 
	}
	//	static double fcalFaceZ()  { return 625.3*k_cm; }

	//	static double fcalMidplane() { return fcalFaceZ + 0.5 * blockLength(0) ; }

  bool isBlockActive( int row, int column) const;
  int  numActiveBlocks() const { return m_numActiveBlocks; }

	
  DVector2 positionOnFace( int row, int column ) const;
  DVector2 positionOnFace( int channel ) const;
	
  int channel( int row, int column) const;

  int row   ( int channel) const { return m_row[channel];    }
  int column( int channel) const { return m_column[channel]; }
  
  // get row and column from x and y positions
  int row   ( float y, int calor ) const;
  int column( float x, int calor ) const;
  
  void toStrings(vector<pair<string,string> > &items)const{
    AddString(items, "kBlocksWide", "%d", kBlocksWide);
    AddString(items, "kBlocksTall", "%d", kBlocksTall);
    AddString(items, "kMaxChannels", "%d", kMaxChannels);
    AddString(items, "kBeamHoleSize", "%2.3f", kBeamHoleSize);
  }
	
private:

	bool   m_activeBlock[kBlocksTall][kBlocksWide][2];
	DVector2 m_positionOnFace[kBlocksTall][kBlocksWide][2];

	int    m_channelNumber[kBlocksTall][kBlocksWide][2];
	int    m_row[kMaxChannels];
	int    m_column[kMaxChannels];
	
	int    m_numActiveBlocks;
};

#endif // _DFCALGeometry_
