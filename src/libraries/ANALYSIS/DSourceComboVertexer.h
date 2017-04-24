#ifndef DSourceComboVertexer_h
#define DSourceComboVertexer_h

#include <deque>
#include <set>
#include <unordered_map>
#include <utility>
#include <memory>
#include <vector>
#include <algorithm>
#include <iostream>
#include <sstream>

#include "JANA/JEventLoop.h"

#include "particleType.h"
#include "PID/DChargedTrackHypothesis.h"
#include "PID/DChargedTrack.h"
#include "PID/DNeutralShower.h"
#include "ANALYSIS/DReaction.h"
#include "ANALYSIS/DReactionStepVertexInfo.h"
#include "ANALYSIS/DKinFitUtils_GlueX.h"
#include "ANALYSIS/DAnalysisUtilities.h"

using namespace std;

namespace DAnalysis
{

class DSourceComboVertexer
{
	public:

		//CONSTRUCTORS
		DSourceComboVertexer(void) = delete;
		DSourceComboVertexer(JEventLoop* locEventLoop, DSourceComboer* locSourceComboer, DSourceComboP4Handler* locSourceComboP4Handler);
		void Reset(void);

		void Calc_VertexTimeOffsets(const DReactionVertexInfo* locReactionVertexInfo, const DSourceCombo* locReactionCombo);

		bool Get_VertexDeterminableWithCharged(const DReactionStepVertexInfo* locStepVertexInfo) const{return dVertexDeterminableWithChargedMap.find(locStepVertexInfo)->second;}
		pair<DVector3, double> Get_VertexTimeOffset(const DSourceCombo* locReactionChargedCombo, const DReactionStepVertexInfo* locStepVertexInfo) const{return dVertexTimeOffsets.find(locReactionChargedCombo)->second.find(locStepVertexInfo)->second;}
		pair<DVector3, double> Get_VertexTimeOffset(const DSourceCombo* locChargedCombo) const{return dVertexTimeOffsetsByCombo.find(locChargedCombo)->second;}

		signed char Get_VertexZBin(const DSourceCombo* locReactionChargedCombo, const DReactionStepVertexInfo* locStepVertexInfo) const{return Get_PhotonVertexZBin(Get_VertexTimeOffset(locReactionChargedCombo, locStepVertexInfo).first.Z());}
		signed char Get_VertexZBin(const DSourceCombo* locChargedCombo) const{return Get_PhotonVertexZBin(Get_VertexTimeOffset(locChargedCombo).first.Z());}
		vector<signed char> Get_VertexZBins(const DReactionVertexInfo* locVertexInfo, const DSourceCombo* locReactionCombo) const;

		//UTILITY FUNCTIONS
		size_t Get_PhotonVertexZBin(double locVertexZ) const;
		double Get_PhotonVertexZBinCenter(signed char locVertexZBin) const;

	private:
		DKinFitUtils_GlueX* dKinFitUtils;

		vector<const DKinematicData*>::const_iterator Get_ThetaNearest90Iterator(const vector<const DKinematicData*>& locParticles);
		vector<const DKinematicData*> Get_FullConstrainDecayingParticles(const DReactionStepVertexInfo* locStepVertexInfo, const map<pair<int, int>, const DKinematicData*>& locReconDecayParticleMap);
		void Construct_DecayingParticle(const DReactionStepVertexInfo* locReactionStepVertexInfo, const DSourceCombo* locSourceCombo, DVector3 locVertex, map<pair<int, int>, const DKinematicData*>& locReconDecayParticleMap);
		void Calc_TimeOffsets(const DSourceCombo* locReactionCombo);
		void Set_VertexTimeOffsets_ByCombo(const DSourceCombo* locVertexPrimaryCombo, const pair<DVector3, double>& locVertexTimeOffsetPair);

		DSourceComboer* dSourceComboer;
		DSourceComboP4Handler* dSourceComboP4Handler;
		const DAnalysisUtilities* dAnalysisUtilities;

		//EXPERIMENT INFORMATION
		DVector3 dTargetCenter;
		double dTargetLength = 30.0;
		double dTargetCenterZ;
		double dMinThetaForVertex = 30.0;

		//VERTEX-DEPENDENT PHOTON INFORMATION
		//For every 10cm in vertex-z, calculate the photon p4 & time for placing mass & delta-t cuts
		//The z-range extends from the upstream end of the target - 5cm to the downstream end + 15cm
		//so for a 30-cm-long target, it's a range of 50cm: 5bins, evaluated at the center of each bin
		float dPhotonVertexZBinWidth = 10.0;
		float dPhotonVertexZRangeLow = 45.0;
		size_t dNumPhotonVertexZBins = 5;

		//VERTICES AND TIME OFFSETS
		unordered_map<const DReactionStepVertexInfo*, bool> dVertexDeterminableWithChargedMap; //excludes dangling vertex infos!!
		unordered_map<const DSourceCombo*, unordered_map<const DReactionStepVertexInfo*, pair<DVector3, double>>> dVertexTimeOffsets; //double: time offset from the RF time //combo: primary at specific vertex

		//Only works if beam not needed! (e.g. not by missing mass)
		unordered_map<const DSourceCombo*, pair<DVector3, double>> dVertexTimeOffsetsByCombo; //combo: any at specific vertex, double: time offset from the RF time
};

inline void DSourceComboVertexer::Reset(void)
{
	dVertexTimeOffsets.clear();
	dVertexTimeOffsetsByCombo.clear();
}

inline size_t DSourceComboVertexer::Get_PhotonVertexZBin(double locVertexZ) const
{
	//given some vertex-z, what bin am I in?
	int locPhotonVertexZBin = int((locVertexZ - dPhotonVertexZRangeLow)/dPhotonVertexZBinWidth);
	if(locPhotonVertexZBin < 0)
		return 0;
	else if(locPhotonVertexZBin >= dNumPhotonVertexZBins)
		return dNumPhotonVertexZBins - 1;
	return locPhotonVertexZBin;
}

inline double DSourceComboVertexer::Get_PhotonVertexZBinCenter(signed char locVertexZBin) const
{
	return dPhotonVertexZRangeLow + (double(locVertexZBin) + 0.5)*dPhotonVertexZBinWidth;
}


inline vector<signed char> DSourceComboVertexer::Get_VertexZBins(const DReactionVertexInfo* locVertexInfo, const DSourceCombo* locReactionChargedCombo) const
{
	vector<signed char> locVertexZBins;
	for(auto locVertexStepInfo : locVertexInfo->Get_StepVertexInfos())
		locVertexZBins.emplace_back(Get_VertexZBin(locReactionChargedCombo, locVertexStepInfo));
	return locVertexZBins;
}

inline void DSourceComboVertexer::Set_VertexTimeOffsets_ByCombo(const DSourceCombo* locVertexPrimaryCombo, const pair<DVector3, double>& locVertexTimeOffsetPair)
{
	auto locVertexCombos = DAnalysis::Get_SourceCombos_ThisVertex(locVertexPrimaryCombo);
	for(auto& locVertexCombo : locVertexCombos)
		dVertexTimeOffsetsByCombo.emplace(locVertexCombo, locVertexTimeOffsetPair);
}

inline vector<const DKinematicData*>::const_iterator DSourceComboVertexer::Get_ThetaNearest90Iterator(const vector<const DKinematicData*>& locParticles)
{
	auto Get_Nearer90Theta = [](const DKinematicData* lhs, const DKinematicData* rhs) -> bool
		{return fabs(lhs->momentum().Theta() - 0.5*TMath::Pi()) < fabs(rhs->momentum().Theta() - 0.5*TMath::Pi());};
	return std::max_element(locParticles.begin(), locParticles.end(), Get_Nearer90Theta);
}

} //end DAnalysis namespace

#endif // DSourceComboVertexer_h
