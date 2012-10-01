/*
 *  DBCALPoint.cc
 *
 *  Created by Matthew Shepherd on 3/13/11.
 *
 */

#include <iostream>

using namespace std;

#include "BCAL/DBCALHit.h"
#include "BCAL/DBCALPoint.h"
#include "BCAL/DBCALGeometry.h"

#include "units.h"

DBCALPoint::DBCALPoint( const DBCALHit& hit1, const DBCALHit& hit2 )
{
  
  // this is a problem -- both hits are on the same end...
  // this constructor is not equipped for this!

  assert( hit1.end != hit2.end );

  // check to be sure both hits are in the same cell
  
  int cellId = DBCALGeometry::cellId( hit1.module, hit1.layer, hit1.sector );
  assert( cellId == 
             DBCALGeometry::cellId( hit2.module, hit2.layer, hit2.sector ) );

  // save typing
  
  float fibLen = DBCALGeometry::BCALFIBERLENGTH;
  float cEff = DBCALGeometry::C_EFFECTIVE;
  
  // figure out which hit is upstream and which is downstream
  // (downstream means farthest from the target)

  const DBCALHit& upHit = 
     ( hit1.end == DBCALGeometry::kUpstream ? hit1 : hit2 );
  const DBCALHit& downHit = 
     ( hit1.end == DBCALGeometry::kDownstream ? hit1 : hit2 );
  
  // in relality, here we would need to calibrate the time on a channel
  // by channel bases (calib. DB lookup)
  
  float tUp = upHit.t;
  float tDown = downHit.t;
  
  // get the position with respect to the center of the module -- positive
  // z in the downstream direction
  
  m_zLocal = 0.5 * cEff * ( tUp - tDown ); 

  // if it is outside the module then set it to the end of the module
  m_zLocal = ( m_zLocal > 0.5 * fibLen ? 0.5 * fibLen : m_zLocal );
  m_zLocal = ( m_zLocal < -0.5 * fibLen ? -0.5 * fibLen : m_zLocal );
  
  // set the z position relative to the center of the target -- this needs a database
  // lookup to get the target position (set for now at 65 cm)
  
  m_z = m_zLocal + DBCALGeometry::GLOBAL_CENTER - 65.0;
  
  // compute the arrival time of the energy at the cell
  m_t = 0.5 * ( tUp + tDown - fibLen / cEff );
  
  // now compute attentuation factors for each end based on distance
  // the light must travel
  
  float dUp = 0.5 * fibLen + m_zLocal;
  float dDown = 0.5 * fibLen - m_zLocal;

  float attUp = exp( -dUp / DBCALGeometry::ATTEN_LENGTH );
  float attDown = exp( -dDown / DBCALGeometry::ATTEN_LENGTH );
 
  // use these to correct the energy
  m_E =  ( upHit.E / attUp + downHit.E / attDown ) / 2;
  
  // The "E"s from the DBCALHit objects are in units of fADC counts.
  // a calibration constant needs to be applied here to get it
  // (at least approximately) into GeV. For now, apply an empirical
  // factor.
  float fADC_counts_per_GeV = 14000.0;
  m_E /= fADC_counts_per_GeV;
  
  m_r = DBCALGeometry::r( cellId );
  //for a uniform distribution of width a, the RMS is a/sqrt(12)
  m_sig_r = DBCALGeometry::rSize( cellId )/sqrt(12.0);
  
  m_phi = DBCALGeometry::phi( cellId );
  m_sig_phi = DBCALGeometry::phiSize( cellId )/sqrt(12.0);
  
  // this needs more careful examination.. for now assume that the error on the
  // 1/2 the time difference is 70 ps / sqrt( E ), as was reported in the BCAL NIM
  
  m_sig_z = cEff * 70 * k_psec / sqrt( m_E );

  m_module = hit1.module;
  m_layer = hit1.layer;
  m_sector = hit1.sector;
  
  // recast in terms of spherical coordinates
  convertCylindricalToSpherical();
}

DBCALPoint::DBCALPoint( const DBCALHit& hit, float zTarget )
{
  
  int cellId = DBCALGeometry::cellId( hit.module, hit.layer, hit.sector );
  
  // a boolean true if upstream hit false if downstream hit
  bool isUp = ( hit.end == DBCALGeometry::kUpstream );
  
  // save typing
  float fibLen = DBCALGeometry::BCALFIBERLENGTH;
  float cEff = DBCALGeometry::C_EFFECTIVE;
  
  // set the z position relative to the center of the target -- this needs a database
  // lookup to get the target position (set for now at 65 cm)
  m_z = zTarget;
  m_zLocal = m_z + 65 - DBCALGeometry::GLOBAL_CENTER;
  
  float d = ( isUp ? 0.5 * fibLen + m_zLocal : 0.5 * fibLen - m_zLocal );
  float att = exp( -d / DBCALGeometry::ATTEN_LENGTH );
  
  m_t = hit.t - d / cEff;
  m_E =  hit.E / att;
  
  m_r = DBCALGeometry::r( cellId );
  m_sig_r = DBCALGeometry::rSize( cellId );
  
  m_phi = DBCALGeometry::phi( cellId );
  m_sig_phi = DBCALGeometry::phiSize( cellId );
  
  // this needs more careful examination.. especially for single end
  // hits like this one
  
  m_sig_z = cEff * 400 * k_psec;

  m_module = hit.module;
  m_layer = hit.layer;
  m_sector = hit.sector;
  
  // recast in terms of spherical coordinates
  convertCylindricalToSpherical();
}


float
DBCALPoint::tInnerRadius() const {
 
  // the path length in the module
  
  float modulePath = m_rho - DBCALGeometry::BCALINNERRAD / sin( m_theta );
  
  // retard the time by that distance divided by the speed of light

  return m_t - modulePath / ( 30 * k_cm / k_nsec );
}

void
DBCALPoint::convertCylindricalToSpherical(){
  
  m_rho = sqrt( m_r * m_r + m_z * m_z );
  m_theta = fabs( atan2( m_r, m_z ) );
  
  float d_rho_d_r = m_r / sqrt( m_r * m_r + m_z * m_z );
  float d_rho_d_z = m_z / sqrt( m_r * m_r + m_z * m_z );
  
  m_sig_rho = sqrt( m_sig_r * m_sig_r * d_rho_d_r * d_rho_d_r +
                    m_sig_z * m_sig_z * d_rho_d_z * d_rho_d_z );

  float d_theta_d_r =  m_z / ( m_r * m_r + m_z * m_z );
  float d_theta_d_z = -m_r / ( m_r * m_r + m_z * m_z );
  
  m_sig_theta = sqrt( m_sig_r * m_sig_r * d_theta_d_r * d_theta_d_r +
                      m_sig_z * m_sig_z * d_theta_d_z * d_theta_d_z );
  
}


// Using const_cast is generally not a good idea, but we
// really want these to be const functions since, in practice,
// they don't change the location of the point.
// Clearly we can't make m_phi mutable -- that would be worse.

void
DBCALPoint::add2Pi() const {

  const_cast< DBCALPoint* >( this )->m_phi += 2*PI;
}

void
DBCALPoint::sub2Pi() const {
  
  const_cast< DBCALPoint* >( this )->m_phi -= 2*PI;
}
