/**************************************************************************                                                             
* HallD software                                                          * 
* Copyright(C) 2020       GlueX and PrimEX-D Collaborations               * 
*                                                                         *                                                                
* Author: The GlueX and PrimEX-D Collaborations                           *                                                                
* Contributors: Igal Jaegle                                               *                                                               
*                                                                         *
*                                                                         *   
* This software is provided "as is" without any warranty.                 *
**************************************************************************/

#include <math.h>

#include "JEventProcessor_single_neutral_skim.h"
using namespace jana;

// Routine used to create our JEventProcessor
#include "JANA/JApplication.h"
#include <TLorentzVector.h>
#include "TMath.h"
#include "JANA/JApplication.h"
#include "DANA/DApplication.h"
#include "FCAL/DFCALShower.h"
#include "BCAL/DBCALShower.h"
#include "CCAL/DCCALShower.h"
#include "FCAL/DFCALCluster.h"
#include "FCAL/DFCALHit.h"
#include "START_COUNTER/DSCHit.h"
#include "ANALYSIS/DAnalysisUtilities.h"
#include "PID/DVertex.h"
#include "PID/DEventRFBunch.h"
#include "HDDM/DEventWriterHDDM.h"
#include <HDGEOMETRY/DGeometry.h>
#include <TOF/DTOFPoint.h>

#include "GlueX.h"
#include <vector>
#include <deque>
#include <string>
#include <iostream>
#include <algorithm>
#include <stdio.h>
#include <stdlib.h>

extern "C"{
  void InitPlugin(JApplication *app){
    InitJANAPlugin(app);
    app->AddProcessor(new JEventProcessor_single_neutral_skim());
  }
} // "C"


//------------------
// JEventProcessor_single_neutral_skim (Constructor)
//------------------
JEventProcessor_single_neutral_skim::JEventProcessor_single_neutral_skim()
{

  WRITE_EVIO = 1;
  WRITE_HDDM = 0;

  gPARMS->SetDefaultParameter( "SINGLE_NEUTRAL_SKIM:WRITE_EVIO", WRITE_EVIO );
  gPARMS->SetDefaultParameter( "SINGLE_NEUTRAL_SKIM:WRITE_HDDM", WRITE_HDDM );


  /*
  MIN_MASS   = 0.03; // GeV
  MAX_MASS   = 0.30; // GeV
  MIN_E      =  1.0; // GeV (photon energy cut)
  MIN_R      =   20; // cm  (cluster distance to beam line)
  MAX_DT     =   10; // ns  (cluster time diff. cut)
  MAX_ETOT   =   12; // GeV (max total FCAL energy)
  MIN_BLOCKS =    2; // minumum blocks per cluster

  WRITE_ROOT = 0;
  WRITE_EVIO = 1;

  gPARMS->SetDefaultParameter( "SINGLE_NEUTRAL_SKIM:MIN_MASS", MIN_MASS );
  gPARMS->SetDefaultParameter( "SINGLE_NEUTRAL_SKIM:MAX_MASS", MAX_MASS );
  gPARMS->SetDefaultParameter( "SINGLE_NEUTRAL_SKIM:MIN_E", MIN_E );                
  gPARMS->SetDefaultParameter( "SINGLE_NEUTRAL_SKIM:MIN_R", MIN_R );
  gPARMS->SetDefaultParameter( "SINGLE_NEUTRAL_SKIM:MAX_DT", MAX_DT );
  gPARMS->SetDefaultParameter( "SINGLE_NEUTRAL_SKIM:MAX_ETOT", MAX_ETOT );
  gPARMS->SetDefaultParameter( "SINGLE_NEUTRAL_SKIM:MIN_BLOCKS", MIN_BLOCKS );
  gPARMS->SetDefaultParameter( "SINGLE_NEUTRAL_SKIM:WRITE_ROOT", WRITE_ROOT );
  */
}

//------------------
// ~JEventProcessor_single_neutral_skim (Destructor)
//------------------
JEventProcessor_single_neutral_skim::~JEventProcessor_single_neutral_skim()
{

}

//------------------
// init
//------------------
jerror_t JEventProcessor_single_neutral_skim::init(void)
{
  num_epics_events = 0;
/*
  if( ! ( WRITE_ROOT || WRITE_EVIO ) ){

    cerr << "No output mechanism has been specified." << endl;
    return UNRECOVERABLE_ERROR;
  }

  if( WRITE_ROOT ){

    japp->RootWriteLock();

    m_tree = new TTree( "cluster", "Cluster Tree for Pi0 Calibration" );
    m_tree->Branch( "nClus", &m_nClus, "nClus/I" );
    m_tree->Branch( "hit0", m_hit0, "hit0[nClus]/I" );
    m_tree->Branch( "px", m_px, "px[nClus]/F" );
    m_tree->Branch( "py", m_py, "py[nClus]/F" );
    m_tree->Branch( "pz", m_pz, "pz[nClus]/F" );

    m_tree->Branch( "nHit", &m_nHit, "nHit/I" );
    m_tree->Branch( "chan", m_chan, "chan[nHit]/I" );
    m_tree->Branch( "e", m_e, "e[nHit]/F" );

    japp->RootUnLock();
  }
*/
  return NOERROR;
}

//------------------
// brun
//------------------
jerror_t JEventProcessor_single_neutral_skim::brun(JEventLoop *eventLoop, int32_t runnumber)
{
  DGeometry* dgeom = NULL;
  DApplication* dapp = dynamic_cast< DApplication* >(eventLoop->GetJApplication());
  if (dapp) dgeom = dapp->GetDGeometry(runnumber);
  if (dgeom) {
    dgeom->GetTargetZ(m_targetZ);
  } else {
    cerr << "No geometry accessbile to ccal_timing monitoring plugin." << endl;
    return RESOURCE_UNAVAILABLE;
  }	
  jana::JCalibration *jcalib = japp->GetJCalibration(runnumber);
  std::map<string, float> beam_spot;
  jcalib->Get("PHOTON_BEAM/beam_spot", beam_spot);
  m_beamSpotX = beam_spot.at("x");
  m_beamSpotY = beam_spot.at("y");

  return NOERROR;
}

//------------------
// evnt
//------------------
jerror_t JEventProcessor_single_neutral_skim::evnt(JEventLoop *loop, uint64_t eventnumber)
{
 
  vector< const DFCALShower* > locFCALShowers;
  vector< const DBCALShower* > locBCALShowers;
  vector< const DCCALShower* > locCCALShowers;
  vector< const DVertex* > kinfitVertex;
  vector<const DTOFPoint*> tof_points;
  vector< const DBeamPhoton* > locBeamPhotons;
  vector< const DSCHit* > locSCHit;

  loop->Get(locFCALShowers);
  loop->Get(locBCALShowers);
  loop->Get(locCCALShowers);
  loop->Get(kinfitVertex);
  loop->Get(tof_points);
  loop->Get(locBeamPhotons);
  loop->Get(locSCHit);
  
  vector<const DTrackCandidate*> locTrackCandidate;
  loop->Get(locTrackCandidate);

  vector< const DTrackTimeBased* > locTrackTimeBased;
  loop->Get(locTrackTimeBased);

  

  vector < const DFCALShower * > matchedShowers;
  
  const DEventWriterEVIO* locEventWriterEVIO = NULL;
  loop->GetSingle(locEventWriterEVIO);

  // always write out BOR events
  if(loop->GetJEvent().GetStatusBit(kSTATUS_BOR_EVENT)) {
    //jout << "Found BOR!" << endl;
    locEventWriterEVIO->Write_EVIOEvent( loop, "single_neutral_skim" );
    return NOERROR;
  }

  // write out the first few EPICS events to save run number & other meta info
  if(loop->GetJEvent().GetStatusBit(kSTATUS_EPICS_EVENT) && (num_epics_events<5)) {
      //jout << "Found EPICS!" << endl;
      locEventWriterEVIO->Write_EVIOEvent( loop, "single_neutral_skim" );
      num_epics_events++;
      return NOERROR;
  }

  
  vector< const JObject* > locObjectsToSave;  

  bool Candidate = false;
  
  DVector3 vertex;
  vertex.SetXYZ(m_beamSpotX, m_beamSpotY, m_targetZ);
  for (unsigned int i = 0 ; i < tof_points.size(); i++) {
    locObjectsToSave.push_back(static_cast<const JObject *>(tof_points[i]));
  }
  for (unsigned int i = 0 ; i < locBCALShowers.size(); i++) {
    locObjectsToSave.push_back(static_cast<const JObject *>(locBCALShowers[i]));
  }
  for (unsigned int i = 0 ; i < locFCALShowers.size(); i++) {
    locObjectsToSave.push_back(static_cast<const JObject *>(locFCALShowers[i]));
  }
  for (unsigned int i = 0 ; i < locCCALShowers.size(); i++) {
    locObjectsToSave.push_back(static_cast<const JObject *>(locCCALShowers[i]));
  }
  for (unsigned int i = 0 ; i < locTrackCandidate.size(); i++) {
    locObjectsToSave.push_back(static_cast<const JObject *>(locTrackCandidate[i]));
  }
  for (unsigned int i = 0 ; i < locTrackTimeBased.size(); i++) {
    locObjectsToSave.push_back(static_cast<const JObject *>(locTrackTimeBased[i]));
  }
  for (unsigned int i = 0 ; i < locBeamPhotons.size(); i++) {
    locObjectsToSave.push_back(static_cast<const JObject *>(locBeamPhotons[i]));
  }
  for (unsigned int i = 0 ; i < locSCHit.size(); i++) {
    locObjectsToSave.push_back(static_cast<const JObject *>(locSCHit[i]));
  }
  //Use kinfit when available
  double kinfitVertexX = m_beamSpotX;
  double kinfitVertexY = m_beamSpotY;
  double kinfitVertexZ = m_targetZ;
  for (unsigned int i = 0 ; i < kinfitVertex.size(); i++) {
    if(i==0)
      locObjectsToSave.push_back(static_cast<const JObject *>(kinfitVertex[0]));
  }
  TLorentzVector Target4Vec(0,0,0,me);
  vector<const DEventRFBunch*> locEventRFBunches;
  loop->Get(locEventRFBunches);
  if(locEventRFBunches.size() > 0) {
    locObjectsToSave.push_back(static_cast<const JObject *>(locEventRFBunches[0]));
  }
  int photon_nb = locFCALShowers.size() + locCCALShowers.size();
  if (photon_nb == 1 && locBCALShowers.size() == 0) {
    for (unsigned int i = 0; i < locFCALShowers.size(); i ++) {
      double e1 = locFCALShowers[i]->getEnergy();
      double x1 = locFCALShowers[i]->getPosition().X() - kinfitVertexX;
      double y1 = locFCALShowers[i]->getPosition().Y() - kinfitVertexY;
      double z1 = locFCALShowers[i]->getPosition().Z() - kinfitVertexZ;
      DVector3 vertex1(x1, y1, z1);
      double r1 = vertex1.Mag();
      double t1 = locFCALShowers[i]->getTime() - (r1 / TMath::C() * 1e6);
      
      double p1 = e1;
      double px1 = p1 * sin(vertex1.Theta()) * cos(vertex1.Phi());
      double py1 = p1 * sin(vertex1.Theta()) * sin(vertex1.Phi());
      double pz1 = p1 * cos(vertex1.Theta());
      TLorentzVector Photon1Vec(px1, py1, pz1, e1);
      double theta1 = Photon1Vec.Theta() * TMath::RadToDeg();
      for (unsigned int k = 0; k < locBeamPhotons.size(); k ++) {
	double eb = locBeamPhotons[k]->lorentzMomentum().E();
	double tb = locBeamPhotons[k]->time();
	TLorentzVector PhotonBeam4Vec(0,0,eb,eb);
	TLorentzVector Dark4Vec = Photon1Vec - (PhotonBeam4Vec + Target4Vec);
	double M2 = Dark4Vec.M2();
	Candidate |= ((theta1 < 5.0) && (-0.0005 < M2 && M2 < pow(0.2, 2)));
      }
    }
    for (unsigned int i = 0; i < locCCALShowers.size(); i ++) {
      double e1 = locCCALShowers[i]->E;
      double x1 = locCCALShowers[i]->x - kinfitVertexX;
      double y1 = locCCALShowers[i]->y - kinfitVertexY;
      double z1 = locCCALShowers[i]->z - kinfitVertexZ;
      DVector3 vertex1(x1, y1, z1);
      double r1 = vertex1.Mag();
      double t1 = locCCALShowers[i]->time - (r1 / TMath::C() * 1e6);
      
      double p1 = e1;
      double px1 = p1 * sin(vertex1.Theta()) * cos(vertex1.Phi());
      double py1 = p1 * sin(vertex1.Theta()) * sin(vertex1.Phi());
      double pz1 = p1 * cos(vertex1.Theta());
      TLorentzVector Photon1Vec(px1, py1, pz1, e1);
      double theta1 = Photon1Vec.Theta() * TMath::RadToDeg();
      for (unsigned int k = 0; k < locBeamPhotons.size(); k ++) {
	double eb = locBeamPhotons[k]->lorentzMomentum().E();
	double tb = locBeamPhotons[k]->time();
	TLorentzVector PhotonBeam4Vec(0,0,eb,eb);
	TLorentzVector Dark4Vec = Photon1Vec - (PhotonBeam4Vec + Target4Vec);
	double M2 = Dark4Vec.M2();
	Candidate |= ((theta1 < 5.0) && (-0.0005 < M2 && M2 < pow(0.2, 2)));
      }
    }
  }
  
  if( Candidate ){
    //cout <<"single_neutral_skim"<<endl;
    if( WRITE_EVIO ){
      //locEventWriterEVIO->Write_EVIOEvent( loop, "single_neutral_skim", locObjectsToSave );
      locEventWriterEVIO->Write_EVIOEvent( loop, "single_neutral_skim");
    }
    if( WRITE_HDDM ) {
      vector<const DEventWriterHDDM*> locEventWriterHDDMVector;
      loop->Get(locEventWriterHDDMVector);
      locEventWriterHDDMVector[0]->Write_HDDMEvent(loop, ""); 
    }
    
 }
 

  return NOERROR;
}

//------------------
// erun
//------------------
jerror_t JEventProcessor_single_neutral_skim::erun(void)
{
  // This is called whenever the run number changes, before it is
  // changed to give you a chance to clean up before processing
  // events from the next run number.
  return NOERROR;
}

//------------------
// Fin
//------------------
jerror_t JEventProcessor_single_neutral_skim::fini(void)
{
  // Called before program exit after event processing is finished.
  return NOERROR;
}
