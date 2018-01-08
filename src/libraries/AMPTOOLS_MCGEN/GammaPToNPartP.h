#if !defined(GAMMAPTONPARTP)
#define GAMMAPTONPARTP

/*
 *  GammaPToNPartP.h
 *   by Igor Senderovich
 *  structure based on GammaToXYZP
 *  written by Matthew Shepherd
 */

#include "TLorentzVector.h"

#include "AMPTOOLS_MCGEN/ProductionMechanism.h"

class Kinematics;

class GammaPToNPartP {
  
public:
  
  GammaPToNPartP( float lowMass, float highMass, 
		  vector<double> &ChildMass,
		  ProductionMechanism::Type type,
		  float tcoef=4.0, float Ebeam=9.0/*GeV*/);
  
  Kinematics* generate();
  
  void addResonance( float mass, float width, float bf );
  
private:
  
  ProductionMechanism m_prodMech;
  
  TLorentzVector m_beam;
  TLorentzVector m_target;
  
  //double m_ChildMass[12];
  vector<double> m_ChildMass;
  unsigned int m_Npart;

};

#endif
