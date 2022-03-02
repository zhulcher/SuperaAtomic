#ifndef __LARTPCMLRECO3D_CXX__
#define __LARTPCMLRECO3D_CXX__

#include "LArTPCMLReco3D.h"

namespace supera {

    LArTPCMLReco3D::LArTPCMLReco3D()
    : LabelAlgorithm()
    , _debug(0)
    {}

    void LArTPCMLReco3D::Configure(const PSet& cfg)
    {
        _semantic_priority.resize((size_t)(supera::kShapeUnknown));
        for(size_t i=0; i<_semantic_priority.size(); ++i)
            _semantic_priority[i]=i;
        
        _semantic_priority = cfg.get<std::vector<size_t> >("SemanticPriority",_semantic_priority);

        _touch_threshold = cfg.get<size_t>("TouchDistance",1);
        _delta_size = cfg.get<size_t>("DeltaSize",10);
        _eioni_size = cfg.get<size_t>("IonizationSize",5);
        _compton_size = cfg.get<size_t>("ComptonSize",10);
        _edep_threshold = cfg.get<double>("EnergyDepositThreshold",0.01);

        _use_sed = cfg.get<bool>("UseSimEnergyDeposit");
        _use_sed_points = cfg.get<bool>("UseSimEnergyDepositPoints");
        _store_dedx = cfg.get<bool>("StoreDEDX",false);

        _use_true_pos = cfg.get<bool>("UseTruePosition",true);
        _check_particle_validity = cfg.get<bool>("CheckParticleValidity",true);

        std::vector<double> min_coords(3,std::numeric_limits<double>::min());
        std::vector<double> max_coords(3,std::numeric_limits<double>::max());

        min_coords = cfg.get<std::vector<double> >("WorldBoundMin",min_coords);
        max_coords = cfg.get<std::vector<double> >("WorldBoundMax",max_coords);

        _world_bounds.update(min_coords.at(0),min_coords.at(1),min_coords.at(2),
            max_coords.at(0),max_coords.at(1),max_coords.at(2));

    }

    EventOutput LArTPCMLReco3D::Generate(const EventInput& data, const ImageMeta3D& meta)
    {

        _mcpl.InferParentage(data);

        auto const& trackid2index = _mcpl.TrackIdToIndex();

        EventOutput result;

        return result;
    }
/*
    void SuperaMCParticleCluster::MergeShowerIonizations(const EventInput& part_grp_v)
    {
        // Loop over particles of a type kIonization (=touching to its parent physically by definition)
        // If a parent is found, merge to the parent
        int merge_ctr = 0;
        int invalid_ctr = 0;
        do {
            merge_ctr = 0;
            for(auto& grp : part_grp_v) {
                if(!grp.part.valid) continue;
                if(grp.part.type != supera::kIonization) continue;
                // merge to a valid "parent"
                bool parent_found = false;
                int parent_index = grp.part.parent_trackid;
                int parent_index_before = grp.part.trackid;
                while(1) {
                    if(parent_index <0) {
                        if(_debug>1) {
                            std::cout << "Invalid parent track id " << parent_index
                            << " Could not find a parent for " << grp.part.trackid << " PDG " << grp.part.pdg
                            << " " << grp.part.process << " E = " << grp.part.energy_init
                            << " (" << grp.part.energy_deposit << ") MeV" << std::endl;
                            auto const& parent = part_grp_v[parent_index_before].part;
                            std::cout << "Previous parent: " << parent.trackid << " PDG " << parent.pdg
                            << " " << parent.process
                            << std::endl;
                        }
                        parent_found=false;
                        invalid_ctr++;
                        break;
                    }
                    auto const& parent = part_grp_v[parent_index].part;
                    parent_found = parent.valid;
                    if(parent_found) break;
                    else{
                        int ancestor_index = parent.part.parent_trackid;
                        if(ancestor_index == parent_index) {
                          std::cout << "Particle " << parent_index << " is root and invalid particle..." << std::endl
                          << "PDG " << parent.part.pdg << " " << parent.part.process << std::endl;
                          break;
                      }
                      parent_index_before = parent_index;
                      parent_index = ancestor_index;
                  }
              }
                // if parent is found, merge
              if(parent_found) {
                auto& parent = part_grp_v[parent_index];
                parent.Merge(grp);
                merge_ctr++;
            }

        }
        if(_debug>0){
            std::cout << "Ionization merge counter: " << merge_ctr << " invalid counter: " << invalid_ctr << std::endl;
        }
        }while(merge_ctr>0);
    }
    */
}
#endif