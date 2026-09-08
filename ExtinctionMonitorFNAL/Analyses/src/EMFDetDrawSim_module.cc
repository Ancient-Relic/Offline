// An EDAnalyzer template to print out a data collection
//
// Andrei Gaponenko, 2012

#ifndef ExtinctionMonitorFNAL_Analyses_EMFDetDrawSim_hh
#define ExtinctionMonitorFNAL_Analyses_EMFDetDrawSim_hh

#include "TDirectory.h"
#include "TH1.h"
#include "TTree.h"

#include "canvas/Utilities/InputTag.h"
#include "fhiclcpp/ParameterSet.h" 
#include "art/Framework/Core/EDAnalyzer.h"
#include "fhiclcpp/types/Atom.h" 
#include "fhiclcpp/types/OptionalSequence.h"
#include "art/Framework/Principal/Event.h"
#include "art/Framework/Principal/Run.h"
#include "art/Framework/Principal/Provenance.h"
#include "art_root_io/TFileService.h"

#include <iostream>

#include "fhiclcpp/ParameterSet.h"
#include "art/Framework/Core/EDAnalyzer.h"
#include "art/Framework/Principal/Event.h"

#include "Offline/MCDataProducts/inc/ExtMonFNALSimHit.hh"
#include "Offline/MCDataProducts/inc/ProcessCode.hh"
#include "Offline/MCDataProducts/inc/SimParticle.hh"

namespace mu2e {

  struct ExtSimHit {
          int RunID;
	  int SubRunID;
	  long long EventID;
      	  unsigned int planeId;
          unsigned int moduleId;
          double eTot;
	  double eIon;
	  double startX;
	  double startY;
	  double startZ;
	  double startT;
	  double endX;
	  double endY;
	  double endZ;
	  double endT;
          unsigned int particleId;
	int pdgId;
	int parentId;
	int creationCode;
	int stoppingCode;


    ExtSimHit() :  RunID(-1), SubRunID(-1), EventID(-1),
	           planeId(-1), moduleId(-1),
		   eTot(0),eIon(0),
                   startX(0), startY(0), startZ(0), startT(0),
		   endX(0), endY(0), endZ(0), endT(0),
		   particleId(-1),pdgId(-1),parentId(-1),creationCode(-1),stoppingCode(-1)
                {}

ExtSimHit(int runID, int subrunID, long long eventID, const ExtMonFNALSimHit& hit) :
  RunID(runID), SubRunID(subrunID), EventID(eventID),
  planeId(hit.moduleId().plane()), moduleId(hit.moduleId().number()),
  eTot(hit.totalEnergyDeposit()), eIon(hit.ionizingEnergyDeposit()),
  startX(hit.localStartPosition().x()),
  startY(hit.localStartPosition().y()),
  startZ(hit.localStartPosition().z()),
  startT(hit.startTime()),
  endX(hit.localEndPosition().x()),
  endY(hit.localEndPosition().y()),
  endZ(hit.localEndPosition().z()),
  endT(hit.endTime()),
  particleId(hit.simParticle() ? hit.simParticle()->id().asUint() : 0u),
  pdgId(hit.simParticle()->pdgId()),
  parentId(hit.simParticle()->hasParent()
         ? hit.simParticle()->parent()->id().asInt() : -1),
  creationCode(-1),
  stoppingCode(-1)
{
  auto const sp = hit.simParticle();
  if (sp) {
    creationCode = static_cast<int>(sp->creationCode().id());
    stoppingCode = static_cast<int>(sp->stoppingCode().id());
  }
}

  }; // struct ExtSimHit


//================================================================
  class EMFDetDrawSim : public art::EDAnalyzer {
  protected:
    std::string _inModuleLabel;
    std::string _inInstanceName;


    TTree *nt_;
    ExtSimHit hit_;

  public:
    explicit EMFDetDrawSim(const fhicl::ParameterSet& pset);
    virtual void beginJob();    
    virtual void analyze(const art::Event& event);
  };

  //================================================================
  EMFDetDrawSim::EMFDetDrawSim(const fhicl::ParameterSet& pset)
    : art::EDAnalyzer(pset)
    , _inModuleLabel(pset.get<std::string>("inputModuleLabel"))
    , _inInstanceName(pset.get<std::string>("inputInstanceName"))
    , nt_(0)
    
  {}


  void EMFDetDrawSim::beginJob() {

    art::ServiceHandle<art::TFileService> tfs;
    static const char branchDesc[] = "RunID/I:SubRunID/I:EventID/L:planeId/i:moduleId/i:eTot/D:eIon/D:startX/D:startY/D:startZ/D:startT/D:endX/D:endY/D:endZ/D:endT/D:particleId/i:pdgId/I:parentId/I:creationCode/I:stoppingCode/I";
    nt_ = tfs->make<TTree>( "nt", "ExtSimHits ntuple");
    nt_->Branch("hits", &hit_, branchDesc);	  

  }	  

  //================================================================
  void EMFDetDrawSim::analyze(const art::Event& event) {

    art::Handle<ExtMonFNALSimHitCollection> ih;
    event.getByLabel(_inModuleLabel, _inInstanceName, ih);

    const ExtMonFNALSimHitCollection& inputs(*ih);


    for(typename ExtMonFNALSimHitCollection::const_iterator i=inputs.begin(); i!=inputs.end(); ++i) {

      //std::cout<<"run "<<event.run()<<", subrun "<<event.subRun()<<", event "<<event.event()
      //        <<", plane : "<<i->moduleId().plane()<<", module : "<<i->moduleId().number()
      //        <<", particle : "<<i->simParticle()->id().asUint()<<", eTot : "<<i->totalEnergyDeposit()<<", eIon : "<<i->ionizingEnergyDeposit()
      //        <<", start : ("<<i->localStartPosition().x()<<", "<<i->localStartPosition().y()<<", "<<i->localStartPosition().z()<<", t = "<<i->startTime()<<") "
      //        <<", end : ("<<i->localEndPosition().x()<<", "<<i->localEndPosition().y()<<", "<<i->localEndPosition().z()<<", t = "<<i->endTime()<<") "	      
      //	<<std::endl;
	    
      hit_ = ExtSimHit(event.run(), event.subRun(), event.event(), *i);
      nt_->Fill();
    }
  }

  //================================================================
} // namespace mu2e

DEFINE_ART_MODULE(mu2e::EMFDetDrawSim)

#endif/*ExtinctionMonitorFNAL_Analyses_EMFDetDrawSim_hh*/
