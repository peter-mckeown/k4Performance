#include "DiPhotonAnalysis.hpp"

#include <Eigen/Dense>
#include <cmath>
#include <vector>
#include <algorithm>


#include "k4FWCore/Consumer.h"

#include "GaudiKernel/ITHistSvc.h"
#include "GaudiKernel/SmartIF.h"

// Testing
#include <Gaudi/Functional/Producer.h>

#include "TH1F.h"
#include "TH2F.h"

#include "edm4hep/ReconstructedParticleCollection.h"
#include "edm4hep/MCParticleCollection.h"
#include "edm4hep/ClusterCollection.h"
#include "edm4hep/RecoMCParticleLinkCollection.h"

// For linking and navigation
#include "podio/ObjectID.h"
#include "podio/LinkNavigator.h"

#include "DDRec/Vector3D.h" //Remove DD4hep dependency?

using dd4hep::rec::Vector3D;

/*
struct ObjectIDLess {
    bool operator()(const podio::ObjectID& a, const podio::ObjectID& b) const noexcept {
      return (a.collectionID < b.collectionID) ||
             (a.collectionID == b.collectionID && a.index < b.index);
    }
  };
*/

// A structure to hold photon PFO information, and pointer to the original pfo
struct PhotonPFO {
  double energy;
  Vector3D position;
  const edm4hep::ReconstructedParticle* pfo;
};

/*
struct DiPhotonAnalysis : Gaudi::Functional::Producer<int()> {

  DiPhotonAnalysis(const std::string& name, ISvcLocator* svcLoc)
    : Producer(name, svcLoc,
                KeyValue{"OutputLocation", "DummyInt"}) {}
    
      int operator()() const override {
    debug() << "RUNNING" << endmsg;
    return StatusCode::SUCCESS;
  }
};
*/
    

// Core DiPhotonAnalysis algo
struct DiPhotonAnalysis final
    : k4FWCore::Consumer<void(const edm4hep::ReconstructedParticleCollection&,
                        const edm4hep::MCParticleCollection&,
                        const edm4hep::ClusterCollection&,
                        const edm4hep::RecoMCParticleLinkCollection&)> {

    DiPhotonAnalysis(const std::string& name, ISvcLocator* svcLoc)
    : Consumer(name, svcLoc, {
        KeyValues("InputPFOs",          {"PandoraPFOs"}),
        KeyValues("InputMCParticles",   {"MCParticles"}),
        KeyValues("InputClusters",    {"PandoraClusters"}),
        KeyValues("InputRecoMC",      {"RecoMCTruthLink"})
    }) {}


    /// This part is all for plotting
    // Services & booking flag
    /// Move to header?
    mutable SmartIF<ITHistSvc> m_histSvc;     
    mutable TH1F *h_nPFO_photons=nullptr, *h_nPFO=nullptr, *h_Frag_Energy=nullptr;
    Gaudi::Property<std::string> m_histPath{this,"HistPath","/PLOTS",
      "THistSvc directory (must include stream, e.g. '/PLOTS/...')."};
    Gaudi::Property<int> m_bins_sep {this, "SeparationBins", 18, "True photon separation [mm]"};
    Gaudi::Property<double> m_min_sep {this, "MinSep", 0.0, "Minimum separation [mm]"};
    Gaudi::Property<double> m_max_sep {this, "MaxSep", 90.0, "Maximum separation [mm]"};
    bool ok_nPFO_photon=true;
    bool ok_nPFO=true;
    bool ok_Frag_Energy=true;

   
  //  std::map createMCPFOMaps(const edm4hep::MCParticleCollection& mcpColl, 
  //                          const edm4hep::ReconstructedParticleCollection& pfoColl,
  //                          const edm4hep::RecoMCParticleLinkCollection& linkColl){
  //                              // Transcribed from original LCIO, following Anna's implementation: https://github.com/Zehvogel/k4Performance/blob/main/PFlowValidation/components/PFOtoMCviaClusterLink.cpp
  //                              
  //                              // Index MC Particle by Object ID
  //                              std::map<podio::ObjectID, size_t, ObjectIDLess> mcIndex;
  //                              for (size_t i=0; i<mcpColl.size(); ++i) mcIndex[ mcpColl[i].getObjectID() ] = i;
  //
  //                             // Collect primaries in vector
  //                            std::vector<>
  //   }


    /// Initialize histograms etc
    StatusCode initialize() override{

      if (Gaudi::Algorithm::initialize().isFailure()){
        return StatusCode::FAILURE;
      }

      m_histSvc = service("THistSvc", true);
      if (!m_histSvc){
          error() << "Could not get THistSvc!"<< endmsg;
          return StatusCode::FAILURE;
      }

      // Register histograms
    auto reg = [&](TH1* h){
      const std::string full = m_histPath.value() + "/" + h->GetName();
      return m_histSvc->regHist(full, h).isSuccess();
    };

    h_nPFO_photons = new TH1F("Reco_photons_sep","Avg. No. Photon PFOs;d_{#gamma#gamma sep}[mm];Events", m_bins_sep, m_min_sep, m_max_sep);
    ok_nPFO_photon&=reg(h_nPFO_photons);
    if(!ok_nPFO_photon) {
      error() << "Failed to register nPFO_photons histogram" <<endmsg;
      return StatusCode::FAILURE;
    }
    h_nPFO = new TH1F("Reco_sep","Avg. No. PFOs;d_{#gamma#gamma sep}[mm];Events", m_bins_sep, m_min_sep, m_max_sep);
    ok_nPFO&=reg(h_nPFO);
    if(!ok_nPFO) {
      error() << "Failed to register nPFO histogram" <<endmsg;
      return StatusCode::FAILURE;
    }
    h_Frag_Energy = new TH1F("Frag_Energy","Fractional Fragment Energy;d_{#gamma#gamma sep}[mm];Events", m_bins_sep, m_min_sep, m_max_sep);
    ok_Frag_Energy&=reg(h_Frag_Energy);
    if(!ok_Frag_Energy){
      error() << "Failed to register Fragment Energy histogram" <<endmsg;
      return StatusCode::FAILURE;
    }

      debug() << "Did DiPhotonAnalysis Initialize " << endmsg;

      return Consumer::initialize();
    }
                                 

    void operator()(const edm4hep::ReconstructedParticleCollection& pfos,
                  const edm4hep::MCParticleCollection& mcps,
                  const edm4hep::ClusterCollection&, //clus
                  const edm4hep::RecoMCParticleLinkCollection& RecoMCTruthLinks) const override {

                    error() << " I AM A GOOD ALGORITHM THAT IS RUNNING " << endmsg;
                    // Loop over all PFOs to separate photon PFOs and fragment energy.
                    int nPFOs = 0;
                    int nPFOPhotons = 0;
                    double Total_PFO_Energy = 0.0;
                    double E_Frag = 0.0;
                    std::vector<PhotonPFO> photonPFOs;

                    // Create a navigator to get mc particles related to PFO
                    podio::LinkNavigator RecoMCTruthLinkNavigator(RecoMCTruthLinks);

                    for (size_t ir=0; ir<pfos.size(); ++ir){
                      const auto& pfo = pfos[ir];
                      info() << "pfo number: " << ir << endmsg;

                      const int pfo_pdg = pfo.getPDG();
                      debug() << "Pfo PDG: " << pfo_pdg << endmsg;

                      const auto pfo_energy = pfo.getEnergy();
                      const auto cluster_vec = pfo.getClusters();
                      int no_clusters = cluster_vec.size();

                      debug() << "No. Clusters in PFO: " << no_clusters << endmsg;
                      Vector3D pfo_position;

                      // If there is more than one cluster in the PFO, we have to manually calculate PFO information
                      if (no_clusters == 1){
                        debug() << "Single Cluster" << endmsg;
                        pfo_position.fill(cluster_vec[0].getPosition().x, cluster_vec[0].getPosition().y, cluster_vec[0].getPosition().z);
                      }
                      else if (no_clusters > 1) {
                        info() << "More than one cluster in Photon PFO! Number of clusters = " << no_clusters 
                               << "Switching to manual position computation." << endmsg;
                        std::vector<const edm4hep::CalorimeterHit*>  Total_Hit_vec;

                        for (int cluster_counter = 0; cluster_counter < no_clusters; ++cluster_counter) {
                          int no_cluster_hits = 0;
                          const auto cluster = cluster_vec[cluster_counter];
                           // push hits back to a container
                          for (const auto& hit: cluster.getHits()){
                            Total_Hit_vec.push_back(&hit);
                            ++no_cluster_hits;
                          }

                          debug() << no_cluster_hits << " Calo Hits in cluster " << cluster_counter << endmsg;
                        }

                        int n_total_hits = Total_Hit_vec.size();
                        debug() << "Total_Hit_vec size: " << n_total_hits << endmsg;
                        double COG_x_numerator = 0.0;
                        double COG_y_numerator = 0.0;
                        double COG_z_numerator = 0.0;
                        double Total_E_Hits = 0.0;
                        for (int hits_counter = 0; hits_counter < n_total_hits; ++hits_counter) {
                          double hit_E = Total_Hit_vec[hits_counter]->getEnergy();
                          Vector3D hit_position( Total_Hit_vec[hits_counter]->getPosition().x, Total_Hit_vec[hits_counter]->getPosition().y, Total_Hit_vec[hits_counter]->getPosition().z);
                          COG_x_numerator += (hit_E * hit_position.x());
                          COG_y_numerator += (hit_E * hit_position.y());
                          COG_z_numerator += (hit_E * hit_position.z());
                          Total_E_Hits += hit_E;
                        }

                        if (Total_E_Hits != pfo_energy) {
                          error() << "Energy of hits Total_E_Hits = " << Total_E_Hits 
                                       << " is not equal to pfo_energy = " << pfo_energy << " !?!?!?!"
                                       << endmsg;
                        }

                        pfo_position.fill( COG_x_numerator / Total_E_Hits,
                               COG_y_numerator / Total_E_Hits,
                               COG_z_numerator / Total_E_Hits );

                      }
                      else if (no_clusters == 0){
                        error() << "NO clusters in PFO!?!?!?" << endmsg;
                      }

                      // If the PFO is a photon, add it to our vector.
                      if (pfo_pdg == 22) {
                          ++nPFOPhotons;
                          PhotonPFO photon;
                          photon.energy = pfo_energy;
                          photon.position = pfo_position;
                          photon.pfo = &pfo;
                          photonPFOs.push_back(photon);
                      }
                      else {
                        E_Frag += pfo_energy;
                      }
                      Total_PFO_Energy += pfo_energy;
                      ++nPFOs;          
                      

                    if (photonPFOs.empty()){
                      error() << "No photon PFOs found!" << endmsg;
                      return;
                    }

                    // Now need to process the extracted photon PFO(s) and check links to the MC Particle
                    // Sort photon PFOs by energy (descending).
                    std::sort(photonPFOs.begin(), photonPFOs.end(), [](const PhotonPFO &a, const PhotonPFO &b) {
                        return a.energy > b.energy;
                    });

                    // Use the two highest–energy photon PFOs.
                    double E_PFO_1 = photonPFOs[0].energy;
                    double E_PFO_2 = (photonPFOs.size() > 1) ? photonPFOs[1].energy : 0.0;
                    Vector3D position_PFO_1 = photonPFOs[0].position;
                    Vector3D position_PFO_2 = (photonPFOs.size() > 1) ? photonPFOs[1].position : Vector3D(0,0,0);

                    for (size_t i = 2; i < photonPFOs.size(); ++i) {
                      E_Frag += photonPFOs[i].energy;
                    }


                    if (Total_PFO_Energy != E_PFO_1 + E_PFO_2 + E_Frag) {
                      error () << "*****************************************************************************************************" << "\n"
                              << "**************** FRAGMENT CHECK FAILED:  TOTAL PFO ENERGY: " << Total_PFO_Energy << "******************" << "\n"
                              << "****************                         E_PFO_1: " << E_PFO_1 << "***********************************" << "\n"
                              << "****************                         E_PFO_2: " << E_PFO_2 << "***********************************" << "\n"
                              << "****************                         E_Frag: " << E_Frag << "*************************************" << "\n"
                              << "****************                         E_PFO_1+E_PFO_2+E_Frag: " << E_PFO_1 + E_PFO_2 + E_Frag << "******" << "\n"
                              << "*****************************************************************************************************" << endmsg;
                    }

                    double frac_frag_energy = E_Frag / Total_PFO_Energy;
                    Vector3D PFO_separation_vector(
                        position_PFO_1.x() - position_PFO_2.x(),
                        position_PFO_1.y() - position_PFO_2.y(),
                        position_PFO_1.z() - position_PFO_2.z()
                    );
                    double PFO_separation = PFO_separation_vector.r();

                    debug() << " frac_frag_energy = " << E_Frag / Total_PFO_Energy << endmsg;

                    // Now retrieve the MC truth associated with each photon PFO,
                    // choose MCParticle with the highest weight.
                    const edm4hep::MCParticle* mcForPFO1 = nullptr;
                    const edm4hep::MCParticle* mcForPFO2 = nullptr;

                    // Process the first photon PFO.
                    const auto related1 = RecoMCTruthLinkNavigator.getLinked(*photonPFOs[0].pfo);
                    if (!related1.empty()){
                      float max_weight = 0.;
                      for (const auto& [mclinked, weight] : related1){
                        if (weight > max_weight){
                          max_weight = weight;
                          mcForPFO1 = &mclinked;                        
                      }
                    }
                  }
                  else {debug() << "No MC Particle linked to PFO" << endmsg;}

                  // Process the second photon PFO, if available.
                  if (photonPFOs.size() > 1) {
                    const auto related2 = RecoMCTruthLinkNavigator.getLinked(*photonPFOs[1].pfo);
                    if (!related2.empty()){
                      float max_weight = 0.;
                      for (const auto& [mclinked, weight] : related2){
                        if (weight > max_weight){
                          max_weight = weight;
                          mcForPFO2 = &mclinked;
                        }
                      } 
                    }
                  }
                  else {
                    // If there is only one photon PFO, use the MC truth candidates from the single PFO.
                    // Since there are always two MC particles (photons) in the event, we try to select the second-best candidate.
                    const auto related = RecoMCTruthLinkNavigator.getLinked(*photonPFOs[0].pfo);
                    if (related.size() > 1) {
                      // First, find the index corresponding to the highest weight (already used for mcForPFO1).
                      size_t index_max = 0;
                      float max_weight = 0.;
                      for (size_t i = 1; i < related.size(); i++){
                        const auto& related_weight = related[i].weight;
                        if (related_weight > max_weight) {
                          max_weight = related_weight;
                          index_max = i;
                        }
                      }
                      // Then, find the candidate with the next highest weight
                      size_t index_second = (index_max == 0) ? 1 : 0;
                      for (size_t i = 0; i < related.size(); i++){
                        const auto& related_weight = related[i].weight;
                        const auto& related_second_weight = related[index_second].weight;
                        if (i == index_max) continue;
                        if (related_weight > related_second_weight) {
                            index_second = i;
                        }
                      }
                      mcForPFO2 = &related[index_second].o;
                    }
                    else {
                       // Fallback: if there is only one candidate (for whatever reason), assign it as mcForPFO2 as well.
                       mcForPFO2 = mcForPFO1;
                       debug() << "Only one PFO candidate found" << endmsg;
                    }

                  }

                  // Extract MC truth information from the linked MCParticles.
                  double mc_photon_1_x = 0.0, mc_photon_1_y = 0.0, mc_photon_1_z = 0.0;
                  double mc_photon_2_x = 0.0, mc_photon_2_y = 0.0, mc_photon_2_z = 0.0;
                  Vector3D mc_photon_1_axis(0,0,0), mc_photon_2_axis(0,0,0);

                   if(mcForPFO1){
                      Vector3D vertex1(mcForPFO1->getVertex().x, mcForPFO1->getVertex().y, mcForPFO1->getVertex().z);
                      mc_photon_1_x = vertex1.x();
                      mc_photon_1_y = vertex1.y();
                      mc_photon_1_z = vertex1.z();
                      Vector3D momentum1(mcForPFO1->getMomentum().x, mcForPFO1->getMomentum().y, mcForPFO1->getMomentum().z);
                      double mag1 = momentum1.r();
                      if(mag1 > 0) {
                          mc_photon_1_axis = Vector3D(
                              momentum1.x() / mag1,
                              momentum1.y() / mag1,
                              momentum1.z() / mag1
                          );
                      }
                    }

                    if(mcForPFO2){
                        Vector3D vertex2(mcForPFO2->getVertex().x, mcForPFO2->getVertex().y, mcForPFO2->getVertex().z);
                        mc_photon_2_x = vertex2.x();
                        mc_photon_2_y = vertex2.y();
                        mc_photon_2_z = vertex2.z();
                        Vector3D momentum2(mcForPFO2->getMomentum().x, mcForPFO2->getMomentum().y, mcForPFO2->getMomentum().z);
                        double mag2 = momentum2.r();
                        if(mag2 > 0) {
                            mc_photon_2_axis = Vector3D(
                                momentum2.x() / mag2,
                                momentum2.y() / mag2,
                                momentum2.z() / mag2
                            );
                        }
                    }


                    double True_photon_separation = 0.0;
                    if(mcForPFO1 && mcForPFO2){
                        True_photon_separation = std::sqrt(
                            std::pow(mc_photon_2_x - mc_photon_1_x, 2) +
                            std::pow(mc_photon_2_y - mc_photon_1_y, 2) +
                            std::pow(mc_photon_2_z - mc_photon_1_z, 2)
                        );
                    }

                    /// Still to do: plotting workflow

                    // Fill (weighted) histograms
                    h_nPFO_photons->Fill(True_photon_separation, nPFOPhotons);
                    h_nPFO->Fill(True_photon_separation, nPFOs);
                    h_Frag_Energy->Fill(True_photon_separation, frac_frag_energy);
                    debug() << "True_photon_separation: " << True_photon_separation << endmsg;




                    //createMCPFOMaps();
                    
                    //PFOtoMCLink
                    //MCtoPFOLink

                  }

            }
      
      StatusCode finalize() override {
        if (Gaudi::Algorithm::finalize().isFailure()) return StatusCode::FAILURE;

        debug() << "Did DiPhotonAnalysis Finalize " << endmsg;
        return Consumer::finalize();
      }
      };



DECLARE_COMPONENT(DiPhotonAnalysis)