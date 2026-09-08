// An EDAnalyzer template to print out a data collection
//
// Andrei Gaponenko, 2012

#ifndef ExtinctionMonitorFNAL_Analyses_EMFDetDrawRaw_hh
#define ExtinctionMonitorFNAL_Analyses_EMFDetDrawRaw_hh

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

#include "Offline/RecoDataProducts/inc/ExtMonFNALRawHit.hh"


namespace mu2e {

  struct ExtRawHit {
          
          int RunID;
	  int SubRunID;
	  long long EventID;
          unsigned int planeId;
          unsigned int moduleId;
          unsigned int chipCol;
	  unsigned int chipRow;
	  unsigned int Col;
	  unsigned int Row;
          int clock;
	  int tot;

    ExtRawHit() :  RunID(-1), SubRunID(-1), EventID(-1),
                   planeId(-1), moduleId(-1),
		   chipCol(-1), chipRow(-1),
		   Col(-1), Row(-1),
		   clock(-1), tot(-1)
                {}

    ExtRawHit(int runID, int subrunID, long long eventID, const ExtMonFNALRawHit& hit) :
            RunID(runID), SubRunID(subrunID),EventID(eventID),
            planeId(hit.pixelId().chip().module().plane()), moduleId(hit.pixelId().chip().module().number()),
	    chipCol(hit.pixelId().chip().chipCol()), chipRow(hit.pixelId().chip().chipRow()),
	    Col(hit.pixelId().col()), Row(hit.pixelId().row()),
	    clock(hit.clock()), tot(hit.tot())
                {}


  }; // struct ExtRawHit


//================================================================
  class EMFDetDrawRaw : public art::EDAnalyzer {
  protected:
    std::string _inModuleLabel;
    std::string _inInstanceName;


    TTree *nt_;
    ExtRawHit hit_;

  public:
    explicit EMFDetDrawRaw(const fhicl::ParameterSet& pset);
    virtual void beginJob();    
    virtual void analyze(const art::Event& event);
  };

  //================================================================
  EMFDetDrawRaw::EMFDetDrawRaw(const fhicl::ParameterSet& pset)
    : art::EDAnalyzer(pset)
    , _inModuleLabel(pset.get<std::string>("inputModuleLabel"))
    , _inInstanceName(pset.get<std::string>("inputInstanceName"))
    , nt_(0)
  {}


  void EMFDetDrawRaw::beginJob() {

    art::ServiceHandle<art::TFileService> tfs;
    static const char branchDesc[] = "RunID/I:SubRunID/I:EventID/L:planeId/i:moduleId/i:chipCol/i:chipRow/i:Col/i:Row/i:clock/I:tot/i"; 
    nt_ = tfs->make<TTree>( "nt", "ExtRawHits ntuple");
    nt_->Branch("hits", &hit_, branchDesc);	  

  }	  

  //================================================================
  void EMFDetDrawRaw::analyze(const art::Event& event) {

    art::Handle<ExtMonFNALRawHitCollection> ih;
    event.getByLabel(_inModuleLabel, _inInstanceName, ih);

    const ExtMonFNALRawHitCollection& inputs(*ih);

    std::cout<<"EMFDetDrawRaw: inModuleLabel = "<<_inModuleLabel
             <<", inInstanceName = "<<_inInstanceName<<std::endl;

    for(typename ExtMonFNALRawHitCollection::const_iterator i=inputs.begin(); i!=inputs.end(); ++i) {

     // std::cout<<"run "<<event.run()<<", subrun "<<event.subRun()<<", event "<<event.event()
     //	      <<", plane : "<<i->pixelId().chip().module().plane()<<", module : "<<i->pixelId().chip().module().number()
     //       <<", chip : ("<<i->pixelId().chip().chipCol()<<", "<<i->pixelId().chip().chipRow()
     //	      <<", "<<i->pixelId().col()<<", "<<i->pixelId().row()<<") "
     //       <<"clock = "<<i->clock()<<",  "<<"tot = "<<i->tot()<<std::endl;

      hit_ = ExtRawHit(event.run(), event.subRun(), event.event(), *i);

      nt_->Fill();
    }
  }

  //================================================================
} // namespace mu2e

DEFINE_ART_MODULE(mu2e::EMFDetDrawRaw)

#endif/*ExtinctionMonitorFNAL_Analyses_EMFDetDrawRaw_hh*/
