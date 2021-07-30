// $Id$
//
//    File: DCustomAction_TrackingEfficiency.cc
// Created: Wed Feb 25 09:38:06 EST 2015
// Creator: pmatt (on Linux pmattdesktop.jlab.org 2.6.32-504.8.1.el6.x86_64 x86_64)
//

#include "DCustomAction_TrackingEfficiency.h"
#include "TObjString.h"

#include <TRACKING/DMCThrown.h>

static int dInitNumThrownArraySize = 20;


// these next three functions are basically taken from the DSelector library for the sake of consistency
map<Particle_t, UInt_t> DCustomAction_TrackingEfficiency::Get_NumFinalStateThrown(const vector<const DMCThrown*>& locMCThrowns) const
{
	//the # of thrown final-state particles (+ pi0) of each type (multiplexed in base 10)
	map<Particle_t, UInt_t> locNumFinalStateThrown;
	
	for(auto &thrown : locMCThrowns) {
		Particle_t locPID = thrown->PID();
		if( locNumFinalStateThrown.find(locPID) == locNumFinalStateThrown.end() ) {
			locNumFinalStateThrown[locPID] = 1;
		} else {
			locNumFinalStateThrown[locPID]++;
		}
	}

	return locNumFinalStateThrown;
}

vector<UInt_t> DCustomAction_TrackingEfficiency::Get_ThrownDecayingPIDs(const vector<const DMCThrown*>& locMCThrowns) const
{
/*
	//the types of the thrown decaying particles in the event 
	//not the quantity of each, just whether or not they were present (1 or 0)
	vector<Particle_t> locDecayingThrown;

	for(auto &thrown : locMCThrowns) {
		Particle_t locPID = thrown->PID();
		if( find(locDecayingThrown.begin(), locDecayingThrown.end(), locPID) == locDecayingThrown.end() ) {
			locDecayingThrown.push_back(locPID);
		} 
	}
*/
	// use a set so we don't have to worry about multiples
	set<Particle_t> locDecayingThrown;

	for(auto &thrown : locMCThrowns) {
		Particle_t locPID = thrown->PID();
		locDecayingThrown.insert(locPID);
	}
	
	// switch to a vector so we can do things like saving this info in ROOT trees
	vector<UInt_t> locDecayingThrownVec;
	std::copy(locDecayingThrown.begin(), locDecayingThrown.end(), std::back_inserter(locDecayingThrownVec));

	return locDecayingThrownVec;
}


TString DCustomAction_TrackingEfficiency::Get_ThrownTopologyString(JEventLoop* locEventLoop) const
{
	TString locReactionName;
	int locNumPi0s = 0;

	vector<const DMCThrown*> locMCThrowns_FinalState;
	locEventLoop->Get(locMCThrowns_FinalState, "FinalState");

	vector<const DMCThrown*> locMCThrowns_Decaying;
	locEventLoop->Get(locMCThrowns_Decaying, "Decaying");

	// Get final state particles and multiplicity
	map<Particle_t, UInt_t> locNumFinalStateThrown = Get_NumFinalStateThrown(locMCThrowns_FinalState);
	for(auto const& locParticle : locNumFinalStateThrown) {
		Particle_t locPID = locParticle.first; 
		int locNumParticles = locParticle.second;
		if(locNumParticles > 0) {
			// List pi0s with decaying particles, since all final state photons are given
			if(locPID == Pi0) {
				locNumPi0s = locNumParticles;
				continue;
			}

			if(locNumParticles > 1) locReactionName += locNumParticles;
			locReactionName += ParticleName_ROOT(locPID);
		}
	}
/*
	// Get list of decaying particles (if they exist)
	vector<UInt_t> locThrownDecayingPIDs = Get_ThrownDecayingPIDs(locMCThrowns_Decaying);
	if(locThrownDecayingPIDs.size() > 0 || locNumPi0s > 0) {
		locReactionName += "[";
		if(locNumPi0s == 1) locReactionName += "#pi^{0}";
		else if(locNumPi0s > 1) {
			locReactionName += locNumPi0s;
			locReactionName += "#pi^{0}";
		}
					  
		// add comma if previous decaying particle exists
		bool locAddComma = false; 
		if(locNumPi0s > 0) locAddComma = true;

		for(auto locPID : locThrownDecayingPIDs ) {
			//Particle_t locPID = locThrownDecayingPIDs[i]; 
			if(locAddComma) locReactionName += ",";
			locReactionName += ParticleName_ROOT(locPID);
			locAddComma = true;
		}
		locReactionName += "]";
	}
*/
	return locReactionName;
}

void DCustomAction_TrackingEfficiency::Initialize(JEventLoop* locEventLoop)
{
	//Optional: Create histograms and/or modify member variables.
	//Create any histograms/trees/etc. within a ROOT lock. 
		//This is so that when running multithreaded, only one thread is writing to the ROOT file at a time. 

	vector<const DMCThrown*> locMCThrowns;
	locEventLoop->Get(locMCThrowns);
	bool locIsMC = locMCThrowns.size() > 0;

	const DReaction* locReaction = Get_Reaction();

	auto locMissingPIDs = Get_Reaction()->Get_MissingPIDs();
	dMissingPID = (locMissingPIDs.size() == 1) ? locMissingPIDs[0] : Unknown;
	if(locMissingPIDs.size() != 1)
		return; //invalid reaction setup

	Run_Update(locEventLoop);

	//CREATE TTREE, TFILE
	dTreeInterface = DTreeInterface::Create_DTreeInterface(locReaction->Get_ReactionName(), "tree_trackeff.root");

	//TTREE BRANCHES
	DTreeBranchRegister locBranchRegister;

	//USER INFO
	TList* locUserInfo = locBranchRegister.Get_UserInfo();
	TMap* locMiscInfoMap = new TMap(); //collection of pairs
	locMiscInfoMap->SetName("MiscInfoMap");
	locUserInfo->Add(locMiscInfoMap);
	//set reaction name
	locMiscInfoMap->Add(new TObjString("ReactionName"), new TObjString(locReaction->Get_ReactionName().c_str()));
	//set pid
	ostringstream locOStream;
	locOStream << PDGtype(dMissingPID);
	locMiscInfoMap->Add(new TObjString("MissingPID_PDG"), new TObjString(locOStream.str().c_str()));
	//set run#
	locOStream.str("");
	locOStream << locEventLoop->GetJEvent().GetRunNumber();
	locMiscInfoMap->Add(new TObjString("RunNumber"), new TObjString(locOStream.str().c_str()));

	//CHANNEL INFO
	locBranchRegister.Register_Single<ULong64_t>("EventNumber"); //for debugging
	locBranchRegister.Register_Single<Float_t>("BeamEnergy");
	locBranchRegister.Register_Single<Float_t>("BeamRFDeltaT");
	locBranchRegister.Register_Single<Float_t>("ComboVertexZ");
	locBranchRegister.Register_Single<UChar_t>("NumExtraTracks");
	locBranchRegister.Register_Single<Float_t>("MissingMassSquared"); //is measured
	locBranchRegister.Register_Single<Float_t>("KinFitChiSq"); //is -1 if no kinfit or failed to converge
	locBranchRegister.Register_Single<UInt_t>("KinFitNDF"); //is 0 if no kinfit or failed to converged 
	locBranchRegister.Register_Single<TVector3>("MissingP3"); //is kinfit if kinfit (& converged)

	if(locIsMC) {
		// Thrown info
		locBranchRegister.Register_Single<TLorentzVector>("ThrownBeam_X4"); //reported at target center
		locBranchRegister.Register_Single<TLorentzVector>("ThrownBeam_P4");
		locBranchRegister.Register_Single<Float_t>("ThrownBeam_GeneratedEnergy");
		//locBranchRegister.Register_FundamentalArray<UInt_t>("ThrownDecayingPIDs");

		locBranchRegister.Register_Single<UInt_t>("NumThrown");
		locBranchRegister.Register_FundamentalArray<Int_t>("Thrown_PID", "NumThrown", dInitNumThrownArraySize);
		locBranchRegister.Register_ClonesArray<TLorentzVector>("Thrown_X4", dInitNumThrownArraySize);
		locBranchRegister.Register_ClonesArray<TLorentzVector>("Thrown_P4", dInitNumThrownArraySize);
	}

	//MISSING P3 ERROR MATRIX //is kinfit if kinfit (& converged)
	locBranchRegister.Register_Single<Float_t>("MissingP3_CovPxPx");
	locBranchRegister.Register_Single<Float_t>("MissingP3_CovPxPy");
	locBranchRegister.Register_Single<Float_t>("MissingP3_CovPxPz");
	locBranchRegister.Register_Single<Float_t>("MissingP3_CovPyPy");
	locBranchRegister.Register_Single<Float_t>("MissingP3_CovPyPz");
	locBranchRegister.Register_Single<Float_t>("MissingP3_CovPzPz");

	//TRACKING INFO:
	locBranchRegister.Register_Single<UInt_t>("NumUnusedWireBased");
	locBranchRegister.Register_Single<UInt_t>("NumUnusedTimeBased");
	locBranchRegister.Register_FundamentalArray<Float_t>("ReconMatchFOM_WireBased", "NumUnusedWireBased"); //FOM < 0 if nothing, no-match
	locBranchRegister.Register_FundamentalArray<Float_t>("ReconTrackingFOM_WireBased", "NumUnusedWireBased"); //FOM < 0 if nothing, no-match
	locBranchRegister.Register_ClonesArray<TVector3>("ReconP3_WireBased"); //wire-based
	locBranchRegister.Register_FundamentalArray<Float_t>("ReconMatchFOM", "NumUnusedTimeBased"); //FOM < 0 if nothing, no-match (time-based)
	locBranchRegister.Register_FundamentalArray<Float_t>("ReconTrackingFOM", "NumUnusedTimeBased"); //FOM < 0 if nothing, no-match (time-based)
	locBranchRegister.Register_ClonesArray<TVector3>("ReconP3"); //time-based (time-based)
	locBranchRegister.Register_FundamentalArray<Float_t>("MeasuredMissingE", "NumUnusedTimeBased"); //includes recon time-based track if found, else is -999.0
	locBranchRegister.Register_FundamentalArray<UInt_t>("TrackCDCRings", "NumUnusedTimeBased"); //rings correspond to bits (1 -> 28)
	locBranchRegister.Register_FundamentalArray<UInt_t>("TrackFDCPlanes", "NumUnusedTimeBased"); //planes correspond to bits (1 -> 24)

	//RECON P3 ERROR MATRIX
	locBranchRegister.Register_FundamentalArray<Float_t>("ReconP3_CovPxPx", "NumUnusedTimeBased");
	locBranchRegister.Register_FundamentalArray<Float_t>("ReconP3_CovPxPy", "NumUnusedTimeBased");
	locBranchRegister.Register_FundamentalArray<Float_t>("ReconP3_CovPxPz", "NumUnusedTimeBased");
	locBranchRegister.Register_FundamentalArray<Float_t>("ReconP3_CovPyPy", "NumUnusedTimeBased");
	locBranchRegister.Register_FundamentalArray<Float_t>("ReconP3_CovPyPz", "NumUnusedTimeBased");
	locBranchRegister.Register_FundamentalArray<Float_t>("ReconP3_CovPzPz", "NumUnusedTimeBased");

	//REGISTER BRANCHES
	dTreeInterface->Create_Branches(locBranchRegister);
}

bool DCustomAction_TrackingEfficiency::Perform_Action(JEventLoop* locEventLoop, const DParticleCombo* locParticleCombo)
{
	//Write custom code to perform an action on the INPUT DParticleCombo (DParticleCombo)
	//NEVER: Grab DParticleCombo or DAnalysisResults objects (of any tag!) from the JEventLoop within this function
	//NEVER: Grab objects that are created post-kinfit (e.g. DKinFitResults, etc.) from the JEventLoop if Get_UseKinFitResultsFlag() == false: CAN CAUSE INFINITE DEPENDENCY LOOP

	vector<const DMCThrown*> locMCThrowns;
	locEventLoop->Get(locMCThrowns);
	bool locIsMC = locMCThrowns.size() > 0;

	const DDetectorMatches* locDetectorMatches = NULL;
	locEventLoop->GetSingle(locDetectorMatches);

	vector<const DBCALShower*> locBCALShowers;
	locEventLoop->Get(locBCALShowers);

	vector<const DChargedTrack*> locUnusedChargedTracks;
	dAnalysisUtilities->Get_UnusedChargedTracks(locEventLoop, locParticleCombo, locUnusedChargedTracks);

	const DEventRFBunch* locEventRFBunch = locParticleCombo->Get_EventRFBunch();
	const DKinFitResults* locKinFitResults = locParticleCombo->Get_KinFitResults();

	/*********************************************** MISSING PARTICLE INFO ***********************************************/

	if(dMissingPID == Unknown)
		return true; //invalid reaction setup
	if(ParticleCharge(dMissingPID) == 0)
		return true; //NOT SUPPORTED

	auto locMissingParticle = (locParticleCombo->Get_MissingParticles(Get_Reaction()))[0]; //is NULL if no kinfit!!
	const DParticleComboStep* locParticleComboStep = locParticleCombo->Get_ParticleComboStep(0);

	// Get missing particle p4 & covariance
	DLorentzVector locMeasuredMissingP4 = dAnalysisUtilities->Calc_MissingP4(Get_Reaction(), locParticleCombo, false);
	DVector3 locMissingP3 = locMeasuredMissingP4.Vect();
	TMatrixDSym locMissingCovarianceMatrix(3);
	const DKinematicData* locBeamParticle = NULL;
	if(locKinFitResults == NULL) //no kinfit (yet?), or kinfit failed
	{
		TMatrixFSym locFMissingCovarianceMatrix = dAnalysisUtilities->Calc_MissingP3Covariance(Get_Reaction(), locParticleCombo);
		for(unsigned int loc_q = 0; loc_q < 3; ++loc_q)
		{
			for(unsigned int loc_r = 0; loc_r < 3; ++loc_r)
				locMissingCovarianceMatrix(loc_q, loc_r) = locFMissingCovarianceMatrix(loc_q, loc_r);
		}
		locBeamParticle = locParticleComboStep->Get_InitialParticle_Measured();
	}
	else //kinfit succeeded
	{
		locMissingP3 = locMissingParticle->momentum();
		const TMatrixFSym& locKinFitCovarianceMatrix = *(locMissingParticle->errorMatrix());
		for(unsigned int loc_q = 0; loc_q < 3; ++loc_q)
		{
			for(unsigned int loc_r = 0; loc_r < 3; ++loc_r)
				locMissingCovarianceMatrix(loc_q, loc_r) = locKinFitCovarianceMatrix(loc_q, loc_r);
		}
		locBeamParticle = locParticleComboStep->Get_InitialParticle();
	}
	
	double locVertexZ = locParticleCombo->Get_EventVertex().Z();
	double locBeamRFDeltaT = locBeamParticle->time() - locEventRFBunch->dTime;
	
	vector<const DMCThrown*> locMCThrowns_FinalState;
	vector<const DMCThrown*> locMCThrowns_Decaying;
	vector<UInt_t> locDecayingPIDs;
	vector<const DBeamPhoton*> locMCGenBeams;
	vector<const DBeamPhoton*> locTaggedMCGenBeams;
	if(locIsMC) {
		// thrown data
		//TString locThrownTopologyString = Get_ThrownTopologyString(locEventLoop);
		locEventLoop->Get(locMCGenBeams, "MCGEN");
		locEventLoop->Get(locTaggedMCGenBeams, "TAGGEDMCGEN");

		locEventLoop->Get(locMCThrowns_FinalState, "FinalState");
		locEventLoop->Get(locMCThrowns_Decaying, "Decaying");
		//locDecayingPIDs = Get_ThrownDecayingPIDs(locMCThrowns_Decaying);
	}
	const DBeamPhoton* locTaggedMCGenBeam = locTaggedMCGenBeams.empty() ? locMCGenBeams[0] : locTaggedMCGenBeams[0]; //if empty: will have to do. 

	//get number of extra tracks that have at least 10 hits
	size_t locNumExtraTracks = 0;
	for(auto locChargedTrack : locUnusedChargedTracks)
	{
		auto locBestHypothesis = locChargedTrack->Get_BestFOM();
		auto locNumTrackHits = locBestHypothesis->Get_TrackTimeBased()->Ndof + 5;
		if(locNumTrackHits >= 10)
			++locNumExtraTracks;
	}

	//kinfit results are unique for each DParticleCombo: no need to check for duplicates

	//FILL CHANNEL INFO
	dTreeFillData.Fill_Single<ULong64_t>("EventNumber", locEventLoop->GetJEvent().GetEventNumber());
	dTreeFillData.Fill_Single<Float_t>("BeamEnergy", locBeamParticle->energy());
	dTreeFillData.Fill_Single<Float_t>("BeamRFDeltaT", locBeamRFDeltaT);
	dTreeFillData.Fill_Single<UChar_t>("NumExtraTracks", (UChar_t)locNumExtraTracks);
	dTreeFillData.Fill_Single<Float_t>("MissingMassSquared", locMeasuredMissingP4.M2());
	dTreeFillData.Fill_Single<Float_t>("ComboVertexZ", locVertexZ);
	if(locKinFitResults == NULL) //is true if no kinfit or failed to converged
	{
		dTreeFillData.Fill_Single<Float_t>("KinFitChiSq", -1.0);
		dTreeFillData.Fill_Single<UInt_t>("KinFitNDF", 0);
	}
	else
	{
		dTreeFillData.Fill_Single<Float_t>("KinFitChiSq", locKinFitResults->Get_ChiSq());
		dTreeFillData.Fill_Single<UInt_t>("KinFitNDF", locKinFitResults->Get_NDF());
	}
	TVector3 locTMissingP3(locMissingP3.Px(), locMissingP3.Py(), locMissingP3.Pz());
	dTreeFillData.Fill_Single<TVector3>("MissingP3", locTMissingP3); //is kinfit if kinfit

	if(locIsMC) {

		//FILL THROWN INFO		
		const DMCReaction* locMCReaction = NULL;
		locEventLoop->GetSingle(locMCReaction);
		dTreeFillData.Fill_Single<Float_t>("ThrownBeam_GeneratedEnergy", locMCReaction->beam.energy());

		DVector3 locThrownBeamX3 = locMCReaction->beam.position();
		TLorentzVector locThrownBeamTX4(locThrownBeamX3.X(), locThrownBeamX3.Y(), locThrownBeamX3.Z(), locMCReaction->beam.time());
		dTreeFillData.Fill_Single<TLorentzVector>("ThrownBeam_X4", locThrownBeamTX4);

		DLorentzVector locThrownBeamP4 = locTaggedMCGenBeam->lorentzMomentum();
		TLorentzVector locThrownBeamTP4(locThrownBeamP4.Px(), locThrownBeamP4.Py(), locThrownBeamP4.Pz(), locThrownBeamP4.E());
		dTreeFillData.Fill_Single<TLorentzVector>("ThrownBeam_P4", locThrownBeamTP4);

		
		//for(int locArrayIndex=0; locArrayIndex<locDecayingPIDs.size(); locArrayIndex++)
		//	dTreeFillData.Fill_Array<UInt_t>("ThrownDecayingPIDs", locDecayingPIDs[locArrayIndex], locArrayIndex);
		dTreeFillData.Fill_Single<UInt_t>("NumThrown", locMCThrowns_FinalState.size());
		for(int locArrayIndex=0; locArrayIndex<locMCThrowns_FinalState.size(); locArrayIndex++) {
			TLorentzVector locX4_Thrown(locMCThrowns_FinalState[locArrayIndex]->position().X(), 
										locMCThrowns_FinalState[locArrayIndex]->position().Y(), 
										locMCThrowns_FinalState[locArrayIndex]->position().Z(), 
										locMCThrowns_FinalState[locArrayIndex]->time());
			dTreeFillData.Fill_Array<TLorentzVector>("Thrown_X4", locX4_Thrown, locArrayIndex);
			TLorentzVector locP4_Thrown(locMCThrowns_FinalState[locArrayIndex]->momentum().X(), 
										locMCThrowns_FinalState[locArrayIndex]->momentum().Y(), 
										locMCThrowns_FinalState[locArrayIndex]->momentum().Z(), 
										locMCThrowns_FinalState[locArrayIndex]->energy());
			dTreeFillData.Fill_Array<TLorentzVector>("Thrown_P4", locP4_Thrown, locArrayIndex);
		}
		
	}

	//MISSING P3 ERROR MATRIX //is kinfit if kinfit (& converged)
	dTreeFillData.Fill_Single<Float_t>("MissingP3_CovPxPx", locMissingCovarianceMatrix(0, 0));
	dTreeFillData.Fill_Single<Float_t>("MissingP3_CovPxPy", locMissingCovarianceMatrix(0, 1));
	dTreeFillData.Fill_Single<Float_t>("MissingP3_CovPxPz", locMissingCovarianceMatrix(0, 2));
	dTreeFillData.Fill_Single<Float_t>("MissingP3_CovPyPy", locMissingCovarianceMatrix(1, 1));
	dTreeFillData.Fill_Single<Float_t>("MissingP3_CovPyPz", locMissingCovarianceMatrix(1, 2));
	dTreeFillData.Fill_Single<Float_t>("MissingP3_CovPzPz", locMissingCovarianceMatrix(2, 2));

	/************************************************* WIRE-BASED TRACKS *************************************************/

	if(!locEventLoop->GetJEvent().GetStatusBit(kSTATUS_REST))
	{
		//Get unused tracks
		vector<const DTrackWireBased*> locUnusedWireBasedTracks;
		dAnalysisUtilities->Get_UnusedWireBasedTracks(locEventLoop, locParticleCombo, locUnusedWireBasedTracks);

		//loop over unused tracks
		size_t locNumWireBasedTracks = 0;
		for(size_t loc_i = 0; loc_i < locUnusedWireBasedTracks.size(); ++loc_i)
		{
			const DTrackWireBased* locWireBasedTrack = locUnusedWireBasedTracks[loc_i];
			if(locWireBasedTrack->PID() != dMissingPID)
				continue; //only use tracking results with correct PID

			DVector3 locWireBasedDP3 = locWireBasedTrack->momentum();
			TVector3 locWireBasedP3(locWireBasedDP3.X(), locWireBasedDP3.Y(), locWireBasedDP3.Z());
			dTreeFillData.Fill_Array<TVector3>("ReconP3_WireBased", locWireBasedP3, locNumWireBasedTracks);
			dTreeFillData.Fill_Array<Float_t>("ReconTrackingFOM_WireBased", locWireBasedTrack->FOM, locNumWireBasedTracks);

			const TMatrixFSym& locCovarianceMatrix = *(locWireBasedTrack->errorMatrix());
			TMatrixDSym locDCovarianceMatrix(3);
			for(unsigned int loc_j = 0; loc_j < 3; ++loc_j)
			{
				for(unsigned int loc_k = 0; loc_k < 3; ++loc_k)
					locDCovarianceMatrix(loc_j, loc_k) = locCovarianceMatrix(loc_j, loc_k);
			}
			locDCovarianceMatrix += locMissingCovarianceMatrix;

			//invert matrix
			TDecompLU locDecompLU(locDCovarianceMatrix);
			//check to make sure that the matrix is decomposable and has a non-zero determinant
			if(locDecompLU.Decompose() && (fabs(locDCovarianceMatrix.Determinant()) >= 1.0E-300))
			{
				locDCovarianceMatrix.Invert();
				DVector3 locDeltaP3 = locWireBasedTrack->momentum() - locMissingP3;
				double locMatchFOM = Calc_MatchFOM(locDeltaP3, locDCovarianceMatrix);
				dTreeFillData.Fill_Array<Float_t>("ReconMatchFOM_WireBased", locMatchFOM, locNumWireBasedTracks);
			}
			else //not invertable
				dTreeFillData.Fill_Array<Float_t>("ReconMatchFOM_WireBased", -1.0, locNumWireBasedTracks);

			++locNumWireBasedTracks;
		}
		dTreeFillData.Fill_Single<UInt_t>("NumUnusedWireBased", locNumWireBasedTracks);
	}
	else //is a REST event, no wire-based tracks
		dTreeFillData.Fill_Single<UInt_t>("NumUnusedWireBased", 0);

	/************************************************* TIME-BASED TRACKS *************************************************/

	//Get unused tracks
	vector<const DTrackTimeBased*> locUnusedTimeBasedTracks;
	dAnalysisUtilities->Get_UnusedTimeBasedTracks(locEventLoop, locParticleCombo, locUnusedTimeBasedTracks);

	//loop over unused tracks
	size_t locNumTimeBasedTracks = 0;
	for(size_t loc_i = 0; loc_i < locUnusedTimeBasedTracks.size(); ++loc_i)
	{
		const DTrackTimeBased* locTimeBasedTrack = locUnusedTimeBasedTracks[loc_i];
		if(locTimeBasedTrack->PID() != dMissingPID)
			continue; //only use tracking results with correct PID

		DVector3 locTimeBasedDP3 = locTimeBasedTrack->momentum();
		TVector3 locTimeBasedP3(locTimeBasedDP3.X(), locTimeBasedDP3.Y(), locTimeBasedDP3.Z());
		dTreeFillData.Fill_Array<TVector3>("ReconP3", locTimeBasedP3, locNumTimeBasedTracks);
		dTreeFillData.Fill_Array<Float_t>("ReconTrackingFOM", locTimeBasedTrack->FOM, locNumTimeBasedTracks);

		const TMatrixFSym& locCovarianceMatrix = *(locTimeBasedTrack->errorMatrix());
		TMatrixDSym locDCovarianceMatrix(3);
		for(unsigned int loc_j = 0; loc_j < 3; ++loc_j)
		{
			for(unsigned int loc_k = 0; loc_k < 3; ++loc_k)
				locDCovarianceMatrix(loc_j, loc_k) = locCovarianceMatrix(loc_j, loc_k);
		}
		locDCovarianceMatrix += locMissingCovarianceMatrix;

		//invert matrix
		TDecompLU locDecompLU(locDCovarianceMatrix);
		//check to make sure that the matrix is decomposable and has a non-zero determinant
		if(locDecompLU.Decompose() && (fabs(locDCovarianceMatrix.Determinant()) >= 1.0E-300))
		{
			locDCovarianceMatrix.Invert();
			DVector3 locDeltaP3 = locTimeBasedTrack->momentum() - locMissingP3;
			double locMatchFOM = Calc_MatchFOM(locDeltaP3, locDCovarianceMatrix);
			dTreeFillData.Fill_Array<Float_t>("ReconMatchFOM", locMatchFOM, locNumTimeBasedTracks);
		}
		else //not invertible
			dTreeFillData.Fill_Array<Float_t>("ReconMatchFOM", -1.0, locNumTimeBasedTracks);

		DLorentzVector locTotalMeasuredMissingP4 = locMeasuredMissingP4 - locTimeBasedTrack->lorentzMomentum();
		dTreeFillData.Fill_Array<Float_t>("MeasuredMissingE", locTotalMeasuredMissingP4.E(), locNumTimeBasedTracks);
		dTreeFillData.Fill_Array<UInt_t>("TrackCDCRings", locTimeBasedTrack->dCDCRings, locNumTimeBasedTracks);
		dTreeFillData.Fill_Array<UInt_t>("TrackFDCPlanes", locTimeBasedTrack->dFDCPlanes, locNumTimeBasedTracks);

		//RECON P3 ERROR MATRIX
		dTreeFillData.Fill_Array<Float_t>("ReconP3_CovPxPx", locCovarianceMatrix(0, 0), locNumTimeBasedTracks);
		dTreeFillData.Fill_Array<Float_t>("ReconP3_CovPxPy", locCovarianceMatrix(0, 1), locNumTimeBasedTracks);
		dTreeFillData.Fill_Array<Float_t>("ReconP3_CovPxPz", locCovarianceMatrix(0, 2), locNumTimeBasedTracks);
		dTreeFillData.Fill_Array<Float_t>("ReconP3_CovPyPy", locCovarianceMatrix(1, 1), locNumTimeBasedTracks);
		dTreeFillData.Fill_Array<Float_t>("ReconP3_CovPyPz", locCovarianceMatrix(1, 2), locNumTimeBasedTracks);
		dTreeFillData.Fill_Array<Float_t>("ReconP3_CovPzPz", locCovarianceMatrix(2, 2), locNumTimeBasedTracks);

		++locNumTimeBasedTracks;
	}

	dTreeFillData.Fill_Single<UInt_t>("NumUnusedTimeBased", locNumTimeBasedTracks);

	//FILL TTREE
	dTreeInterface->Fill(dTreeFillData);
	return true;
}

double DCustomAction_TrackingEfficiency::Calc_MatchFOM(const DVector3& locDeltaP3, TMatrixDSym locInverse3x3Matrix) const
{
	DMatrix locDeltas(3, 1);
	locDeltas(0, 0) = locDeltaP3.Px();
	locDeltas(1, 0) = locDeltaP3.Py();
	locDeltas(2, 0) = locDeltaP3.Pz();

	double locChiSq = (locInverse3x3Matrix.SimilarityT(locDeltas))(0, 0);
	return TMath::Prob(locChiSq, 3);
}
