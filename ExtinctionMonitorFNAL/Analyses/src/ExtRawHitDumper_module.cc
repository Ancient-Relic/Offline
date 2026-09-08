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
#include "Offline/RecoDataProducts/inc/ExtMonFNALRawHit.hh"
#include "Offline/MCDataProducts/inc/ExtMonFNALHitTruthAssn.hh"
#include "Offline/MCDataProducts/inc/SimParticle.hh"
#include "canvas/Persistency/Common/FindMany.h"

namespace mu2e {

  //================================================================

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
	    double truthCharge;
          int truthParticleId;
          int truthPdgId;
          int truthParentId;
    ExtRawHit() :  RunID(-1), SubRunID(-1), EventID(-1),
                   planeId(-1), moduleId(-1),
                   chipCol(-1), chipRow(-1),
                   Col(-1), Row(-1),
                   clock(-1), tot(-1),
		   truthCharge(0), truthParticleId(-1), truthPdgId(0), truthParentId(-1)
                {}

    ExtRawHit(int runID, int subrunID, long long eventID, const ExtMonFNALRawHit& hit) :
            RunID(runID), SubRunID(subrunID),EventID(eventID),
            planeId(hit.pixelId().chip().module().plane()), moduleId(hit.pixelId().chip().module().number()),
            chipCol(hit.pixelId().chip().chipCol()), chipRow(hit.pixelId().chip().chipRow()),
            Col(hit.pixelId().col()), Row(hit.pixelId().row()),
            clock(hit.clock()), tot(hit.tot()),
            truthCharge(0), truthParticleId(-1), truthPdgId(0), truthParentId(-1)
                {}

  }; // struct ExtRawHit

  //================================================================
  class ExtRawHitDumper : public art::EDAnalyzer {
    struct Config {
      using Name=fhicl::Name;
      using Comment=fhicl::Comment;
      fhicl::Atom<std::string> hits     {Name("hitsInputTag"     ), Comment("MC collection")};
      fhicl::Atom<std::string> truth {Name("truthInputTag"), Comment("Hit truth assn")};
    };

    typedef art::EDAnalyzer::Table<Config> Parameters;

  protected:

    art::InputTag hitsInputTag_;
    art::InputTag truthInputTag_;
    TTree *nt_;
    ExtRawHit hit_;

    public:
    explicit ExtRawHitDumper(const Parameters& pset);
    virtual void beginJob();
    virtual void analyze(const art::Event& event);
  };

  //================================================================
  ExtRawHitDumper::ExtRawHitDumper(const Parameters& pset)
    : art::EDAnalyzer(pset)
      , hitsInputTag_(pset().hits())
      , truthInputTag_(pset().truth())
      , nt_(0)
  {

  }

  //================================================================
  void ExtRawHitDumper::beginJob() {
    art::ServiceHandle<art::TFileService> tfs;
    static const char branchDesc[] =
      "RunID/I:SubRunID/I:EventID/L:planeId/i:moduleId/i"
      ":chipCol/i:chipRow/i:Col/i:Row/i:clock/I:tot/i"
      ":truthCharge/D"
      ":truthParticleId/I:truthPdgId/I:truthParentId/I";
    nt_ = tfs->make<TTree>( "nt", "ExtRawHits ntuple");
    nt_->Branch("hits", &hit_, branchDesc);
  }

  //================================================================
void ExtRawHitDumper::analyze(const art::Event& event) {
    const auto& ih = event.getValidHandle<ExtMonFNALRawHitCollection>(hitsInputTag_);
    art::FindMany<SimParticle, ExtMonFNALHitTruthBits>
        truthFinder(ih, event, truthInputTag_);

    for (size_t i = 0; i < ih->size(); ++i) {
        hit_ = ExtRawHit(event.run(), event.subRun(), event.event(), (*ih)[i]);
        hit_.truthParticleId = -1;
        hit_.truthPdgId = 0;
        hit_.truthParentId = -1;
        hit_.truthCharge = 0;

        if (truthFinder.isValid()) {
            std::vector<const SimParticle*> particles;
            std::vector<const ExtMonFNALHitTruthBits*> bits;
            truthFinder.get(i, particles, bits);
            double maxCharge = 0;
            for (size_t j = 0; j < particles.size(); ++j) {
                double q = bits[j]->charge();
                if (q > maxCharge) {
                    maxCharge = q;
                    hit_.truthParticleId = particles[j]->id().asInt();
                    hit_.truthPdgId = particles[j]->pdgId();
                    hit_.truthParentId = particles[j]->hasParent()
                        ? particles[j]->parent()->id().asInt() : -1;
                    hit_.truthCharge = q;
                }
            }
        }
        nt_->Fill();
    }
}

  //================================================================

} // namespace mu2e

DEFINE_ART_MODULE(mu2e::ExtRawHitDumper)
