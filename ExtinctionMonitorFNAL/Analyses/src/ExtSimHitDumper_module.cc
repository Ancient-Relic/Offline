// Ntuple dumper for MCs.
//
// Andrei Gaponenko, 2013

#include <string>
#include <vector>
#include <limits>
#include <cmath>

#include "cetlib_except/exception.h"
#include "CLHEP/Vector/ThreeVector.h"

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

#include "Offline/GlobalConstantsService/inc/GlobalConstantsHandle.hh"
#include "Offline/GlobalConstantsService/inc/ParticleDataList.hh"
#include "Offline/Mu2eUtilities/inc/SimParticleGetTau.hh"
#include "Offline/GeometryService/inc/GeomHandle.hh"
#include "Offline/GeometryService/inc/DetectorSystem.hh"

#include "KinKal/General/ParticleState.hh"
#include "Offline/MCDataProducts/inc/ExtMonFNALSimHit.hh"

namespace mu2e {

  //================================================================
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
    double startPmag;

    unsigned int particleId;
    int parentId;
    int pdgId;
    int creationCode;
    int stoppingCode;

    ExtSimHit()
      : RunID(-1)
      , SubRunID(-1)
      , EventID(-1)
      , planeId(0u)
      , moduleId(0u)
      , eTot(0)
      , eIon(0)
      , startX(0)
      , startY(0)
      , startZ(0)
      , startT(0)
      , endX(0)
      , endY(0)
      , endZ(0)
      , endT(0)
      , startPmag(0)
      , particleId(-1)
      , parentId(-1), pdgId(0), creationCode(-1), stoppingCode(-1)
    {}

    ExtSimHit(int runID, int subrunID, long long eventID, const ExtMonFNALSimHit& hit)
      : RunID(runID)
      , SubRunID(subrunID)
      , EventID(eventID)
      , planeId(hit.moduleId().plane())
      , moduleId(hit.moduleId().number())
      , eTot(hit.totalEnergyDeposit())
      , eIon(hit.ionizingEnergyDeposit())
      , startX(hit.localStartPosition().x())
      , startY(hit.localStartPosition().y())
      , startZ(hit.localStartPosition().z())
      , startT(hit.startTime())
      , endX(hit.localEndPosition().x())
      , endY(hit.localEndPosition().y())
      , endZ(hit.localEndPosition().z())
      , endT(hit.endTime())
      , startPmag(hit.simParticle()->startMomentum().mag())
      , particleId(hit.simParticle() ? hit.simParticle()->id().asUint() : -1)
      , parentId(hit.simParticle()->hasParent()
		? hit.simParticle()->parent()->id().asInt() : -1),
	pdgId(hit.simParticle()->pdgId()),
	creationCode(hit.simParticle()->creationCode().id()),
	stoppingCode(hit.simParticle()->stoppingCode().id())
    {}

  }; // struct ExtSimHit

  //================================================================
  class ExtSimHitDumper : public art::EDAnalyzer {
    struct Config {
      using Name=fhicl::Name;
      using Comment=fhicl::Comment;
      fhicl::Atom<std::string> hits     {Name("hitsInputTag"     ), Comment("MC collection")};
    };

    typedef art::EDAnalyzer::Table<Config> Parameters;

  protected:

    art::InputTag hitsInputTag_;
    TTree *nt_;
    ExtSimHit hit_;

    public:
    explicit ExtSimHitDumper(const Parameters& pset);
    virtual void beginJob();
    virtual void analyze(const art::Event& event);
  };

  //================================================================
  ExtSimHitDumper::ExtSimHitDumper(const Parameters& pset)
    : art::EDAnalyzer(pset)
      , hitsInputTag_(pset().hits())
      , nt_(0)
  {

  }

  //================================================================
  void ExtSimHitDumper::beginJob() {
    art::ServiceHandle<art::TFileService> tfs;
static const char branchDesc[] =
      "RunID/I:SubRunID/I:EventID/L:planeId/i:moduleId/i"
      ":eTot/D:eIon/D"
      ":startX/D:startY/D:startZ/D:startT/D"
      ":endX/D:endY/D:endZ/D:endT/D"
      ":startPmag/D"
      ":particleId/i:parentId/I:pdgId/I"
      ":creationCode/I:stoppingCode/I";


    nt_ = tfs->make<TTree>( "nt", "ExtSimHits ntuple");
    nt_->Branch("hits", &hit_, branchDesc);
  }

  //================================================================

  void ExtSimHitDumper::analyze(const art::Event& event) {

    const auto& ih = event.getValidHandle<ExtMonFNALSimHitCollection>(hitsInputTag_);

    for(const auto& i : *ih) {
      hit_ = ExtSimHit(event.run(), event.subRun(), event.event(), i);
      nt_->Fill();
    }
  }

  //================================================================

} // namespace mu2e

DEFINE_ART_MODULE(mu2e::ExtSimHitDumper)
