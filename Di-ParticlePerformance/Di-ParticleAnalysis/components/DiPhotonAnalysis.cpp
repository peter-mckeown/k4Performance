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
#include <TFile.h>
#include <TH1D.h>
#include <TCanvas.h>

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
  edm4hep::ReconstructedParticle pfo;
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
    /*
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
    */

    //// Attempt at Gaudi standalone histograms
    Gaudi::Property<std::string> m_histPath{this,"HistPath","/PLOTS",
      "THistSvc directory (must include stream, e.g. '/PLOTS/...')."};
    Gaudi::Property<int> m_bins_sep {this, "SeparationBins", 18, "True photon separation [mm]"};
    Gaudi::Property<double> m_min_sep {this, "MinSep", 0.0, "Minimum separation [mm]"};
    Gaudi::Property<double> m_max_sep {this, "MaxSep", 90.0, "Maximum separation [mm]"};

   
    mutable TH1D *h_nPFO_photons_counts=nullptr, *h_nPFO_photons_weighted=nullptr, *h_nPFO_photons=nullptr, *h_nPFO=nullptr, *h_nPFO_counts=nullptr, *h_nPFO_weighted=nullptr, *h_Frag_Energy=nullptr;

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

  /*
    TH1D* h_avg plot_weighted_average (const std::vector<double>& data,
                                const std::vector<double>& weights,
                                const int& nbins,
                                const double& minE,
                                const double& maxE){

      std::vector<double> bin_edges(nbins+1);

      double bin_width = (maxE - minE) / nbins;
      for (int i = 0; i <= nbins; ++i) {
        bin_edges[i] = minE + i*bin_width;
      }

      // Histograms
      TH1D* h_counts   = new TH1D("h_counts", "counts", nbins, &bin_edges[0]);
      TH1D* h_weighted = new TH1D("h_weighted", "weighted", nbins, &bin_edges[0]);
      TH1D* h_avg      = (TH1D*)h_weighted->Clone("h_avg"); 

      // Fill counts and weighted sums
      for (size_t i = 0; i < data.size(); ++i) {
          h_counts->Fill(data[i]);
          h_weighted->Fill(data[i], weights[i]);
      }

      // Compute bin-wise average
      h_avg->Divide(h_weighted, h_counts, "B");  // h_avg = h_weighted / h_counts

      // Modify histogram style
      h_avg->SetLineColor(kBlue);
      h_avg->SetLineWidth(2);
      h_avg->SetFillStyle(0);    // no fill (step)
      h_avg->SetLineStyle(1);    // solid line

      return h_avg;

    }
      */
    /// Initialize histograms etc
    StatusCode initialize() override{
      
      info() << "Initialising DiPhotonAnalysis" << endmsg;

      //// Attempt at Gaudi standalone histograms
      /*
      mutable Gaudi::Accumulators::StaticHistogram<1> h_nPFO_photons{this, "Reco_photons_sep", "Avg. No. Photon PFOs", m_bins_sep, m_min_sep, m_max_sep};
      mutable Gaudi::Accumulators::StaticHistogram<1> h_nPFO{this, "Reco_sep", "Avg. No. PFOs", m_bins_sep, m_min_sep, m_max_sep};
      mutable Gaudi::Accumulators::StaticHistogram<1> h_Frag_Energy{this, "Frag_Energy", "Fractional Fragment Energy", m_bins_sep, m_min_sep, m_max_sep};
      */


      h_nPFO_photons_counts = new TH1D("Reco_photons_sep_counts","No. Photon PFOs;d_{#gamma#gamma sep}[mm]; No. Photon PFOs", m_bins_sep, m_min_sep, m_max_sep);
      h_nPFO_photons_weighted = new TH1D("Reco_photons_sep_weighted","weighted No. Photon PFOs;d_{#gamma#gamma sep}[mm];weighted No. Photon PFOs", m_bins_sep, m_min_sep, m_max_sep);
      h_nPFO_photons = (TH1D*)h_nPFO_photons_weighted->Clone("Reco_photons_sep");
      h_nPFO_counts = new TH1D("Reco_sep_counts","No. PFOs;d_{#gamma#gamma sep}[mm];No. PFOs", m_bins_sep, m_min_sep, m_max_sep);
      h_nPFO_weighted = new TH1D("Reco_sep_weighted","weighted No. PFOs;d_{#gamma#gamma sep}[mm];weighted No. PFOs", m_bins_sep, m_min_sep, m_max_sep);
      h_nPFO = (TH1D*)h_nPFO_weighted->Clone("Reco_sep_weighted");
      h_Frag_Energy = new TH1D("Frag_Energy","Fractional Fragment Energy;d_{#gamma#gamma sep}[mm];Fractional Fragment Energy", m_bins_sep, m_min_sep, m_max_sep);

      if (Gaudi::Algorithm::initialize().isFailure()){
        return StatusCode::FAILURE;
      }

      /*
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
    */
      debug() << "Did DiPhotonAnalysis Initialize " << endmsg;

      return Consumer::initialize();
    }
                                 

    void operator()(const edm4hep::ReconstructedParticleCollection& pfos,
                  const edm4hep::MCParticleCollection& mcps,
                  const edm4hep::ClusterCollection&, //clus
                  const edm4hep::RecoMCParticleLinkCollection& RecoMCTruthLinks) const override {

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

                      auto related_test = RecoMCTruthLinkNavigator.getLinked(pfo);
                      debug() << "Test no. related mcps: " <<  related_test.size() << endmsg;

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
                          photon.pfo = pfo;
                          photonPFOs.push_back(photon);
                      }
                      else {
                        E_Frag += pfo_energy;
                      }
                      Total_PFO_Energy += pfo_energy;
                      ++nPFOs;          
                    }  

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
                    //const edm4hep::MCParticle* mcForPFO1 = nullptr;
                    //const edm4hep::MCParticle* mcForPFO2 = nullptr;

                    edm4hep::MCParticle mcForPFO1;
                    edm4hep::MCParticle mcForPFO2;

                    // Process the first photon PFO.
                    debug() << "Photon PFO: " << typeid(photonPFOs[0].pfo).name() << endmsg;
                    auto related1 = RecoMCTruthLinkNavigator.getLinked(photonPFOs[0].pfo);
                    debug() << "Number of related MC particles to PFO1: " << related1.size() << endmsg;
                    if (!related1.empty()){
                      float max_weight_MC1 = 0.;
                      for (const auto& [mclinked, weight] : related1){
                        debug() << "PFO 1, Which MC ID? " << mclinked.id() << endmsg;
                        if (weight > max_weight_MC1){
                          max_weight_MC1 = weight;
                          mcForPFO1 =  mclinked; //&mclinked;                       
                      }
                    }
                  }
                  else {warning() << "No MC Particle linked to PFO" << endmsg;}

                  debug() << "Done First PFO linking" << endmsg;
                  // Process the second photon PFO, if available.
                  if (photonPFOs.size() > 1) {
                    edm4hep::MCParticle Second_mcForPFO2;
                    if (photonPFOs.size() > 2) {
                      info() << "More than 2 photon PFOs; Leading two used in True Separation calculation" << endmsg;
                    }
                    debug() << "More than one Photon PFO" << endmsg;
                    auto related2 = RecoMCTruthLinkNavigator.getLinked(photonPFOs[1].pfo);
                    debug() << "Number of related MC particles to PFO2: " << related1.size() << endmsg;
                    if (!related2.empty()){
                      float max_weight = 0.;
                      float max_weight_2_P2 = 0.;
                      for (const auto& [mclinked, weight] : related2){
                        debug() << "PFO 2, Which MC ID? " << mclinked.id() << endmsg;
                        if (weight > max_weight){
                          max_weight = weight;
                          mcForPFO2 =  mclinked; //&mclinked; 
                        }
                        
                        // also record second highest energy photon PFO
                        else if (weight < max_weight &&  weight> max_weight_2_P2){
                              max_weight_2_P2 = weight;
                              Second_mcForPFO2 = mclinked;
                        }
                              
                      }
                      // Incase largest weight linked photon is the same for both PFOs take second largest weight for second PFO
                       if (mcForPFO1.id() == mcForPFO2.id()){
                        if (Second_mcForPFO2.isAvailable()){
                            mcForPFO2 = Second_mcForPFO2;
                        }
                      }
                      
                    }
                  }
                  else {
                    // If there is only one photon PFO, use the MC truth candidates from the single PFO.
                    // Since there are always two MC particles (photons) in the event, we try to select the second-best candidate.
                    const auto related = RecoMCTruthLinkNavigator.getLinked(photonPFOs[0].pfo);
                    debug() << "Number of related MCParticles: " << related.size() << endmsg;
                    if (related.size() > 1) {
                      // First, find the index corresponding to the highest weight (already used for mcForPFO1).
                      debug() << "More than one related mcParticles!!" << endmsg;
                      size_t index_max = 0;
                      float max_weight = 0.;
                      for (size_t i = 1; i < related.size(); i++){
                        const auto related_weight = related[i].weight;
                        if (related_weight > max_weight) {
                          max_weight = related_weight;
                          index_max = i;
                        }
                      }
                      // check this actually corresponds to the highest weight mc link used for PFO_1
                      if (mcForPFO1.id() != related[index_max].o.id()) {warning() << "non-matching highest weighted MC in single PFO scenario!" << endmsg;}
                      mcForPFO1 = related[index_max].o;

                      debug() << "mcForPFO1.id() " << mcForPFO1.id() << "related[index_max].o.id() " << related[index_max].o.id()  << endmsg;// Then, find the candidate with the next highest weight
                      size_t index_second = (index_max == 0) ? 1 : 0;
                      for (size_t i = 0; i < related.size(); i++){
                        const auto related_weight = related[i].weight;
                        const auto related_second_weight = related[index_second].weight;
                        debug() << "related_weight" << related_weight<< "related_second_weight" << related_second_weight << endmsg;
                        if (i == index_max) continue;
                        if ((related_weight > related_second_weight) && (related_weight < max_weight)) {
                            index_second = i;
                        }
                      }
                      //debug() << "Index_second" << index_second << "value: " << related[index_second] << endmsg;
                      auto linked = related[index_second];
                      mcForPFO2 = linked.o;  //&(linked.o);
                      //Vector3D vertex2(mcForPFO2->getVertex().x, mcForPFO2->getVertex().y, mcForPFO2->getVertex().z);
                    }
                    else {
                       // Fallback: if there is only one candidate (for whatever reason), assign it as mcForPFO2 as well.
                       mcForPFO2 = mcForPFO1;
                       debug() << "Only one MC candidate found" << endmsg;
                    }

                  }

                  debug() << "Done ALL PFO linking" << endmsg;
                  // Extract MC truth information from the linked MCParticles.
                  double mc_photon_1_x = 0.0, mc_photon_1_y = 0.0, mc_photon_1_z = 0.0;
                  double mc_photon_2_x = 0.0, mc_photon_2_y = 0.0, mc_photon_2_z = 0.0;
                  Vector3D mc_photon_1_axis(0,0,0), mc_photon_2_axis(0,0,0);

                   //if(mcForPFO1){
                   if (mcForPFO1.isAvailable()){
                      debug() << "MC PFO1 " <<    typeid(mcForPFO1).name() << " " << mcForPFO1 << endmsg;
                      Vector3D vertex1(mcForPFO1.getVertex().x, mcForPFO1.getVertex().y, mcForPFO1.getVertex().z);
                      mc_photon_1_x = vertex1.x();
                      mc_photon_1_y = vertex1.y();
                      mc_photon_1_z = vertex1.z();
                      Vector3D momentum1(mcForPFO1.getMomentum().x, mcForPFO1.getMomentum().y, mcForPFO1.getMomentum().z);
                      double mag1 = momentum1.r();
                      debug() << "MC for PFO 1: x:" << mc_photon_1_x <<  "y:" << mc_photon_1_y << "z:" << mc_photon_1_z << endmsg;
                      if(mag1 > 0) {
                          mc_photon_1_axis = Vector3D(
                              momentum1.x() / mag1,
                              momentum1.y() / mag1,
                              momentum1.z() / mag1
                          );
                      }
                      debug() << "Done MC for PFO 1" << endmsg;
                    }

                    //if(mcForPFO2){
                    if (mcForPFO2.isAvailable()){
                      debug() << "MC PFO2 " <<    typeid(mcForPFO2).name() << " " <<  mcForPFO2 << endmsg;
                      Vector3D vertex2(mcForPFO2.getVertex().x, mcForPFO2.getVertex().y, mcForPFO2.getVertex().z);
                        debug() << "Got PFO 2 vertex" << endmsg;
                        mc_photon_2_x = vertex2.x();
                        mc_photon_2_y = vertex2.y();
                        mc_photon_2_z = vertex2.z();
                        Vector3D momentum2(mcForPFO2.getMomentum().x, mcForPFO2.getMomentum().y, mcForPFO2.getMomentum().z);
                        double mag2 = momentum2.r();
                        debug() << "MC for PFO 2: x:" << mc_photon_2_x <<  "y:" << mc_photon_2_y << "z:" << mc_photon_2_z << endmsg;
                        if(mag2 > 0) {
                            mc_photon_2_axis = Vector3D(
                                momentum2.x() / mag2,
                                momentum2.y() / mag2,
                                momentum2.z() / mag2
                            );
                        }
                        debug() << "Done MC for PFO 2" << endmsg;
                    }

                    debug() << "About to get true di-photon separation" << endmsg;
                    double True_photon_separation = 0.0;
                    //if(mcForPFO1 && mcForPFO2){
                    if (mcForPFO1.isAvailable() && mcForPFO2.isAvailable()){
                        True_photon_separation = std::sqrt(
                            std::pow(mc_photon_2_x - mc_photon_1_x, 2) +
                            std::pow(mc_photon_2_y - mc_photon_1_y, 2) +
                            std::pow(mc_photon_2_z - mc_photon_1_z, 2)
                        );
                    }


                     debug() << "True_photon_separation: " << True_photon_separation << endmsg;

                    /// Still to do: plotting workflow

                    debug() << "About to fill histograms" << endmsg;

                    // Fill (weighted) histograms
                    // Gaudi
                    /*
                    h_nPFO_photons += std::make_pair(True_photon_separation, nPFOPhotons);
                    h_nPFO += std::make_pair(True_photon_separation, nPFOs);
                    h_Frag_Energy += std::make_pair(True_photon_separation, frac_frag_energy);
                    */

                    // Fill histograms
                    h_nPFO_photons_counts->Fill(True_photon_separation);
                    h_nPFO_counts->Fill(True_photon_separation);

                    // Fill (weighted) histograms
                    h_nPFO_photons_weighted->Fill(True_photon_separation, nPFOPhotons);
                    h_nPFO_weighted->Fill(True_photon_separation, nPFOs);
                    h_Frag_Energy->Fill(True_photon_separation, frac_frag_energy);

                    //createMCPFOMaps();
                    
                    //PFOtoMCLink
                    //MCtoPFOLink

            }
      
      StatusCode finalize() override {

        auto file = TFile::Open("DiPhoton_histograms.root", "RECREATE");

        h_nPFO_photons->Divide(h_nPFO_photons_weighted, h_nPFO_photons_counts);
        h_nPFO_photons->Write();
        delete h_nPFO_photons;
        delete h_nPFO_photons_weighted;
        delete h_nPFO_photons_counts;
        h_nPFO->Divide(h_nPFO_weighted, h_nPFO_counts);
        h_nPFO->Write();
        delete h_nPFO;
        delete h_nPFO_weighted;
        delete h_nPFO_counts;
        h_Frag_Energy->Write();
        delete h_Frag_Energy;

        file->Close();


        if (Gaudi::Algorithm::finalize().isFailure()) return StatusCode::FAILURE;

        debug() << "Did DiPhotonAnalysis Finalize " << endmsg;
        return StatusCode::SUCCESS;
      }
      };



DECLARE_COMPONENT(DiPhotonAnalysis)