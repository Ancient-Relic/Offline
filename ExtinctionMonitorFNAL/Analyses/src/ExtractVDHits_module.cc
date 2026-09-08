// ExtractVDHits_module.cc
//
// Art EDAnalyzer that reads StepPointMCCollection from virtual detectors
// and writes selected hit data into a flat ROOT TTree for offline analysis.
//
// Run:   mu2e -c extractVDHits.fcl -s your_file.art

#include "art/Framework/Core/EDAnalyzer.h"
#include "art/Framework/Core/ModuleMacros.h"
#include "art/Framework/Principal/Event.h"
#include "art/Framework/Principal/Handle.h"
#include "art/Framework/Services/Registry/ServiceHandle.h"
#include "art_root_io/TFileService.h"

#include "Offline/MCDataProducts/inc/StepPointMC.hh"
#include "Offline/MCDataProducts/inc/SimParticle.hh"
#include "Offline/MCDataProducts/inc/ProcessCode.hh"
#include "Offline/DataProducts/inc/VirtualDetectorId.hh"

#include "TTree.h"

#include <cmath>
#include <set>
#include <string>
#include <iostream>

namespace mu2e {

  class ExtractVDHits : public art::EDAnalyzer {
  public:
    struct Config {
      using Name    = fhicl::Name;
      using Comment = fhicl::Comment;

      fhicl::Atom<art::InputTag> vdStepPoints{
        Name("vdStepPoints"),
        Comment("Input tag for virtual detector StepPointMCCollection"),
        art::InputTag("g4run", "virtualdetector")
      };

      fhicl::Sequence<int> vdIds{
        Name("vdIds"),
        Comment("List of VirtualDetector IDs to extract"),
        std::vector<int>{35, 36, 38, 39, 117, 118}
      };

      fhicl::Sequence<std::string> scintVolumes{
        Name("scintVolumes"),
        Comment("List of scintillator sensitive volume names to read"),
        std::vector<std::string>{}
      };
    };

    using Parameters = art::EDAnalyzer::Table<Config>;
    explicit ExtractVDHits(const Parameters& conf);

    void analyze(const art::Event& event) override;
    void beginJob() override;
    void endJob()   override;

  private:
    art::InputTag  vdTag_;
    std::set<int>  vdIds_;
    std::vector<std::string> scintVolumes_;

    TTree* tree_;

    // VD tree branches
    Int_t   run_, subrun_, event_;
    Int_t   vdId_;
    Int_t   trackId_;
    Int_t   pdgId_;
    Float_t x_,  y_,  z_;
    Float_t postx_, posty_, postz_;
    Float_t px_, py_, pz_, ptot_;
    Float_t postpx_, postpy_, postpz_, postptot_;
    Float_t time_;
    Float_t totalEdep_;
    Float_t nonIonEdep_;
    Float_t stepLength_;
    Float_t theta_, phi_;
    Int_t   endProcessCode_;

    // SimParticle information
    Int_t   creationCode_;
    Int_t   stoppingCode_;
    Int_t   parentId_;
    Int_t   parentPdgId_;
    Float_t startx_, starty_, startz_;
    Float_t endx_, endy_, endz_;

    // --- Scintillator hits TTree ---
    TTree* scintTree_;

    // Scintillator tree branches (per-step)
    Int_t   sc_run_, sc_subrun_, sc_event_;
    Int_t   sc_volIdx_;      // index into scintVolumes list
    Int_t   sc_trackId_;
    Int_t   sc_pdgId_;
    Float_t sc_x_, sc_y_, sc_z_;
    Float_t sc_px_, sc_py_, sc_pz_, sc_ptot_;
    Float_t sc_time_;
    Float_t sc_totalEdep_;
    Float_t sc_nonIonEdep_;
    Float_t sc_stepLength_;
    Int_t   sc_creationCode_;
    Int_t   sc_stoppingCode_;
    Int_t   sc_parentId_;
    Int_t   sc_parentPdgId_;

    // Counters
    long totalEvents_;
    long totalHits_;
    std::map<int, long> hitCounts_;
    std::map<std::string, long> scintHitCounts_;
  };

  // -------------------------------------------------------
  ExtractVDHits::ExtractVDHits(const Parameters& conf)
    : art::EDAnalyzer(conf)
    , vdTag_(conf().vdStepPoints())
    , scintVolumes_(conf().scintVolumes())
    , totalEvents_(0)
    , totalHits_(0)
  {
    auto ids = conf().vdIds();
    vdIds_.insert(ids.begin(), ids.end());
  }

  // -------------------------------------------------------
  void ExtractVDHits::beginJob() {
    art::ServiceHandle<art::TFileService> tfs;
    tree_ = tfs->make<TTree>("vdHits", "Extracted Virtual Detector Hits");

    tree_->Branch("run",            &run_,            "run/I");
    tree_->Branch("subrun",         &subrun_,         "subrun/I");
    tree_->Branch("event",          &event_,          "event/I");
    tree_->Branch("vdId",           &vdId_,           "vdId/I");
    tree_->Branch("trackId",        &trackId_,        "trackId/I");
    tree_->Branch("pdgId",          &pdgId_,          "pdgId/I");
    tree_->Branch("x",              &x_,              "x/F");
    tree_->Branch("y",              &y_,              "y/F");
    tree_->Branch("z",              &z_,              "z/F");
    tree_->Branch("postx",          &postx_,          "postx/F");
    tree_->Branch("posty",          &posty_,          "posty/F");
    tree_->Branch("postz",          &postz_,          "postz/F");
    tree_->Branch("px",             &px_,             "px/F");
    tree_->Branch("py",             &py_,             "py/F");
    tree_->Branch("pz",             &pz_,             "pz/F");
    tree_->Branch("ptot",           &ptot_,           "ptot/F");
    tree_->Branch("postpx",         &postpx_,         "postpx/F");
    tree_->Branch("postpy",         &postpy_,         "postpy/F");
    tree_->Branch("postpz",         &postpz_,         "postpz/F");
    tree_->Branch("postptot",       &postptot_,       "postptot/F");
    tree_->Branch("time",           &time_,           "time/F");
    tree_->Branch("totalEdep",      &totalEdep_,      "totalEdep/F");
    tree_->Branch("nonIonEdep",     &nonIonEdep_,     "nonIonEdep/F");
    tree_->Branch("stepLength",     &stepLength_,     "stepLength/F");
    tree_->Branch("theta",          &theta_,          "theta/F");
    tree_->Branch("phi",            &phi_,            "phi/F");
    tree_->Branch("endProcessCode", &endProcessCode_, "endProcessCode/I");

    // SimParticle branches
    tree_->Branch("creationCode",  &creationCode_,  "creationCode/I");
    tree_->Branch("stoppingCode",  &stoppingCode_,  "stoppingCode/I");
    tree_->Branch("parentId",      &parentId_,      "parentId/I");
    tree_->Branch("parentPdgId",   &parentPdgId_,   "parentPdgId/I");
    tree_->Branch("startx",        &startx_,        "startx/F");
    tree_->Branch("starty",        &starty_,        "starty/F");
    tree_->Branch("startz",        &startz_,        "startz/F");
    tree_->Branch("endx",          &endx_,          "endx/F");
    tree_->Branch("endy",          &endy_,          "endy/F");
    tree_->Branch("endz",          &endz_,          "endz/F");

    // --- Scintillator hits TTree ---
    if (!scintVolumes_.empty()) {
      scintTree_ = tfs->make<TTree>("scintHits", "Scintillator Step Hits");

      scintTree_->Branch("run",           &sc_run_,          "run/I");
      scintTree_->Branch("subrun",        &sc_subrun_,       "subrun/I");
      scintTree_->Branch("event",         &sc_event_,        "event/I");
      scintTree_->Branch("volIdx",        &sc_volIdx_,       "volIdx/I");
      scintTree_->Branch("trackId",       &sc_trackId_,      "trackId/I");
      scintTree_->Branch("pdgId",         &sc_pdgId_,        "pdgId/I");
      scintTree_->Branch("x",             &sc_x_,            "x/F");
      scintTree_->Branch("y",             &sc_y_,            "y/F");
      scintTree_->Branch("z",             &sc_z_,            "z/F");
      scintTree_->Branch("px",            &sc_px_,           "px/F");
      scintTree_->Branch("py",            &sc_py_,           "py/F");
      scintTree_->Branch("pz",            &sc_pz_,           "pz/F");
      scintTree_->Branch("ptot",          &sc_ptot_,         "ptot/F");
      scintTree_->Branch("time",          &sc_time_,         "time/F");
      scintTree_->Branch("totalEdep",     &sc_totalEdep_,    "totalEdep/F");
      scintTree_->Branch("nonIonEdep",    &sc_nonIonEdep_,   "nonIonEdep/F");
      scintTree_->Branch("stepLength",    &sc_stepLength_,   "stepLength/F");
      scintTree_->Branch("creationCode",  &sc_creationCode_, "creationCode/I");
      scintTree_->Branch("stoppingCode",  &sc_stoppingCode_, "stoppingCode/I");
      scintTree_->Branch("parentId",      &sc_parentId_,     "parentId/I");
      scintTree_->Branch("parentPdgId",   &sc_parentPdgId_,  "parentPdgId/I");

      std::cout << "ExtractVDHits: will read " << scintVolumes_.size()
                << " scintillator volumes" << std::endl;
      for (size_t i = 0; i < scintVolumes_.size(); i++) {
        std::cout << "  volIdx " << i << ": " << scintVolumes_[i] << std::endl;
      }
    }
  }

  // -------------------------------------------------------
  void ExtractVDHits::analyze(const art::Event& event) {
    totalEvents_++;

    run_    = event.run();
    subrun_ = event.subRun();
    event_  = event.event();

    auto vdHitsH = event.getValidHandle<StepPointMCCollection>(vdTag_);
    const auto& vdHits = *vdHitsH;

    for (const auto& hit : vdHits) {
      int vid = hit.volumeId();

      // Skip VDs we don't care about
      if (vdIds_.find(vid) == vdIds_.end()) continue;

      hitCounts_[vid]++;
      totalHits_++;

      vdId_    = vid;
      trackId_ = hit.trackId().asInt();

      // PDG ID from SimParticle
      if (hit.simParticle().isNonnull()) {
        const auto& sim = *hit.simParticle();
        pdgId_ = sim.pdgId();

        // Creation and stopping codes
        creationCode_ = sim.creationCode().id();
        stoppingCode_ = sim.stoppingCode().id();

        // Creation position
        startx_ = sim.startPosition().x();
        starty_ = sim.startPosition().y();
        startz_ = sim.startPosition().z();

        // Stopping position
        endx_ = sim.endPosition().x();
        endy_ = sim.endPosition().y();
        endz_ = sim.endPosition().z();

        // Parent info
        if (sim.parent().isNonnull()) {
          parentId_    = sim.parent()->id().asInt();
          parentPdgId_ = sim.parent()->pdgId();
        } else {
          parentId_    = -1;  // no parent (primary)
          parentPdgId_ = 0;
        }
      } else {
        pdgId_        = 0;
        creationCode_ = -1;
        stoppingCode_ = -1;
        parentId_     = -1;
        parentPdgId_  = 0;
        startx_ = starty_ = startz_ = 0;
        endx_ = endy_ = endz_ = 0;
      }

      // Pre-step position
      x_ = hit.position().x();
      y_ = hit.position().y();
      z_ = hit.position().z();

      // Post-step position
      postx_ = hit.postPosition().x();
      posty_ = hit.postPosition().y();
      postz_ = hit.postPosition().z();

      // Pre-step momentum
      px_   = hit.momentum().x();
      py_   = hit.momentum().y();
      pz_   = hit.momentum().z();
      ptot_ = hit.momentum().mag();

      // Post-step momentum
      postpx_   = hit.postMomentum().x();
      postpy_   = hit.postMomentum().y();
      postpz_   = hit.postMomentum().z();
      postptot_ = hit.postMomentum().mag();

      // Scalars
      time_       = hit.time();
      totalEdep_  = hit.totalEDep();
      nonIonEdep_ = hit.nonIonizingEDep();
      stepLength_ = hit.stepLength();
      endProcessCode_ = hit.endProcessCode();

      // Derived angles from pre-step momentum
      theta_ = (ptot_ > 0) ? std::acos(std::clamp((double)(pz_ / ptot_), -1.0, 1.0)) : 0;
      phi_   = std::atan2(py_, px_);

      tree_->Fill();
    }

    // --- Read scintillator sensitive volume collections ---
    for (size_t iVol = 0; iVol < scintVolumes_.size(); iVol++) {
      art::InputTag scintTag("g4run", scintVolumes_[iVol]);
      art::Handle<StepPointMCCollection> scintH;
      event.getByLabel(scintTag, scintH);

      if (!scintH.isValid()) continue;

      const auto& scintHits = *scintH;
      scintHitCounts_[scintVolumes_[iVol]] += scintHits.size();

      for (const auto& hit : scintHits) {
        sc_run_    = event.run();
        sc_subrun_ = event.subRun();
        sc_event_  = event.event();
        sc_volIdx_ = iVol;
        sc_trackId_ = hit.trackId().asInt();

        if (hit.simParticle().isNonnull()) {
          const auto& sim = *hit.simParticle();
          sc_pdgId_        = sim.pdgId();
          sc_creationCode_ = sim.creationCode().id();
          sc_stoppingCode_ = sim.stoppingCode().id();
          if (sim.parent().isNonnull()) {
            sc_parentId_    = sim.parent()->id().asInt();
            sc_parentPdgId_ = sim.parent()->pdgId();
          } else {
            sc_parentId_    = -1;
            sc_parentPdgId_ = 0;
          }
        } else {
          sc_pdgId_ = 0;
          sc_creationCode_ = -1;
          sc_stoppingCode_ = -1;
          sc_parentId_ = -1;
          sc_parentPdgId_ = 0;
        }

        sc_x_ = hit.position().x();
        sc_y_ = hit.position().y();
        sc_z_ = hit.position().z();

        sc_px_   = hit.momentum().x();
        sc_py_   = hit.momentum().y();
        sc_pz_   = hit.momentum().z();
        sc_ptot_ = hit.momentum().mag();

        sc_time_       = hit.time();
        sc_totalEdep_  = hit.totalEDep();
        sc_nonIonEdep_ = hit.nonIonizingEDep();
        sc_stepLength_ = hit.stepLength();

        scintTree_->Fill();
      }
    }
  }

  // -------------------------------------------------------
  void ExtractVDHits::endJob() {
    std::cout << "\n=========================================" << std::endl;
    std::cout << "  ExtractVDHits Summary" << std::endl;
    std::cout << "=========================================" << std::endl;
    std::cout << "  Events processed: " << totalEvents_ << std::endl;
    std::cout << "  Hits extracted:   " << totalHits_ << std::endl;
    std::cout << std::endl;
    std::cout << "  VD ID      Hits  Name" << std::endl;
    std::cout << "  ---------------------------------" << std::endl;

    std::map<int, std::string> vdNames = {
      {35,  "EMFDetectorUpEntrance"},
      {36,  "EMFDetectorUpExit"},
      {38,  "EMFDetectorDnEntrance"},
      {39,  "EMFDetectorDnExit"},
      {117, "EMFDetectorUp_Scint"},
      {118, "EMFC2Exit_Large"},
    };

    for (const auto& [vid, count] : hitCounts_) {
      std::string name = vdNames.count(vid) ? vdNames[vid] : "???";
      std::cout << "  " << std::setw(5) << vid
                << std::setw(10) << count
                << "  " << name << std::endl;
    }

    if (!scintHitCounts_.empty()) {
      std::cout << std::endl;
      std::cout << "  Scintillator Step Hits" << std::endl;
      std::cout << "  ---------------------------------" << std::endl;
      long totalScintHits = 0;
      for (const auto& [name, count] : scintHitCounts_) {
        std::cout << "  " << std::setw(25) << name
                  << std::setw(10) << count << std::endl;
        totalScintHits += count;
      }
      std::cout << "  " << std::setw(25) << "TOTAL"
                << std::setw(10) << totalScintHits << std::endl;
    }

    std::cout << "=========================================" << std::endl;
  }

} // namespace mu2e

DEFINE_ART_MODULE(mu2e::ExtractVDHits)
