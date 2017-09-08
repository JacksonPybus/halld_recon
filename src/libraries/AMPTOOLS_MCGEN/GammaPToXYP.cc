/*
 *  GammaPToXYP.cc
 *  GlueXTools
 *
 *  Created by Matthew Shepherd on 1/22/10.
 *  Copyright 2010 Home. All rights reserved.
 *
 */

#include "TLorentzVector.h"

#include "AMPTOOLS_MCGEN/GammaPToXYP.h"
#include "AMPTOOLS_MCGEN/TwoBodyDecayFactory.h"

#include "IUAmpTools/Kinematics.h"

#include <CobremsGeneration.hh>

GammaPToXYP::GammaPToXYP( float lowMassXY, float highMassXY, 
                          float massX, float massY, float beamMaxE, float beamPeakE, float beamLowE, float beamHighE,
                          ProductionMechanism::Type type, float slope ) : 
m_prodMech( ProductionMechanism::kProton, type, slope ), // last arg is t dependence
m_target( 0, 0, 0, 0.938272 ),
m_childMass( 0 ) {

  m_childMass.push_back( massX );
  m_childMass.push_back( massY );
  
  m_prodMech.setMassRange( lowMassXY, highMassXY );
 
  // Initialize coherent brem table
  float Emax =  beamMaxE;
  float Epeak = beamPeakE;
  float Elow = beamLowE;
  float Ehigh = beamHighE;
  
  int doPolFlux=0;  // want total flux (1 for polarized flux)
  float emitmr=10.e-9; // electron beam emittance
  float radt=20.e-6; // radiator thickness in m
  float collDiam=0.0034; // meters
  float Dist = 76.0; // meters
  CobremsGeneration cobrems(Emax, Epeak);
  cobrems.setBeamEmittance(emitmr);
  cobrems.setTargetThickness(radt);
  cobrems.setCollimatorDistance(Dist);
  cobrems.setCollimatorDiameter(collDiam);
  cobrems.setPolarizedFlag(doPolFlux);

  // Create histogram
  cobrem_vs_E = new TH1D("cobrem_vs_E", "Coherent Bremstrahlung vs. E_{#gamma}", 1000, Elow, Ehigh);
  
  // Fill histogram
  for(int i=1; i<=cobrem_vs_E->GetNbinsX(); i++){
	  double x = cobrem_vs_E->GetBinCenter(i)/Emax;
	  double y = 0;
	  if(Epeak<Elow) y = cobrems.Rate_dNidx(x);
	  else y = cobrems.Rate_dNtdx(x);
	  cobrem_vs_E->SetBinContent(i, y);
  }

}

Kinematics* 
GammaPToXYP::generate(){

  double beamE = cobrem_vs_E->GetRandom();
  m_beam.SetPxPyPzE(0,0,beamE,beamE);

  TLorentzVector resonance = m_prodMech.produceResonance( m_beam );
  double genWeight = m_prodMech.getLastGeneratedWeight();
  
  vector< TLorentzVector > allPart;
  allPart.push_back( m_beam );
  allPart.push_back( m_beam + m_target - resonance );
  
  TwoBodyDecayFactory decay( resonance.M(), m_childMass );
  
  vector<TLorentzVector> fsPart = decay.generateDecay();
  
  for( vector<TLorentzVector>::iterator aPart = fsPart.begin();
      aPart != fsPart.end(); ++aPart ){
    
    aPart->Boost( resonance.BoostVector() );
    allPart.push_back( *aPart );
  }
 
  return new Kinematics( allPart, genWeight );
}

void
GammaPToXYP::addResonance( float mass, float width, float bf ){
  
  m_prodMech.addResonance( mass, width, bf );
}

