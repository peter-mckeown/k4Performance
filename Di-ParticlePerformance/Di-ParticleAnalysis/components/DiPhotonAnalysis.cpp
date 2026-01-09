#include "DiPhotonAnalysis.hpp"

#include <Eigen/Dense>
#include <cmath>
#include <vector>
#include <algorithm>


#include "k4FWCore/Consumer.h"

#include "edm4hep/ReconstructedParticleCollection.h"
#include "edm4hep/MCParticleCollection.h"
#include "edm4hep/ClusterCollection.h"
#include "edm4hep/RecoMCParticleLinkCollection.h"
#include "podio/ObjectID.h"

namespace{

struct ObjectIDLess {
    bool operator()(const podio::ObjectID& a, const podio::ObjectID& b) const noexcept {
      return (a.collectionID < b.collectionID) ||
             (a.collectionID == b.collectionID && a.index < b.index);
    }
  };

// A structure to hold photon PFo information, and pointer to the original pfo
struct PhotonPFO {
  double energy;
  edm4hep::Vector3d position;
  const edm4hep::ReconstructedParticle& pfo;
};

}
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
        KeyValues("InputRecoMC",      {"MCTruthRecoLink"})
    }) {}

    /// This part is all for plotting
    // Services & booking flag
    //mutable SmartIF<ITHistSvc> m_histSvc;
    //mutable bool m_booked = false;

    /*
    std::map createMCPFOMaps(const edm4hep::MCParticleCollection& mcpColl, 
                            const edm4hep::ReconstructedParticleCollection& pfoColl,
                            const edm4hep::RecoMCParticleLinkCollection& linkColl){
                                // Transcribed from original LCIO, following Anna's implementation: https://github.com/Zehvogel/k4Performance/blob/main/PFlowValidation/components/PFOtoMCviaClusterLink.cpp
                                
                                // Index MC Particle by Object ID
                                std::map<podio::ObjectID, size_t, ObjectIDLess> mcIndex;
                                for (size_t i=0; i<mcpColl.size(); ++i) mcIndex[ mcpColl[i].getObjectID() ] = i;

                                // Collect primaries in vector
                                std::vector<>
     }
      */
                                 

    void operator()(const edm4hep::ReconstructedParticleCollection& pfos,
                  const edm4hep::MCParticleCollection& mcps,
                  const edm4hep::ClusterCollection& /*clus*/,
                  const edm4hep::RecoMCParticleLinkCollection& links) const override {

                    // Loop over all PFOs to separate photons PFOs and fragment energy.
                    int nPFOs = 0;
                    int nPFOPhotons = 0;
                    double Total_PFO_Energy = 0.0;
                    double E_Frag = 0.0;
                    std::vector<PhotonPFO> photonPFOs;

                    for (size_t ir=0; ir<pfos.size(); ++ir){
                      const auto& pfo = pfos[ir];
                      //info << 
                      info() << "pfo number: " << ir << endmsg;
                    }



                    //createMCPFOMaps();
                    
                    //PFOtoMCLink
                    //MCtoPFOLink

                  }

    };

    DECLARE_COMPONENT(DiPhotonAnalysis)