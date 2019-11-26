// $Id$
//
//    File: DTOFHit_factory.cc
// Created: Wed Aug  7 09:30:17 EDT 2013
// Creator: davidl (on Darwin harriet.jlab.org 11.4.2 i386)
//


#include <iostream>
#include <iomanip>
#include <cmath>
#include <vector>
#include <limits>

#include <TMath.h>

using namespace std;

#include <TOF/DTOFDigiHit.h>
#include <TOF/DTOFTDCDigiHit.h>
#include "DTOFHit_factory.h"
#include <DAQ/Df250PulseIntegral.h>
#include <DAQ/Df250Config.h>
#include <DAQ/DCODAROCInfo.h>

using namespace jana;

static bool COSMIC_DATA = false;

int TOF_DEBUG = 0;

//------------------
// init
//------------------
jerror_t DTOFHit_factory::init(void)
{

  gPARMS->SetDefaultParameter("TOF:DEBUG_TOF_HITS", TOF_DEBUG,
			      "Generate DEBUG output");

  USE_NEWAMP_4WALKCORR = 1;
  gPARMS->SetDefaultParameter("TOF:USE_NEWAMP_4WALKCORR", USE_NEWAMP_4WALKCORR,
			      "Use Signal Amplitude for NEW walk correction with two fit functions");
  USE_AMP_4WALKCORR = 0;
  gPARMS->SetDefaultParameter("TOF:USE_AMP_4WALKCORR", USE_AMP_4WALKCORR,
			      "Use Signal Amplitude for walk correction rather than Integral");

  USE_NEW_4WALKCORR = 0;
  gPARMS->SetDefaultParameter("TOF:USE_NEW_4WALKCORR", USE_NEW_4WALKCORR,
			      "Use NEW walk correction function with 4 parameters");

  DELTA_T_ADC_TDC_MAX = 20.0; // ns
  //	DELTA_T_ADC_TDC_MAX = 30.0; // ns, value based on the studies from cosmic events
  gPARMS->SetDefaultParameter("TOF:DELTA_T_ADC_TDC_MAX", DELTA_T_ADC_TDC_MAX, 
			      "Maximum difference in ns between a (calibrated) fADC time and F1TDC time for them to be matched in a single hit");
  
  int analyze_cosmic_data = 0;
  gPARMS->SetDefaultParameter("TOF:COSMIC_DATA", analyze_cosmic_data,
			      "Special settings for analysing cosmic data");
  if(analyze_cosmic_data > 0)
    COSMIC_DATA = true;
  
  CHECK_FADC_ERRORS = true;
  gPARMS->SetDefaultParameter("TOF:CHECK_FADC_ERRORS", CHECK_FADC_ERRORS, "Set to 1 to reject hits with fADC250 errors, ser to 0 to keep these hits");
  
  
  /// Set basic conversion constants
  a_scale    = 0.2/5.2E5;
  t_scale    = 0.0625;   // 62.5 ps/count
  t_base     = 0.;       // ns
  t_base_tdc = 0.; // ns
  
  if(COSMIC_DATA)
    // Hardcoding of 110 taken from cosmics events
    tdc_adc_time_offset = 110.;
  else 
    tdc_adc_time_offset = 0.;
  
  // default values, will override from DTOFGeometry
  TOF_NUM_PLANES = 2;
  TOF_NUM_BARS = 44;
  TOF_MAX_CHANNELS = 176;
  
  return NOERROR;
}

//------------------
// brun
//------------------
jerror_t DTOFHit_factory::brun(jana::JEventLoop *eventLoop, int32_t runnumber)
{
    // Only print messages for one thread whenever run number change
    static pthread_mutex_t print_mutex = PTHREAD_MUTEX_INITIALIZER;
    static set<int> runs_announced;
    pthread_mutex_lock(&print_mutex);
    bool print_messages = false;
    if(runs_announced.find(runnumber) == runs_announced.end()){
        print_messages = true;
        runs_announced.insert(runnumber);
    }
    pthread_mutex_unlock(&print_mutex);
    
    // read in geometry information
    vector<const DTOFGeometry*> tofGeomVect;
    eventLoop->Get( tofGeomVect );
    if(tofGeomVect.size()<1)  return OBJECT_NOT_AVAILABLE;
    const DTOFGeometry& tofGeom = *(tofGeomVect[0]);
    
    TOF_NUM_PLANES = tofGeom.Get_NPlanes();
    TOF_NUM_BARS = tofGeom.Get_NBars();
    TOF_MAX_CHANNELS = TOF_NUM_PLANES*TOF_NUM_BARS*2;  // total number of bars * 2 ends
    
    /// Read in calibration constants
    vector<double> raw_adc_pedestals;
    vector<double> raw_adc_gains;
    vector<double> raw_adc_offsets;
    vector<double> raw_tdc_offsets;
    vector<double> raw_adc2E;
    
    if(print_messages) jout << "In DTOFHit_factory, loading constants..." << endl;
    
    // load timing cut values
    vector<double> time_cut_values;
	string locTOFHitTimeCutTable = tofGeom.Get_CCDB_DirectoryName() + "/HitTimeCut";
    if(eventLoop->GetCalib(locTOFHitTimeCutTable.c_str(), time_cut_values)){
      jout << "Error loading " << locTOFHitTimeCutTable << " SET DEFUALT to 0 and 100!" << endl;
      TimeCenterCut = 0.;
      TimeWidthCut = 100.;
    } else {
      double loli = time_cut_values[0];
      double hili = time_cut_values[1];
      TimeCenterCut = hili - (hili-loli)/2.;
      TimeWidthCut = (hili-loli)/2.;
      //jout<<"TOF Timing Cuts for PRUNING: "<<TimeCenterCut<<" +/- "<<TimeWidthCut<<endl;
    }

    // load scale factors
    map<string,double> scale_factors;
	string locTOFDigiScalesTable = tofGeom.Get_CCDB_DirectoryName() + "/digi_scales";
    if(eventLoop->GetCalib(locTOFDigiScalesTable.c_str(), scale_factors))
      jout << "Error loading " << locTOFDigiScalesTable << " !" << endl;
    if( scale_factors.find("TOF_ADC_ASCALE") != scale_factors.end() ) {
      ;	//a_scale = scale_factors["TOF_ADC_ASCALE"];
    } else {
      jerr << "Unable to get TOF_ADC_ASCALE from " << locTOFDigiScalesTable << " !" << endl;
    }
    if( scale_factors.find("TOF_ADC_TSCALE") != scale_factors.end() ) {
      ; //t_scale = scale_factors["TOF_ADC_TSCALE"];
    } else {
      jerr << "Unable to get TOF_ADC_TSCALE from " << locTOFDigiScalesTable << " !" << endl;
    }
    
    // load base time offset
    map<string,double> base_time_offset;
	string locTOFBaseTimeOffsetTable = tofGeom.Get_CCDB_DirectoryName() + "/base_time_offset";
    if (eventLoop->GetCalib(locTOFBaseTimeOffsetTable.c_str(),base_time_offset))
      jout << "Error loading " << locTOFBaseTimeOffsetTable << " !" << endl;
    if (base_time_offset.find("TOF_BASE_TIME_OFFSET") != base_time_offset.end())
      t_base = base_time_offset["TOF_BASE_TIME_OFFSET"];
    else
      jerr << "Unable to get TOF_BASE_TIME_OFFSET from "<<locTOFBaseTimeOffsetTable<<" !" << endl;	
    
    if (base_time_offset.find("TOF_TDC_BASE_TIME_OFFSET") != base_time_offset.end())
      t_base_tdc = base_time_offset["TOF_TDC_BASE_TIME_OFFSET"];
    else
      jerr << "Unable to get TOF_TDC_BASE_TIME_OFFSET from "<<locTOFBaseTimeOffsetTable<<" !" << endl;
    
    // load constant tables
	string locTOFPedestalsTable = tofGeom.Get_CCDB_DirectoryName() + "/pedestals";
    if(eventLoop->GetCalib(locTOFPedestalsTable.c_str(), raw_adc_pedestals))
      jout << "Error loading " << locTOFPedestalsTable << " !" << endl;
	string locTOFGainsTable = tofGeom.Get_CCDB_DirectoryName() + "/gains";
    if(eventLoop->GetCalib(locTOFGainsTable.c_str(), raw_adc_gains))
      jout << "Error loading " << locTOFGainsTable << " !" << endl;
	string locTOFADCTimeOffetsTable = tofGeom.Get_CCDB_DirectoryName() + "/adc_timing_offsets";
    if(eventLoop->GetCalib(locTOFADCTimeOffetsTable.c_str(), raw_adc_offsets))
      jout << "Error loading " << locTOFADCTimeOffetsTable << " !" << endl;

    if (USE_NEWAMP_4WALKCORR){

		string locTOFChanOffsetNEWAMPTable = tofGeom.Get_CCDB_DirectoryName() + "/timing_offsets_NEWAMP";
		if(eventLoop->GetCalib(locTOFChanOffsetNEWAMPTable.c_str(), raw_tdc_offsets)) {
			jout<<"\033[1;31m";  // red text";
			jout<< "Error loading "<<locTOFChanOffsetNEWAMPTable<<" !\033[0m" << endl;

			USE_NEWAMP_4WALKCORR = 0;
			jout << "\033[1;31m"; // red text again";
			string locTOFChanOffsetTable = tofGeom.Get_CCDB_DirectoryName() + "/timing_offsets";
			jout << "Try to resort back to old calibration table "<<locTOFChanOffsetTable<<" !\033[0m" << endl; 
			if(eventLoop->GetCalib(locTOFChanOffsetTable.c_str(), raw_tdc_offsets)) {
				jout << "Error loading "<<locTOFChanOffsetTable<<" !" << endl;
			} else {
				jout<<"OK Found!"<<endl;
			}
		
			string locTOFTimewalkTable = tofGeom.Get_CCDB_DirectoryName() + "/timewalk_parms";
			if(eventLoop->GetCalib(locTOFTimewalkTable.c_str(), timewalk_parameters))
				jout << "Error loading "<<locTOFTimewalkTable<<" !" << endl;
		} else { // timing offsets for NEWAMP exist, get also the walk parameters
			string locTOFTimewalkNEWAMPTable = tofGeom.Get_CCDB_DirectoryName() + "/timewalk_parms_NEWAMP";
			if(eventLoop->GetCalib(locTOFTimewalkNEWAMPTable.c_str(), timewalk_parameters_NEWAMP))
				jout << "Error loading "<<locTOFTimewalkNEWAMPTable<<" !" << endl;
		}

    } else if (USE_AMP_4WALKCORR){
		string locTOFTimewalkAMPTable = tofGeom.Get_CCDB_DirectoryName() + "/timewalk_parms_AMP";
		if(eventLoop->GetCalib(locTOFTimewalkAMPTable.c_str(), timewalk_parameters_AMP))
			jout << "Error loading "<<locTOFTimewalkAMPTable<<" !" << endl;
		string locTOFChanOffsetTable = tofGeom.Get_CCDB_DirectoryName() + "/timing_offsets";
		if(eventLoop->GetCalib(locTOFChanOffsetTable.c_str(), raw_tdc_offsets))
			jout << "Error loading "<<locTOFChanOffsetTable<<" !" << endl;
	} else if (USE_NEW_4WALKCORR){
		string locTOFTimewalkNEWTable = tofGeom.Get_CCDB_DirectoryName() + "/timewalk_parms_NEW";
		if(eventLoop->GetCalib(locTOFTimewalkNEWTable.c_str(), timewalk_parameters_NEW))
			jout << "Error loading "<<locTOFTimewalkNEWTable<<" !" << endl;
		string locTOFChanOffsetTable = tofGeom.Get_CCDB_DirectoryName() + "/timing_offsets";
		if(eventLoop->GetCalib(locTOFChanOffsetTable.c_str(), raw_tdc_offsets))
			jout << "Error loading "<<locTOFChanOffsetTable<<" !" << endl;
	} else {
		string locTOFTimewalkTable = tofGeom.Get_CCDB_DirectoryName() + "/timewalk_parms";
		if(eventLoop->GetCalib(locTOFTimewalkTable.c_str(), timewalk_parameters))
			jout << "Error loading "<<locTOFTimewalkTable<<" !" << endl;
		string locTOFChanOffsetTable = tofGeom.Get_CCDB_DirectoryName() + "/timing_offsets";
		if(eventLoop->GetCalib(locTOFChanOffsetTable.c_str(), raw_tdc_offsets))
			jout << "Error loading "<<locTOFChanOffsetTable<<" !" << endl;
    }

    
    FillCalibTable(adc_pedestals, raw_adc_pedestals, tofGeom);
    FillCalibTable(adc_gains, raw_adc_gains, tofGeom);
    FillCalibTable(adc_time_offsets, raw_adc_offsets, tofGeom);
    FillCalibTable(tdc_time_offsets, raw_tdc_offsets, tofGeom);

    
	string locTOFADC2ETable = tofGeom.Get_CCDB_DirectoryName() + "/adc2E";
    if(eventLoop->GetCalib(locTOFADC2ETable.c_str(), raw_adc2E))
      jout << "Error loading " << locTOFADC2ETable << " !" << endl;

    // make sure we have one entry per channel
	adc2E.resize(TOF_NUM_PLANES*TOF_NUM_BARS*2);
    for (unsigned int n=0; n<raw_adc2E.size(); n++){
      adc2E[n] = raw_adc2E[n];
    }

    /*
      CheckCalibTable(adc_pedestals,"/TOF/pedestals");
      CheckCalibTable(adc_gains,"/TOF/gains");
      CheckCalibTable(adc_time_offsets,"/TOF/adc_timing_offsets");
      CheckCalibTable(tdc_time_offsets,"/TOF/timing_offsets");
    */

    return NOERROR;
}

//------------------
// evnt
//------------------
jerror_t DTOFHit_factory::evnt(JEventLoop *loop, uint64_t eventnumber)
{
  /// Generate DTOFHit object for each DTOFDigiHit object.
  /// This is where the first set of calibration constants
  /// is applied to convert from digitzed units into natural
  /// units.
  ///
  /// Note that this code does NOT get called for simulated
  /// data in HDDM format. The HDDM event source will copy
  /// the precalibrated values directly into the _data vector.
  
  const DTTabUtilities* locTTabUtilities = NULL;
  loop->GetSingle(locTTabUtilities);
  
  // First, make hits out of all fADC250 hits
  vector<const DTOFDigiHit*> digihits;
  loop->Get(digihits);
  for(unsigned int i=0; i<digihits.size(); i++){
    const DTOFDigiHit *digihit = digihits[i];
    
    // Error checking for pre-Fall 2016 firmware
    if(digihit->datasource == 1) {
      // There is a slight difference between Mode 7 and 8 data
      // The following condition signals an error state in the flash algorithm
      // Do not make hits out of these
      const Df250PulsePedestal* PPobj = NULL;
      digihit->GetSingle(PPobj);
      if (PPobj != NULL) {
	if (PPobj->pedestal == 0 || PPobj->pulse_peak == 0) continue;
      } else 
	continue;
      
      //if (digihit->pulse_time == 0) continue; // Should already be caught
    }
    
    if(CHECK_FADC_ERRORS && !locTTabUtilities->CheckFADC250_NoErrors(digihit->QF)){ 

      if (TOF_DEBUG){
	vector <const Df250PulseData *> pulses;
	digihit->Get(pulses);
	const Df250PulseData *p = pulses[0];
	
	cout<<"1: "<<eventnumber<<" P/B/E  "<<digihit->plane<<"/"<<digihit->bar<<"/"<<digihit->end
	    <<" :::>  I/Ped/P/T   "<<digihit->pulse_integral<<"/"<<digihit->pedestal<<"/"<<digihit->pulse_peak<<"/"<<digihit->pulse_time
	    <<" QF: 0x"<<hex<<digihit->QF<<dec
	    <<"       roc/slot/chan "<<p->rocid<<"/"<<p->slot<<"/"<<p->channel
	    << endl;
      }

      //continue;

    }
    // Initialize pedestal to one found in CCDB, but override it
    // with one found in event if is available (?)
    // For now, only keep events with a correct pedestal
    double pedestal = GetConstant(adc_pedestals, digihit); // get mean pedestal from DB in case we need it
    double nsamples_integral = digihit->nsamples_integral;
    double nsamples_pedestal = digihit->nsamples_pedestal;
    
    // nsamples_pedestal should always be positive for valid data - err on the side of caution for now
    if(nsamples_pedestal == 0) {
      jerr << "DTOFDigiHit with nsamples_pedestal == 0 !   Event = " << eventnumber << endl;
      continue;
    }
    
    double pedestal4Amp = pedestal;
    int AlreadyDone = 0;
    if( (digihit->pedestal>0) && locTTabUtilities->CheckFADC250_PedestalOK(digihit->QF) ) {
      pedestal = digihit->pedestal * (double)(nsamples_integral)/(double)(nsamples_pedestal); // overwrite pedestal
    } else {

      if (TOF_DEBUG){
	vector <const Df250PulseData *> pulses;
	digihit->Get(pulses);
	const Df250PulseData *p = pulses[0];
	
	cout<<"2: "<<eventnumber<<" P/B/E  "<<digihit->plane<<"/"<<digihit->bar<<"/"<<digihit->end
	    <<" :::>   I/Ped/P/T    "<<digihit->pulse_integral<<"/"<<digihit->pedestal<<"/"<<digihit->pulse_peak<<"/"<<digihit->pulse_time
	    <<" QF: 0x"<<hex<<digihit->QF<<dec
	    <<"       roc/slot/chan  "<<p->rocid<<"/"<<p->slot<<"/"<<p->channel
 	    << endl;
	
      }
      
      pedestal *= (double)(nsamples_integral); 
      pedestal4Amp *= (double)nsamples_pedestal;
      AlreadyDone = 1;
      //continue;
    }

    if ((digihit->pulse_peak == 0) && (!AlreadyDone)){
      pedestal = pedestal4Amp * (double)(nsamples_integral);
      pedestal4Amp *=  (double)nsamples_pedestal;
    }
    
    // Apply calibration constants here
    double A = (double)digihit->pulse_integral;
    double T = (double)digihit->pulse_time;
    T =  t_scale * T - GetConstant(adc_time_offsets, digihit) + t_base;
    double dA = A - pedestal;
    
    if (dA<0) {
      
      if (TOF_DEBUG){
	
	vector <const Df250PulseData *> pulses;
	digihit->Get(pulses);
	const Df250PulseData *p = pulses[0];
	
	cout<<"3: "<<eventnumber<<"  "<<dA<<"   "<<digihit->plane<<"   "<<digihit->bar<<"   "<<digihit->end
	    <<" :::>  "<<digihit->pulse_integral<<"  "<<digihit->pedestal<<"  "<<digihit->pulse_peak<<"   "<<digihit->pulse_time
	    <<"       roc/slot/chan "<<p->rocid<<"/"<<p->slot<<"/"<<p->channel
	    << endl;

      }
      // ok if Integral is below zero this is a good hint that we can not use this hit!
      continue; 
    }
    // apply Time cut to prune out of time hits
    if (TMath::Abs(T-TimeCenterCut)> TimeWidthCut ) continue;
    
    DTOFHit *hit = new DTOFHit;
    hit->plane = digihit->plane;
    hit->bar   = digihit->bar;
    hit->end   = digihit->end;
    hit->dE=dA;  // this will be scaled to energy units later
    hit->Amp = (float)digihit->pulse_peak - pedestal4Amp/(float)nsamples_pedestal;

    if (hit->Amp<1){ // this happens if pulse_peak is reported as zero, resort to use scaled Integral value
      hit->Amp = dA*0.163;
    }
    
    if(COSMIC_DATA)
      hit->dE = (A - 55*pedestal); // value of 55 is taken from (NSB,NSA)=(10,45) in the confg file
    
    hit->t_TDC=numeric_limits<double>::quiet_NaN();
    hit->t_fADC=T;
    hit->t = hit->t_fADC;  // set initial time to the ADC time, in case there's no matching TDC hit
    
    hit->has_fADC=true;
    hit->has_TDC=false;
    
    /*
      cout << "TOF ADC hit =  (" << hit->plane << "," << hit->bar << "," << hit->end << ")  " 
      << t_scale << " " << T << "  "
      << GetConstant(adc_time_offsets, digihit) << " " 
      << t_scale*GetConstant(adc_time_offsets, digihit) << " " << hit->t << endl;
    */
    
    hit->AddAssociatedObject(digihit);
    
    _data.push_back(hit);
  }
  
  //Get the TDC hits
  vector<const DTOFTDCDigiHit*> tdcdigihits;
  loop->Get(tdcdigihits);
  
  // Next, loop over TDC hits, matching them to the
  // existing fADC hits where possible and updating
  // their time information. If no match is found, then
  // create a new hit with just the TDC info.
  for(unsigned int i=0; i<tdcdigihits.size(); i++)
    {
      const DTOFTDCDigiHit *digihit = tdcdigihits[i];
      
      // Apply calibration constants here
      double T = locTTabUtilities->Convert_DigiTimeToNs_CAEN1290TDC(digihit);
      T += t_base_tdc - GetConstant(tdc_time_offsets, digihit) + tdc_adc_time_offset;
      
      // do not consider Time hits away from coincidence peak Note: This cut should be wide for uncalibrated data!!!!!
      if (TMath::Abs(T-TimeCenterCut)> TimeWidthCut ) continue;

      /*
	cout << "TOF TDC hit =  (" << digihit->plane << "," << digihit->bar << "," << digihit->end << ")  " 
	<< T << "  " << GetConstant(tdc_time_offsets, digihit) << endl;
      */
      
      // Look for existing hits to see if there is a match
      // or create new one if there is no match
      DTOFHit *hit = FindMatch(digihit->plane, digihit->bar, digihit->end, T);
      //DTOFHit *hit = FindMatch(digihit->plane, hit->bar, hit->end, T);
      if(!hit){
	continue; // Do not use unmatched TDC hits
	/*
	hit = new DTOFHit;
	hit->plane = digihit->plane;
	hit->bar   = digihit->bar;
	hit->end   = digihit->end;
	hit->dE = 0.0;
	hit->Amp = 0.0;
	hit->t_fADC=numeric_limits<double>::quiet_NaN();
	hit->has_fADC=false;
	
	_data.push_back(hit);
	*/
      } else if (hit->has_TDC) { // this tof ADC hit has already a matching TDC, make new tof ADC hit
	DTOFHit *newhit = new DTOFHit;
	newhit->plane = hit->plane;
	newhit->bar = hit->bar;
	newhit->end = hit->end;
	newhit->dE = hit->dE;
	newhit->Amp = hit->Amp;
	newhit->t_fADC = hit->t_fADC;
	newhit->has_fADC = hit->has_fADC;
	newhit->t_TDC=numeric_limits<double>::quiet_NaN();
	newhit->t = hit->t_fADC;  // set initial time to the ADC time, in case there's no matching TDC hit	
	newhit->has_TDC=false;
	newhit->AddAssociatedObject(digihit);
	_data.push_back(newhit);
	hit = newhit;
      }
      hit->has_TDC=true;
      hit->t_TDC=T;
      
      if (hit->dE>0.){

	// time walk correction
	// Note at this point the dE value is still in ADC units
	double tcorr = 0.;
	if (USE_AMP_4WALKCORR) {
	  // use amplitude instead of integral
	  tcorr = CalcWalkCorrAmplitude(hit);

	} else if (USE_NEW_4WALKCORR) {
	  // new functional form with 4 parameter but still using integral
	  tcorr = CalcWalkCorrNEW(hit);

	} else if (USE_NEWAMP_4WALKCORR) {
	  // new functional form with 2 functions and 4 parameter using amplitude
 	  tcorr = CalcWalkCorrNEWAMP(hit);

	} else {
	  // use integral
	  tcorr = CalcWalkCorrIntegral(hit);

	}
	
	T -= tcorr;
      }
      hit->t=T;
      
      hit->AddAssociatedObject(digihit);
    }
  
  // Apply calibration constants to convert pulse integrals to energy units
  for (unsigned int i=0;i<_data.size();i++){
    int id=2*TOF_NUM_BARS*_data[i]->plane + TOF_NUM_BARS*_data[i]->end + _data[i]->bar-1;
    _data[i]->dE *= adc2E[id];
    //cout<<id<<"   "<< adc2E[id]<<"      "<<_data[i]->dE<<endl;
  }
  
  return NOERROR;
}

//------------------
// FindMatch
//------------------
DTOFHit* DTOFHit_factory::FindMatch(int plane, int bar, int end, double T)
{
    DTOFHit* best_match = NULL;

    // Loop over existing hits (from fADC) and look for a match
    // in both the sector and the time.
    for(unsigned int i=0; i<_data.size(); i++){
        DTOFHit *hit = _data[i];

        if(!isfinite(hit->t_fADC)) continue; // only match to fADC hits, not bachelor TDC hits
        if(hit->plane != plane) continue;
        if(hit->bar != bar) continue;
        if(hit->end != end) continue;

        //double delta_T = fabs(hit->t - T);
        double delta_T = fabs(T - hit->t);
        if(delta_T > DELTA_T_ADC_TDC_MAX) continue;

        // if there are multiple hits, pick the one that is closest in time
        if(best_match != NULL) {
            if(delta_T < fabs(best_match->t - T))
                best_match = hit;
        } else {
            best_match = hit;
        }

    }

    return best_match;
}

//------------------
// erun
//------------------
jerror_t DTOFHit_factory::erun(void)
{
    return NOERROR;
}

//------------------
// fini
//------------------
jerror_t DTOFHit_factory::fini(void)
{
    return NOERROR;
}


//------------------
// FillCalibTable
//------------------
void DTOFHit_factory::FillCalibTable(tof_digi_constants_t &table, vector<double> &raw_table, 
        const DTOFGeometry &tofGeom)
{
    char str[256];
    int channel = 0;

    // reset the table before filling it
    table.clear();

    for(int plane=0; plane<tofGeom.Get_NPlanes(); plane++) {
        int plane_index=2*tofGeom.Get_NBars()*plane;
        table.push_back( vector< pair<double,double> >(tofGeom.Get_NBars()) );
        for(int bar=0; bar<tofGeom.Get_NBars(); bar++) {
            table[plane][bar] 
                = pair<double,double>(raw_table[plane_index+bar],
                        raw_table[plane_index+tofGeom.Get_NBars()+bar]);
            channel+=2;	      
        }
    }

    // check to make sure that we loaded enough channels
    if(channel != TOF_MAX_CHANNELS) { 
        sprintf(str, "Wrong number of channels for TOF table! channel=%d (should be %d)", 
                channel, TOF_MAX_CHANNELS);
        cerr << str << endl;
        throw JException(str);
    }
}


//------------------------------------
// GetConstant
//   Allow a few different interfaces
//   NOTE: LoadGeometry() must be called before calling these functions
//
//   TOF Geometry as defined in the Translation Table:
//     plane = 0-1
//     bar   = 1-44
//     end   = 0-1
//   Note the different counting schemes used
//------------------------------------
const double DTOFHit_factory::GetConstant( const tof_digi_constants_t &the_table, 
        const int in_plane, const int in_bar, const int in_end) const
{
    char str[256];

    if( (in_plane < 0) || (in_plane > TOF_NUM_PLANES)) {
        sprintf(str, "Bad module # requested in DTOFHit_factory::GetConstant()! requested=%d , should be 1-%d", in_plane, TOF_NUM_PLANES);
        cerr << str << endl;
        throw JException(str);
    }
    if( (in_bar <= 0) || (in_bar > TOF_NUM_BARS)) {
        sprintf(str, "Bad layer # requested in DTOFHit_factory::GetConstant()! requested=%d , should be 1-%d", in_bar, TOF_NUM_BARS);
        cerr << str << endl;
        throw JException(str);
    }
    if( (in_end != 0) && (in_end != 1) ) {
        sprintf(str, "Bad end # requested in DTOFHit_factory::GetConstant()! requested=%d , should be 0-1", in_end);
        cerr << str << endl;
        throw JException(str);
    }

    // we have two ends, indexed as 0/1 
    // could be north/south or up/down depending on the bar orientation
    if(in_end == 0) {
        return the_table[in_plane][in_bar].first;
    } else {
        return the_table[in_plane][in_bar].second;
    }
}

const double DTOFHit_factory::GetConstant( const tof_digi_constants_t &the_table, 
        const DTOFHit *in_hit) const
{
    char str[256];

    if( (in_hit->plane < 0) || (in_hit->plane > TOF_NUM_PLANES)) {
        sprintf(str, "Bad module # requested in DTOFHit_factory::GetConstant()! requested=%d , should be 1-%d", in_hit->plane, TOF_NUM_PLANES);
        cerr << str << endl;
        throw JException(str);
    }
    if( (in_hit->bar <= 0) || (in_hit->bar > TOF_NUM_BARS)) {
        sprintf(str, "Bad layer # requested in DTOFHit_factory::GetConstant()! requested=%d , should be 1-%d", in_hit->bar, TOF_NUM_BARS);
        cerr << str << endl;
        throw JException(str);
    }
    if( (in_hit->end != 0) && (in_hit->end != 1) ) {
        sprintf(str, "Bad end # requested in DTOFHit_factory::GetConstant()! requested=%d , should be 0-1", in_hit->end);
        cerr << str << endl;
        throw JException(str);
    }

    // we have two ends, indexed as 0/1 
    // could be north/south or up/down depending on the bar orientation
    if(in_hit->end == 0) {
        return the_table[in_hit->plane][in_hit->bar-1].first;
    } else {
        return the_table[in_hit->plane][in_hit->bar-1].second;
    }
}

const double DTOFHit_factory::GetConstant( const tof_digi_constants_t &the_table, 
        const DTOFDigiHit *in_digihit) const
{
    char str[256];

    if( (in_digihit->plane < 0) || (in_digihit->plane > TOF_NUM_PLANES)) {
        sprintf(str, "Bad module # requested in DTOFHit_factory::GetConstant()! requested=%d , should be 1-%d", in_digihit->plane, TOF_NUM_PLANES);
        cerr << str << endl;
        throw JException(str);
    }
    if( (in_digihit->bar <= 0) || (in_digihit->bar > TOF_NUM_BARS)) {
        sprintf(str, "Bad layer # requested in DTOFHit_factory::GetConstant()! requested=%d , should be 1-%d", in_digihit->bar, TOF_NUM_BARS);
        cerr << str << endl;
        throw JException(str);
    }
    if( (in_digihit->end != 0) && (in_digihit->end != 1) ) {
        sprintf(str, "Bad end # requested in DTOFHit_factory::GetConstant()! requested=%d , should be 0-1", in_digihit->end);
        cerr << str << endl;
        throw JException(str);
    }

    // we have two ends, indexed as 0/1 
    // could be north/south or up/down depending on the bar orientation
    if(in_digihit->end == 0) {
        return the_table[in_digihit->plane][in_digihit->bar-1].first;
    } else {
        return the_table[in_digihit->plane][in_digihit->bar-1].second;
    }
}

const double DTOFHit_factory::GetConstant( const tof_digi_constants_t &the_table, 
        const DTOFTDCDigiHit *in_digihit) const
{
    char str[256];

    if( (in_digihit->plane < 0) || (in_digihit->plane > TOF_NUM_PLANES)) {
        sprintf(str, "Bad module # requested in DTOFHit_factory::GetConstant()! requested=%d , should be 1-%d", in_digihit->plane, TOF_NUM_PLANES);
        cerr << str << endl;
        throw JException(str);
    }
    if( (in_digihit->bar <= 0) || (in_digihit->bar > TOF_NUM_BARS)) {
        sprintf(str, "Bad layer # requested in DTOFHit_factory::GetConstant()! requested=%d , should be 1-%d", in_digihit->bar, TOF_NUM_BARS);
        cerr << str << endl;
        throw JException(str);
    }
    if( (in_digihit->end != 0) && (in_digihit->end != 1) ) {
        sprintf(str, "Bad end # requested in DTOFHit_factory::GetConstant()! requested=%d , should be 0-1", in_digihit->end);
        cerr << str << endl;
        throw JException(str);
    }

    // we have two ends, indexed as 0/1 
    // could be north/south or up/down depending on the bar orientation
    if(in_digihit->end == 0) {
        return the_table[in_digihit->plane][in_digihit->bar-1].first;
    } else {
        return the_table[in_digihit->plane][in_digihit->bar-1].second;
    }
}

/*
   const double DTOFHit_factory::GetConstant( const tof_digi_constants_t &the_table,
   const DTranslationTable *ttab,
   const int in_rocid, const int in_slot, const int in_channel) const {

   char str[256];

   DTranslationTable::csc_t daq_index = { in_rocid, in_slot, in_channel };
   DTranslationTable::DChannelInfo channel_info = ttab->GetDetectorIndex(daq_index);

   if( (channel_info.tof.plane <= 0) 
   || (channel_info.tof.plane > static_cast<unsigned int>(TOF_NUM_PLANES))) {
   sprintf(str, "Bad plane # requested in DTOFHit_factory::GetConstant()! requested=%d , should be 1-%d", channel_info.tof.plane, TOF_NUM_PLANES);
   cerr << str << endl;
   throw JException(str);
   }
   if( (channel_info.tof.bar <= 0) 
   || (channel_info.tof.bar > static_cast<unsigned int>(TOF_NUM_BARS))) {
   sprintf(str, "Bad bar # requested in DTOFHit_factory::GetConstant()! requested=%d , should be 1-%d", channel_info.tof.bar, TOF_NUM_BARS);
   cerr << str << endl;
   throw JException(str);
   }
   if( (channel_info.tof.end != 0) && (channel_info.tof.end != 1) ) {
   sprintf(str, "Bad end # requested in DTOFHit_factory::GetConstant()! requested=%d , should be 0-1", channel_info.tof.end);
   cerr << str << endl;
   throw JException(str);
   }

   int the_cell = DTOFGeometry::cellId(channel_info.tof.module,
   channel_info.tof.layer,
   channel_info.tof.sector);

   if(channel_info.tof.end == DTOFGeometry::kUpstream) {
// handle the upstream end
return the_table.at(the_cell).first;
} else {
// handle the downstream end
return the_table.at(the_cell).second;
}
}
*/
double DTOFHit_factory::CalcWalkCorrIntegral(DTOFHit* hit){
  int id=2*TOF_NUM_BARS*hit->plane+TOF_NUM_BARS*hit->end+hit->bar-1;
  double A=hit->dE;
  double C0=timewalk_parameters[id][1];
  double C1=timewalk_parameters[id][1];
  double C2=timewalk_parameters[id][2];
  double A0=timewalk_parameters[id][3];

  double a1 = C0 + C1*pow(A,C2);
  double a2 = C0 + C1*pow(A0,C2);

  float corr = a1 - a2;

  //cout<<id<<"   "<<A<<"    "<<a1<<"   "<<a2<<"    "<<corr<<endl;

  return corr;


}


double DTOFHit_factory::CalcWalkCorrAmplitude(DTOFHit* hit){

  int id=2*TOF_NUM_BARS*hit->plane+TOF_NUM_BARS*hit->end+hit->bar-1;
  double A  = hit->Amp;
  double C0 = timewalk_parameters_AMP[id][0];
  double C1 = timewalk_parameters_AMP[id][1];
  double C2 = timewalk_parameters_AMP[id][2];
  double C3 = timewalk_parameters_AMP[id][3];

  double hookx = timewalk_parameters_AMP[id][4]; 
  double refx = timewalk_parameters_AMP[id][5];
  double val_at_ref = C0 + C1*pow(refx,C2); 
  double val_at_hook = C0 + C1*pow(hookx,C2); 
  double slope = (val_at_hook - C3)/hookx;
  if (refx>hookx){
    val_at_ref  = slope * refx + C3; 
  }
  double val_at_A = C0 + C1*pow(A,C2);
  if (A>hookx){
    val_at_A = slope * A + C3; 
  }

  float corr = val_at_A - val_at_ref;

  //cout<<id<<"   "<<val_at_A<<"   "<<val_at_ref<<"    "<<corr<<endl;

  return corr;

}


double DTOFHit_factory::CalcWalkCorrNEW(DTOFHit* hit){
 
  int id=2*TOF_NUM_BARS*hit->plane+TOF_NUM_BARS*hit->end+hit->bar-1;
  double ADC=hit->dE;

  if (ADC<1.){
    return 0;
  }
  
  double A = timewalk_parameters_NEW[id][0];
  double B = timewalk_parameters_NEW[id][1];
  double C = timewalk_parameters_NEW[id][2];
  double D = timewalk_parameters_NEW[id][3];
  double ADCREF = timewalk_parameters_NEW[id][4];

  if (ADC>20000.){
    ADC = 20000.;
  }
  double a1 = A + B*pow(ADC,-0.5) + C*pow(ADC,-0.33) + D*pow(ADC,-0.2);
  double a2 = A + B*pow(ADCREF,-0.5) + C*pow(ADCREF,-0.33) + D*pow(ADCREF,-0.2);


  float corr = a1 - a2;

  //cout<<id<<"   "<<a1<<"   "<<a2<<"    "<<corr<<endl;

  return corr;

}

double DTOFHit_factory::CalcWalkCorrNEWAMP(DTOFHit* hit){
 
  int id=2*TOF_NUM_BARS*hit->plane+TOF_NUM_BARS*hit->end+hit->bar-1;
  double ADC=hit->Amp;
  if (ADC<50.){
    return 0;
  }
  double loc = timewalk_parameters_NEWAMP[id][8];
  int offset = 0;
  if (ADC>loc){
    offset = 4;
  }
  double A = timewalk_parameters_NEWAMP[id][0+offset];
  double B = timewalk_parameters_NEWAMP[id][1+offset];
  double C = timewalk_parameters_NEWAMP[id][2+offset];
  double D = timewalk_parameters_NEWAMP[id][3+offset];

  double ADCREF = timewalk_parameters_NEWAMP[id][9];
  double A2 = timewalk_parameters_NEWAMP[id][4];
  double B2 = timewalk_parameters_NEWAMP[id][5];
  double C2 = timewalk_parameters_NEWAMP[id][6];
  double D2 = timewalk_parameters_NEWAMP[id][7];

  double a1 = A + B*pow(ADC,-0.5) + C*pow(ADC,-0.33) + D*pow(ADC,-0.2);
  double a2 = A2 + B2*pow(ADCREF,-0.5) + C2*pow(ADCREF,-0.33) + D2*pow(ADCREF,-0.2);

  if (ADC>4095){
    a1 += 0.6; // overflow hits are off by about 0.6ns to the regular curve.
  }

  float corr = a1 - a2;

  //cout<<id<<"     "<<ADC<<"      "<<a1<<"   "<<a2<<"    "<<corr<<endl;
  
  return corr;

}

