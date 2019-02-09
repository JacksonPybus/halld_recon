// $Id$
//
//    File: JEventProcessor_DIRC_online.cc


#include <stdint.h>
#include <vector>

#include "JEventProcessor_DIRC_online.h"
#include <JANA/JApplication.h>

using namespace std;
using namespace jana;

#include "TTAB/DTTabUtilities.h"
#include <DAQ/DDIRCTDCHit.h>
#include <DIRC/DDIRCTDCDigiHit.h>
#include <DIRC/DDIRCPmtHit.h>
#include <DAQ/Df250PulseData.h>
#include <TRIGGER/DL1Trigger.h>

#include <TDirectory.h>
#include <TH1.h>
#include <TH2.h>
#include <TProfile.h>

const int Nboxes = 2;
const int Nchannels = 108*64;
const int Npixelrows = 48;
const int Npixelcolumns = 144;

const int NmultBins = 50;
const int Nmult = 500;
const int NmultDigi = 1000;

// Hit pointers
static TH1I *dirc_num_events;
static TH1I *hLEDRefTime;
static TH1I *hLEDRefIntegral;
static TH1I *hRefTime;
static TH1I *hHit_NHits[2];
static TH1I *hHit_Box[2];
static TH2I *hHit_NHitsVsBox[2];
static TH1I *hHit_TimeOverThreshold[Nboxes][2];
static TH2I *hHit_TimeOverThresholdVsChannel[Nboxes][2];
static TH1I *hHit_tdcTime[Nboxes][2];
static TH2I *hHit_tdcTimeVsChannel[Nboxes][2];
static TH2I *hHit_pixelOccupancy[Nboxes][2];

// DigiHit pointers
static TH1I *hDigiHit_NtdcHits[2];
static TH1I *hDigiHit_Box[2];
static TH2I *hDigiHit_NtdcHitsVsBox[2];
static TH1I *hDigiHit_tdcTime[Nboxes][2];
static TH2I *hDigiHit_tdcTimeVsChannel[Nboxes][2];
static TH2I *hDigiHit_pixelOccupancy[Nboxes][2];

// LED specific histograms
static TH2I *hHit_tdcTimeDiffVsChannel[Nboxes];
static TH1I *hHit_tdcTimeDiffEvent[Nboxes];
static TH2I *hHit_Timewalk[Nboxes][Nchannels];

//----------------------------------------------------------------------------------

// Routine used to create our JEventProcessor
extern "C"{
    void InitPlugin(JApplication *app){
        InitJANAPlugin(app);
        app->AddProcessor(new JEventProcessor_DIRC_online());
    }
}


//----------------------------------------------------------------------------------


JEventProcessor_DIRC_online::JEventProcessor_DIRC_online() {
}


//----------------------------------------------------------------------------------


JEventProcessor_DIRC_online::~JEventProcessor_DIRC_online() {
}


//----------------------------------------------------------------------------------

jerror_t JEventProcessor_DIRC_online::init(void) {

    // create root folder for psc and cd to it, store main dir
    TDirectory *mainDir = gDirectory;
    TDirectory *dircDir = gDirectory->mkdir("DIRC_online");
    dircDir->cd();

    hLEDRefTime = new TH1I("LEDRefTime", "LED reference SiPM time; time (ns)", 500, 0, 500);
    hLEDRefIntegral = new TH1I("LEDRefIntegral", "LED reference SiPM integral; integral (ADC)", 500, 0, 2000);
    hRefTime = new TH1I("RefTime", "Reference time from mean hit time; time (ns)", 500, 0, 100);

    // book hists
    dirc_num_events = new TH1I("dirc_num_events","DIRC Number of events",1,0.5,1.5);
    TDirectory *hitDir = gDirectory->mkdir("Hit"); hitDir->cd();
  
    TString trig_str[] = {"LED","NonLED"}; 
    for (int j = 0; j < 2; j++) {
	hHit_NHits[j] = new TH1I("Hit_NHits_"+trig_str[j],"DIRCPmtHit multiplicity " + trig_str[j] + "; hits; events",NmultBins,0.5,0.5+Nmult);
	hHit_Box[j] = new TH1I("Hit_Box_"+trig_str[j],"DIRCPmtHit box" + trig_str[j] + "; box; hits",2,-0.5,-0.5+2);
    	hHit_NHitsVsBox[j] = new TH2I("Hit_NHitsVsBox_"+trig_str[j],"DIRCPmtHit multiplicity vs. box" + trig_str[j] + "; box; hits",2,-0.5,-0.5+2,NmultBins,0.5,0.5+Nmult);
    }

    TString box_str[] = {"NorthUpper","SouthLower"};
    for (int i = 0; i < Nboxes; i++) {
	gDirectory->mkdir(box_str[i]+"Box")->cd();
	for (int j = 0; j < 2; j++) {
		TString strN = "_" + trig_str[j];
		TString strT = ", " + box_str[i] + " box " + trig_str[j] + " trigger";
		hHit_pixelOccupancy[i][j] = new TH2I("Hit_PixelOccupancy"+strN,"DIRCPmtHit pixel occupancy "+strT+"; pixel rows; pixel columns",Npixelcolumns,-0.5,-0.5+Npixelcolumns,Npixelrows,-0.5,-0.5+Npixelrows);
		hHit_TimeOverThreshold[i][j] = new TH1I("Hit_TimeOverThreshold"+strN,"DIRCPmtHit time-over-threshold "+strT+"; time-over-threshold (ns); hits",100,0.0,100.);
		hHit_TimeOverThresholdVsChannel[i][j] = new TH2I("Hit_TimeOverThresholdVsChannel"+strN,"DIRCPmtHit time-over-threshold vs channel "+strT+"; channel; time-over-threshold [ns]",Nchannels,-0.5,-0.5+Nchannels,100,0.0,100.);
		hHit_tdcTime[i][j] = new TH1I("Hit_Time"+strN,"DIRCPmtHit time "+strT+";time [ns]; hits",500,0.0,1000.0);
		hHit_tdcTimeVsChannel[i][j] = new TH2I("Hit_TimeVsChannel"+strN,"DIRCPmtHit time vs. channel "+strT+"; channel;time [ns]",Nchannels,-0.5,-0.5+Nchannels,100,0.0,100.0);
	}

	// LED specific histograms
	hHit_tdcTimeDiffVsChannel[i] = new TH2I("Hit_LEDTimeDiffVsChannel","LED DIRCPmtHit time diff vs. channel; channel;time [ns]",Nchannels,-0.5,-0.5+Nchannels,100,0.0,100.0);
	hHit_tdcTimeDiffEvent[i] = new TH1I("Hit_LEDTimeDiffEvent","LED DIRCPmtHit time diff in event; #Delta t [ns]",200,-50,50);
	
	if(i==1) {
		gDirectory->mkdir("Timewalk")->cd();	
		for(int j=0; j<Nchannels; j++) {
			hHit_Timewalk[i][j] = new TH2I(Form("Hit_Timewalk_%d",j),Form("DIRCPmtHit channel %d: #Delta t vs time-over-threshold; time-over-threshold [ns]; #Delta t [ns]",j),100,0,100,100,-50.,50.);
		}
	}

        hitDir->cd();
    }

    // DIRCTDC digihit-level hists
    dircDir->cd();
    TDirectory *digihitDir = gDirectory->mkdir("DigiHit"); digihitDir->cd();

    for (int j = 0; j < 2; j++) {
	hDigiHit_NtdcHits[j] = new TH1I("DigiHit_NHits_"+trig_str[j],"DIRCTDCDigiHit multiplicity "+ trig_str[j] +";hits;events",NmultBins,0.5,0.5+NmultDigi);
    	hDigiHit_Box[j] = new TH1I("DigiHit_Box_"+trig_str[j],"DIRCTDCDigiHit box" + trig_str[j] + ";box;hits",Nboxes,-0.5,-0.5+Nboxes);
    	hDigiHit_NtdcHitsVsBox[j] = new TH2I("DigiHit_NHitsVsBox_"+trig_str[j],"DIRCTDCDigiHit multiplicity vs box" + trig_str[j] +";box;hits",Nboxes,-0.5,-0.5+Nboxes,NmultBins,0.5,0.5+NmultDigi);
    }

    for (int i = 0; i < Nboxes; i++) {
        gDirectory->mkdir(box_str[i]+"Box")->cd();
	for (int j = 0; j < 2; j++) {
                TString strN = "_" + trig_str[j];
                TString strT = ", " + box_str[i] + " box " + trig_str[j] + " trigger";
		hDigiHit_pixelOccupancy[i][j] = new TH2I("TDCDigiHit_PixelOccupancy"+strN,"DIRCTDCDigiHit pixel occupancy"+strT+"; pixel rows; pixel columns",Npixelcolumns,-0.5,-0.5+Npixelcolumns,Npixelrows,-0.5,-0.5+Npixelrows);
		hDigiHit_tdcTime[i][j] = new TH1I("TDCDigiHit_Time"+strN,"DIRCTDCDigiHit time"+strT+";time [ns]; hits",500,0.0,500.0);
		hDigiHit_tdcTimeVsChannel[i][j] = new TH2I("TDCDigiHit_TimeVsChannel"+strN,"DIRCTDCDigiHit time"+strT+"; channel; time [ns]",Nchannels,-0.5,-0.5+Nchannels,500,0.0,500.0);
	}

	digihitDir->cd();
    }
    // back to main dir
    mainDir->cd();

    return NOERROR;
}


//----------------------------------------------------------------------------------


jerror_t JEventProcessor_DIRC_online::brun(JEventLoop *eventLoop, int32_t runnumber) {
    // This is called whenever the run number changes

    vector<const DDIRCGeometry*> locDIRCGeometry;
    eventLoop->Get(locDIRCGeometry);
    dDIRCGeometry = locDIRCGeometry[0];

    return NOERROR;
}


//----------------------------------------------------------------------------------


jerror_t JEventProcessor_DIRC_online::evnt(JEventLoop *eventLoop, uint64_t eventnumber) {
    // This is called for every event. Use of common resources like writing
    // to a file or filling a histogram should be mutex protected. Using
    // loop-Get(...) to get reconstructed objects (and thereby activating the
    // reconstruction algorithm) should be done outside of any mutex lock
    // since multiple threads may call this method at the same time.

    // Get data for DIRC
    vector<const DDIRCTDCDigiHit*> digihits;
    eventLoop->Get(digihits);
    vector<const DDIRCPmtHit*> hits;
    eventLoop->Get(hits);

    //const DTTabUtilities* ttabUtilities = nullptr;
    //eventLoop->GetSingle(ttabUtilities);

    // check for LED triggers
    bool locDIRCLEDTrig = false;
    //bool locPhysicsTrig = false;
    vector<const DL1Trigger*> trig;
    eventLoop->Get(trig);
    if (trig.size() > 0) {
	    // LED appears as "bit" 15 in L1 front panel trigger monitoring plots
	    if (trig[0]->fp_trig_mask & 0x4000){ 
		    locDIRCLEDTrig = true;
	    }
	    // Physics trigger appears as "bit" 1 in L1 trigger monitoring plots
	    //if (trig[0]->trig_mask & 0x1){ 
	    //	    locPhysicsTrig = true;
	    //}
    }
    int loc_itrig = 1;
    if(locDIRCLEDTrig) loc_itrig = 0;

    // LED specific information
    double locLEDRefTime = 0;
    if(locDIRCLEDTrig) {
	    
	    // Get LED SiPM reference
	    vector<const DCAEN1290TDCHit*> sipmtdchits;
	    eventLoop->Get(sipmtdchits);
	    vector<const Df250PulseData*> sipmadchits;
	    eventLoop->Get(sipmadchits);

	    for(uint i=0; i<sipmadchits.size(); i++) {
		    const Df250PulseData* sipmadchit = (Df250PulseData*)sipmadchits[i];
		    if(sipmadchit->rocid == 77 && sipmadchit->slot == 16 && sipmadchit->channel == 15) {
			    locLEDRefTime = (double)((sipmadchit->course_time<<6) + sipmadchit->fine_time);
			    locLEDRefTime *= 0.0625; // convert time from flash to ns
			    locLEDRefTime -= 115; // adjust to time of LED hits
			    japp->RootFillLock(this); //ACQUIRE ROOT FILL LOCK
			    hLEDRefTime->Fill(locLEDRefTime); 
			    hLEDRefIntegral->Fill(sipmadchit->integral); 
			    japp->RootFillUnLock(this); //ACQUIRE ROOT FILL LOCK
		    }
	    }
    }

    // FILL HISTOGRAMS
    // Since we are filling histograms local to this plugin, it will not interfere with other ROOT operations: can use plugin-wide ROOT fill lock
    japp->RootFillLock(this); //ACQUIRE ROOT FILL LOCK

    if (digihits.size() > 0) dirc_num_events->Fill(1);

    // Fill digihit hists
    int NDigiHits[] = {0,0};
    hDigiHit_NtdcHits[loc_itrig]->Fill(digihits.size());
    for (const auto& hit : digihits) {
	int box = (hit->channel < Nchannels) ? 1 : 0;
        int channel = (hit->channel < Nchannels) ? hit->channel : (hit->channel - Nchannels);
        NDigiHits[box]++;
        hDigiHit_Box[loc_itrig]->Fill(box);
	hDigiHit_pixelOccupancy[box][loc_itrig]->Fill(dDIRCGeometry->GetPixelRow(hit->channel), dDIRCGeometry->GetPixelColumn(hit->channel));
        hDigiHit_tdcTime[box][loc_itrig]->Fill(hit->time);
        hDigiHit_tdcTimeVsChannel[box][loc_itrig]->Fill(channel,hit->time);
    }
    hDigiHit_NtdcHitsVsBox[loc_itrig]->Fill(0.,NDigiHits[0]); hDigiHit_NtdcHitsVsBox[loc_itrig]->Fill(1.,NDigiHits[1]);

    // Loop over calibrated hits to get mean for reference time
    double locRefTime = 0;
    int locNHits = 0;
    for (const auto& hit : hits) {
        int channel = (hit->ch < Nchannels) ? hit->ch : (hit->ch - Nchannels);
        int pmtrow = dDIRCGeometry->GetPmtRow(channel);
        if(pmtrow < 6 && fabs(hit->t-10.) < 5.) {
		//cout<<pmtrow<<" "<<hit->t<<endl;
		locRefTime += hit->t;
		locNHits++;
	}
        else if(pmtrow > 5 && pmtrow < 12 && fabs(hit->t-20.) < 5.) {
		//cout<<pmtrow<<" "<<hit->t<<endl;
		locRefTime += (hit->t - 10);
		locNHits++;
	}
        else if(pmtrow > 11 && pmtrow < 18 && fabs(hit->t-30.) < 5.) {
		//cout<<pmtrow<<" "<<hit->t<<endl;
		locRefTime += (hit->t - 20);
		locNHits++;
	}
	//cout<<"locRefTime "<<locRefTime<<"  "<<locNHits<<endl;
    }
    locRefTime /= locNHits;
    hRefTime->Fill(locRefTime);
    
    // Fill calibrated-hit hists
    int NHits[] = {0,0};
    bool ledFiber[3] = {false, false, false};
    for (const auto& hit : hits) {
	int box = (hit->ch < Nchannels) ? 1 : 0;
        int channel = (hit->ch < Nchannels) ? hit->ch : (hit->ch - Nchannels);
	int pmtrow = dDIRCGeometry->GetPmtRow(channel);
	if(pmtrow < 6) ledFiber[0] = true;
	else if(pmtrow < 12) ledFiber[1] = true;
	else ledFiber[2] = true; 
	hHit_Box[loc_itrig]->Fill(box);
	NHits[box]++;
	hHit_pixelOccupancy[box][loc_itrig]->Fill(dDIRCGeometry->GetPixelRow(hit->ch), dDIRCGeometry->GetPixelColumn(hit->ch));
	hHit_TimeOverThreshold[box][loc_itrig]->Fill(hit->tot);
	hHit_TimeOverThresholdVsChannel[box][loc_itrig]->Fill(channel,hit->tot);
	hHit_tdcTime[box][loc_itrig]->Fill(hit->t);
	hHit_tdcTimeVsChannel[box][loc_itrig]->Fill(channel,hit->t);

	// LED specific histograms
	if(locDIRCLEDTrig) {
		hHit_tdcTimeDiffEvent[box]->Fill(hit->t-locRefTime);
		hHit_tdcTimeDiffVsChannel[box]->Fill(channel,hit->t-locLEDRefTime);
	
		double locDeltaT = hit->t - locRefTime;
		if(ledFiber[1]) locDeltaT -= 10.;
		if(ledFiber[2]) locDeltaT -= 20.;
		hHit_Timewalk[box][channel]->Fill(hit->tot, locDeltaT);
	}

    }
    hHit_NHits[loc_itrig]->Fill(NHits[0]+NHits[1]);
    hHit_NHitsVsBox[loc_itrig]->Fill(0.,NHits[0]); hHit_NHitsVsBox[loc_itrig]->Fill(1.,NHits[1]);

    japp->RootFillUnLock(this); //RELEASE ROOT FILL LOCK

    return NOERROR;
}


//----------------------------------------------------------------------------------


jerror_t JEventProcessor_DIRC_online::erun(void) {
    // This is called whenever the run number changes, before it is
    // changed to give you a chance to clean up before processing
    // events from the next run number.
    return NOERROR;
}


//----------------------------------------------------------------------------------


jerror_t JEventProcessor_DIRC_online::fini(void) {
    // Called before program exit after event processing is finished.
    return NOERROR;
}


//----------------------------------------------------------------------------------
//----------------------------------------------------------------------------------
