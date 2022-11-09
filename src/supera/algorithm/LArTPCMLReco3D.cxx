#ifndef __LARTPCMLRECO3D_CXX__
#define __LARTPCMLRECO3D_CXX__

#include "LArTPCMLReco3D.h"

#include <algorithm>
#include <cassert>
#include <set>
#include <string>
#include <sstream>

namespace supera {

    LArTPCMLReco3D::LArTPCMLReco3D()
    : LabelAlgorithm()
    , _debug(0)
    {}

    // --------------------------------------------------------------------
    void LArTPCMLReco3D::Configure(const PSet& cfg)
    {        
        _semantic_priority.clear();
        _semantic_priority = cfg.get<std::vector<size_t> >("SemanticPriority",_semantic_priority);
        this->SetSemanticPriority(_semantic_priority);

        _touch_threshold = cfg.get<size_t>("TouchDistance",1);
        _delta_size = cfg.get<size_t>("DeltaSize",3);
        _eioni_size = cfg.get<size_t>("IonizationSize",5);
        _compton_size = cfg.get<size_t>("ComptonSize",10);
        _edep_threshold = cfg.get<double>("EnergyDepositThreshold",0.01);
        _lescatter_size = cfg.get<size_t>("LEScatterSize",2);
        _use_sed = cfg.get<bool>("UseSimEnergyDeposit");
        _use_sed_points = cfg.get<bool>("UseSimEnergyDepositPoints");
        _store_dedx = cfg.get<bool>("StoreDEDX",false);
        _store_lescatter = cfg.get<bool>("StoreLEScatter",true);

        _use_true_pos = cfg.get<bool>("UseTruePosition",true);
        _check_particle_validity = cfg.get<bool>("CheckParticleValidity",true);

        std::vector<double> min_coords(3,std::numeric_limits<double>::lowest());
        std::vector<double> max_coords(3,std::numeric_limits<double>::max());

        min_coords = cfg.get<std::vector<double> >("WorldBoundMin",min_coords);
        max_coords = cfg.get<std::vector<double> >("WorldBoundMax",max_coords);

        _world_bounds.update(min_coords.at(0),min_coords.at(1),min_coords.at(2),
            max_coords.at(0),max_coords.at(1),max_coords.at(2));

    }
    // --------------------------------------------------------------------

    void LArTPCMLReco3D::SetSemanticPriority(std::vector<size_t>& order)
    {
        std::vector<size_t> result;
        std::vector<bool> assigned((size_t)(supera::kShapeUnknown),false);
        for(auto const& type : order) {
            if(type >= supera::kShapeUnknown) {
                LOG.FATAL() << "SemanticPriority received an unsupported semantic type " << type << "\n";
                throw meatloaf();
            }
            bool ignore = false;
            for(auto const& used : result) {
                if(used != type) continue;
                ignore = true;
            }
            if(ignore) {
                LOG.FATAL() << "Duplicate SemanticPriority received for type " << type << "\n";
                throw meatloaf();
            }
            result.push_back(type);
            assigned[type]=true;
        }

        // Now add other types to make sure
        for(size_t i=0; i<assigned.size(); ++i)
        {
            if(assigned[i]) continue;
            result.push_back(i);
        }
        if(result.size() != (size_t)(kShapeUnknown)) {
            LOG.FATAL() << "Logic error!\n";
            throw meatloaf();
        }
        order = result;
    }

    EventOutput LArTPCMLReco3D::Generate(const EventInput& data, const ImageMeta3D& meta)
    {
        EventOutput result;

        std::cout<<" Delta Size " << _delta_size << std::endl;

        // fill in the working structures that link the list of particles and its genealogy
        _mcpl.InferParentage(data);
        std::vector<supera::Index_t> const& trackid2index = _mcpl.TrackIdToIndex();

        // Assign the initial labels for each particle.
        // They will be grouped together in various ways in the subsequent steps.
        std::vector<supera::ParticleLabel> labels = this->InitializeLabels(data, meta);

        // Now group the labels together in certain cases
        // (e.g.: electromagnetic showers, neutron clusters, ...)
        // There are lots of edge cases so the logic is spread out over many methods.
        //this->MergeShowerIonizations(labels); // merge supera::kIonization = too small delta rays into parents
        // ** TODO identify and merge too-small shower fragments to other touching showers **
        this->MergeShowerTouchingElectron(meta, labels); // merge larcv::kShapeLEScatter to touching shower
        // Apply energy threshold (may drop some pixels)
        this->ApplyEnergyThreshold(labels);
        this->SetSemanticType(labels); 
        this->MergeShowerConversion(labels); // merge supera::kConversion a photon merged to a parent photon
        this->MergeShowerFamilyTouching(meta, labels); // merge supera::kShapeShower to touching parent shower/delta/michel
        this->MergeShowerTouching(meta, labels); // merge supera::kShapeShower to touching shower in the same family tree
        this->MergeShowerTouchingLEScatter(meta,labels);
        // ** TODO consider this separate from MergeShowerIonizations?? **
        this->MergeDeltas(labels); // merge supera::kDelta to a parent if too small

        // Re-classify small photons into ShapeLEScatter
        for(auto& label : labels) {
            if(!label.valid) continue;
            if(label.part.type != supera::kPhoton) continue;
            if(label.energy.size() < _compton_size)
                label.part.shape = supera::kShapeLEScatter;
        }

        // ** TODO Assign supera::SemanticType_t **

        // Now that we have grouped the true particles together,
        // at this point we're ready to build a new set of labels
        // which contain only the top particle of each merged group.
        // The first step will be to create a mapping
        // from the original GEANT4 trackids to the new groups.
        std::vector<supera::Index_t> trackid2output(trackid2index.size(), kINVALID_INDEX);  // index: original GEANT4 trackid.  stored value: output group index.
        std::vector<supera::TrackID_t> output2trackid;  // reverse of above.
        output2trackid.reserve(trackid2index.size());
        
        this->RegisterOutputParticles(trackid2index, labels, output2trackid, trackid2output);

        this->SetGroupID(labels);

        this->SetAncestorAttributes(labels);

        this->SetInteractionID(labels);
        /*
        result = labels;
        return result;
        */

        this->BuildOutputLabels(labels,result,output2trackid);

        // Next, we need to clean up a number of edge cases that don't always get assigned correctly.
        //this->FixOrphanShowerGroups(labels, output2trackid, trackid2output);
        //this->FixOrphanNonShowerGroups(labels, output2trackid, trackid2output);
        //this->FixInvalidParentShowerGroups(labels, output2trackid, trackid2output);
        //this->FixUnassignedParentGroups(labels, output2trackid, trackid2output);
        //this->FixUnassignedGroups(labels, output2trackid);
        //this->FixUnassignedLEScatterGroups(labels, output2trackid);
        //LArTPCMLReco3D::FixFirstStepInfo(labels, meta, output2trackid);

        // We're finally to fill in the output container.
        // There are two things we need:
        //  (1) labels for each voxel (what semantic type is each one?)
        //  (2) labels for particle groups.
        // The output format is an object containing a collection of supera::ParticleLabel,
        // each of which has voxels attached to it, so that covers both things.
        // EventOutput computes VoxelSets with the sum across all particles
        // for voxel energies, dE/dxs, and semantic labels
        // upon demand (see EventOutput::VoxelEnergies() etc.).
        //result = this->BuildOutputLabels(labels, output2trackid, trackid2output, trackid2index);
        return result;
    }

    // --------------------------------------------------------------------

    void LArTPCMLReco3D::BuildOutputLabels(std::vector<supera::ParticleLabel>& labels,
        supera::EventOutput& result, 
        const std::vector<TrackID_t>& output2trackid) const
    {
        // Build the outupt
        std::vector<supera::ParticleLabel> output_particles;
        output_particles.reserve(output2trackid.size());
        for(auto const& trackid : output2trackid) {
            auto index = this->InputIndex(trackid);
            output_particles.emplace_back(std::move(labels[index]));
            labels[index].valid=false;
        }

        // Semantic label
        // dedx energy semanticlabels
        std::cout<<_semantic_priority.size()<<" semantic types..."<<std::endl;
        for (auto rit = _semantic_priority.crbegin(); rit != _semantic_priority.crend(); ++rit)
        {
            auto stype = supera::SemanticType_t((*rit));
            for(auto& label : output_particles) {
                if(label.part.shape != stype)
                    continue;
                // Contribute to the output
                assert(label.energy.size() == label.dedx.size());
                //result._dEdXs.reserve(label.energy.size()+result._dEdXs.size());
                result._energies.reserve(label.energy.size()+result._energies.size());
                result._semanticLabels.reserve(label.energy.size()+result._semanticLabels.size());
                auto const& input_dedx   = label.dedx.as_vector();
                auto const& input_energy = label.energy.as_vector();
                for(size_t i=0; i<input_dedx.size(); ++i) {
                    //result._dEdXs.emplace(input_dedx[i].id(),input_dedx[i].value(),true);
                    result._energies.emplace(input_energy[i].id(),input_energy[i].value(),true);
                    result._semanticLabels.emplace(input_energy[i].id(),(float)(stype),false);
                }
            }
            // If this is LEScatter type, and if _store_lescatter == false, make sure to add here
            if(stype == supera::kShapeLEScatter && !_store_lescatter) 
            {
                for(auto& label : labels){
                    
                    if(!label.valid) continue;

                    if(label.part.shape != supera::kShapeUnknown) {
                        LOG.FATAL() << "Unexpected (logic error): valid particle remaining that is not kShapeUnknown shape...\n"
                        << label.dump() << "\n";
                        throw meatloaf();
                    }
                    // Contribute to the output
                    assert(label.energy.size() == label.dedx.size());
                    //result._dEdXs.reserve(label.energy.size()+result._dEdXs.size());
                    result._energies.reserve(label.energy.size()+result._energies.size());
                    result._semanticLabels.reserve(label.energy.size()+result._semanticLabels.size());
                    auto const& input_dedx   = label.dedx.as_vector();
                    auto const& input_energy = label.energy.as_vector();
                    for(size_t i=0; i<input_dedx.size(); ++i) {
                        //result._dEdXs.emplace(input_dedx[i].id(),input_dedx[i].value(),true);
                        result._energies.emplace(input_energy[i].id(),input_energy[i].value(),true);
                        result._semanticLabels.emplace(input_energy[i].id(),(float)(stype),false);
                    }
                }
            }
            std::cout<<"Semantic type "<<stype << " ... " << result._energies.size()<<" pixels..."<<std::endl;
        }

        result = std::move(output_particles);

    }

    // --------------------------------------------------------------------

    // ------------------------------------------------------
    void LArTPCMLReco3D::MergeParticleLabel(std::vector<supera::ParticleLabel>& labels,
        TrackID_t dest_trackid,
        TrackID_t target_trackid) const 
    {
        auto& dest   = labels.at(this->InputIndex(dest_trackid));
        auto& target = labels.at(this->InputIndex(target_trackid));
        dest.Merge(target);
        for(auto const& trackid : target.merged_v)
            labels.at(this->InputIndex(trackid)).merge_id = dest.part.trackid;
    }
    // ------------------------------------------------------


    // ------------------------------------------------------
    void LArTPCMLReco3D::SetGroupID(std::vector<supera::ParticleLabel>& labels) const
    {
        for(auto& label : labels){
            if(!label.valid) continue;

            auto& part = label.part;

            // Primary particles get its own group ID
            if( part.trackid == part.parent_trackid ) {
                part.group_id = part.id;
            }else{

                auto parent_index = this->InputIndex(part.parent_trackid);

                switch(part.shape)
                {
                    case kShapeTrack:
                    case kShapeMichel:
                        part.group_id = part.id;
                        break;

                    case kShapeDelta:
                        if(parent_index == kINVALID_INDEX || !labels[parent_index].valid) {
                            LOG.FATAL() << "Delta ray with an invalid parent is not allowed!\n";
                            throw meatloaf();
                        }
                        part.group_id = labels[parent_index].part.id;
                        break;

                    case kShapeShower:
                        part.group_id = part.id;
                        for(auto const& parent_trackid : _mcpl.ParentTrackIdArray(label.part.trackid))
                        {
                            parent_index = this->InputIndex(parent_trackid);
                            if(parent_index == kINVALID_INDEX) continue;
                            if(labels[parent_index].part.shape != kShapeShower)
                                break;
                            if(labels[parent_index].part.id == kINVALID_INSTANCEID)
                                continue;
                            part.group_id = labels[parent_index].part.id;
                        }
                        break;

                    case kShapeLEScatter:
                        break;

                    default:
                        LOG.FATAL() << " Unexpected shape type " << part.shape << "\n";
                        throw meatloaf();
                        break;
                }
            }
        }
    }

    // ------------------------------------------------------

    void LArTPCMLReco3D::SetAncestorAttributes(std::vector<supera::ParticleLabel>& labels) const
    {
        for(auto& label : labels){

            if(!label.valid) continue;
            auto parent_trackid   = label.part.parent_trackid;
            auto ancestor_trackid = label.part.ancestor_trackid;
            auto const& parent_trackid_v = _mcpl.ParentTrackIdArray(label.part.trackid);

            // Consistency check
            if(parent_trackid == kINVALID_TRACKID && parent_trackid_v.size())
                parent_trackid = parent_trackid_v.front();

            if(ancestor_trackid == kINVALID_TRACKID && parent_trackid_v.size())
                ancestor_trackid = parent_trackid_v.back();

            if(!parent_trackid_v.empty() && parent_trackid_v.front() != parent_trackid) {
                LOG.FATAL() << "Logic error: the parent track ID " << parent_trackid
                << " != the first in the ancestory track IDs " << parent_trackid_v.front() << "\n";
                throw meatloaf();
            }

            if(!parent_trackid_v.empty() && parent_trackid_v.back() != ancestor_trackid) {
                LOG.FATAL() << "Logic error: the ancestor track ID " << ancestor_trackid
                << " != the most distant parent ID " << parent_trackid_v.back() << "\n";
                throw meatloaf();
            }

            // Now parent_trackid must be filled unless the input data was insufficient
            if(parent_trackid == kINVALID_TRACKID){
                LOG.FATAL() << "Parent track ID missing for a particle track ID " 
                << label.part.trackid << "\n"
                << "Check the input data and make sure all particles have a parent track ID\n";
                throw meatloaf();
            }

            // If ancestor_trackid is invalid, set it to the parent.
            if(ancestor_trackid == kINVALID_TRACKID) {
                LOG.INFO() << "Ancestor track ID not set for a particle track ID "
                << label.part.trackid << "\n"
                << "Setting it to the parent track ID " << parent_trackid << "\n";
                ancestor_trackid = parent_trackid;
            }

            // Attempt to fill parent info. 
            auto parent_index = this->InputIndex(parent_trackid);
            if(parent_index != kINVALID_INDEX) {
                auto const& parent = labels[parent_index];
                label.part.parent_trackid = parent.part.trackid;
                label.part.parent_id  = parent.part.id;
                label.part.parent_pdg = parent.part.pdg;
                label.part.parent_vtx = parent.part.vtx;
                label.part.parent_process = parent.part.process; 
            }

            // Attempt to fill ancestor info
            auto ancestor_index = this->InputIndex(ancestor_trackid);
            if(ancestor_index != kINVALID_INDEX) {
                auto const& ancestor = labels[ancestor_index];
                label.part.ancestor_trackid = ancestor.part.trackid;
                label.part.ancestor_id  = ancestor.part.id;
                label.part.ancestor_pdg = ancestor.part.pdg;
                label.part.ancestor_vtx = ancestor.part.vtx;
                label.part.ancestor_process = ancestor.part.process; 
            }
        }
    }


    // ------------------------------------------------------


    void LArTPCMLReco3D::SetInteractionID(std::vector<supera::ParticleLabel>& labels) const
    {

        std::vector<supera::Vertex> int2vtx;
        for(auto& label : labels) {
            if(!label.valid) continue;

            InstanceID_t iid = kINVALID_INSTANCEID;
            for(InstanceID_t cand_iid=0; cand_iid<int2vtx.size(); ++cand_iid)
            {
                auto const& vtx = int2vtx[cand_iid];
                if(vtx == label.part.ancestor_vtx){
                    iid = cand_iid;
                    break;
                }
            }
            if(iid == kINVALID_INSTANCEID){
                iid = int2vtx.size();
                int2vtx.push_back(label.part.ancestor_vtx);
            }
            label.part.interaction_id = iid;
        }
    }
    // ------------------------------------------------------


    // ------------------------------------------------------
    void LArTPCMLReco3D::ApplyEnergyThreshold(std::vector<supera::ParticleLabel>& labels) const
    {
        // Loop again and eliminate voxels that has energy below threshold
        for (auto &label : labels)
        {
            supera::VoxelSet energies, dEdXs;
            energies.reserve (label.energy.size() );
            dEdXs.reserve    (label.dedx.size()   );

            const auto energy_vec =  label.energy.as_vector();
            const auto dedx_vec   =  label.dedx.as_vector();
            for (std::size_t idx = 0; idx < energy_vec.size(); idx++)
            {
                const auto & vox = energy_vec[idx];
                if (vox.value() < _edep_threshold)
                    continue;
                if (dedx_vec[idx].id() != vox.id()) {
                    LOG.FATAL() << "Unmatched voxel ID between dE/dX and energy voxels \n";
                    throw meatloaf();
                }
                energies.emplace (vox.id(), vox.value(),           true);
                dEdXs.emplace    (vox.id(), dedx_vec[idx].value(), true);
            }
            label.energy = std::move(energies);
            label.dedx = std::move(dEdXs);
        }
    } // LArTPCMLReco3D::ApplyEnergyThreshold()

    // ------------------------------------------------------
    void LArTPCMLReco3D::SetSemanticType(std::vector<supera::ParticleLabel>& labels) const
    {
        for(auto& label : labels) {
            if(!label.valid) continue;

            switch(label.part.type) {
                case kInvalidProcess:
                label.part.shape = supera::kShapeUnknown;
                throw meatloaf();

                case kTrack:
                case kNeutron:
                label.part.shape = supera::kShapeTrack;
                break;

                case kPrimary:
                if(std::abs(label.part.pdg) != 11 && label.part.pdg != 22) 
                {
                    label.part.shape = supera::kShapeTrack;
                    break;
                }
                label.part.shape = supera::kShapeShower;
                break;

                case kDelta:
                    if(label.energy.size() < _delta_size)
                        label.part.shape = kShapeLEScatter;
                    else
                        label.part.shape = kShapeDelta;
                    break;

                case kDecay:
                    if(std::abs(label.part.pdg) == 11 && std::abs(label.part.parent_pdg) == 13)
                        label.part.shape = kShapeMichel;
                    else if(std::abs(label.part.pdg)==11 || label.part.pdg == 22) {
                        if(label.energy.size() > _compton_size)
                            label.part.shape = kShapeShower;
                        else
                            label.part.shape = kShapeLEScatter;
                    }else{
                        label.part.shape = kShapeTrack;
                    }
                    break;

                case kIonization:
                case kPhotoElectron:
                    label.part.shape = kShapeLEScatter;
                    break;

                case kPhoton:
                    label.part.shape = kShapeShower;
                    break;

                case kConversion:
                case kCompton:
                case kOtherShower:
                    if(label.energy.size() > _compton_size)
                        label.part.shape = kShapeShower;
                    else
                        label.part.shape = kShapeLEScatter;
                    break;
            }
        }
    }


    // ------------------------------------------------------
    
    void LArTPCMLReco3D::RegisterOutputParticles(const std::vector<TrackID_t> &trackid2index,
        std::vector<supera::ParticleLabel> &inputLabels,
        std::vector<TrackID_t> &output2trackid,
        std::vector<Index_t> &trackid2output) const
    {
        /*
            This function registers the output particles to be stored.
            It entails 2 actions
                1. Provide output ID = index number in the output vector of particles
                    1.1. First treat particles that are not kShapeLEScatter
                    1.2. Second, provide output ID to kShapeLEScatter particles
                2. Set merged particle track ID to also point to the output ID of its superset particle
                3. Record track ID => output index mapping
            In addition, for those particles to be stored:
                4. Set the first and the last step 
        */

        std::vector<Index_t> lescatter_index_v;
        lescatter_index_v.reserve(trackid2index.size());    
    
        // first create the track id list
        output2trackid.clear();
        output2trackid.reserve(trackid2index.size());

        // assign particle group ID numbers and make sure they have all info set
        LOG.VERBOSE() << "Considering incoming particles:\n";
        for (size_t label_index=0; label_index<inputLabels.size(); ++label_index)
        {
            auto& inputLabel = inputLabels[label_index];
            LOG.VERBOSE() << " Particle ID=" << inputLabel.part.id << " Track ID=" << inputLabel.part.trackid << "\n";
            LOG.VERBOSE() << "     PDG=" << inputLabel.part.pdg << "\n";
            LOG.VERBOSE() << "     Edep=" << inputLabel.part.energy_deposit << "\n";

            inputLabel.part.energy_deposit = inputLabel.energy.size() ? inputLabel.energy.sum() : 0.;

            if (!inputLabel.valid)
            {
                LOG.VERBOSE() << "   --> invalid particle (i.e. already merged), skipping \n";
                continue;
            }
            /*
            if (inputLabel.Size() < 1)
            {
                LOG.VERBOSE() << "   --> no voxels, skipping \n";
                continue;
            }
            */
            if (inputLabel.part.trackid == supera::kINVALID_TRACKID) 
            {
                LOG.VERBOSE() << "   --> Invalid TrackID, skipping\n";
                continue;
            }
            if (inputLabel.part.shape == supera::kShapeLEScatter) {
                LOG.VERBOSE() << "   --> LEScatter, skipping in the first loop\n";
                lescatter_index_v.push_back(label_index);
                continue;
            }
            if (inputLabel.part.shape == supera::kShapeUnknown) {
                LOG.FATAL()   << "   --> ShapeUnknown found and unexpected!\n"
                << inputLabel.dump() << "\n";
                throw meatloaf();
            }

            auto &part = inputLabel.part;
            // 1. Record output ID
            part.id = output2trackid.size();
            LOG.VERBOSE() << "   --> Assigned output id = " << part.id << "\n";

            // 2. Set merged particle track ID to also point to the output ID of its superset particle
            trackid2output[part.trackid] = part.id;
            for (auto const &child : inputLabel.merged_v)
                trackid2output[child] = part.id;

            // 3. Record track ID => Output index mapping            
            output2trackid.push_back(inputLabel.part.trackid);

            // 4. Set the first and last step 
            auto const &first_pt = inputLabel.first_pt;
            auto const &last_pt = inputLabel.last_pt;
            if (first_pt.t != kINVALID_DOUBLE)
                part.first_step = supera::Vertex(first_pt.x, first_pt.y, first_pt.z, first_pt.t);
            if (last_pt.t != kINVALID_DOUBLE)
                part.last_step = supera::Vertex(last_pt.x, last_pt.y, last_pt.z, last_pt.t);
            LOG.VERBOSE() << "  true particle start: " << part.first_step.dump() << "\n"
                          << "                  end: " << part.last_step.dump() << "\n";

        }

        // If LEScatter particles are meant to be kept, assign the output ID
        if(_store_lescatter) {
            for(auto const& label_index : lescatter_index_v) {
                auto& inputLabel = inputLabels[label_index];
                auto& part = inputLabel.part;
                // 1. Record output ID
                part.id = output2trackid.size();
                LOG.VERBOSE() << "   --> Assigned output id = " << part.id << "\n";

                // 2. Set merged particle track ID to also point to the output ID of its superset particle
                trackid2output[part.trackid] = part.id;
                for (auto const &child : inputLabel.merged_v)
                    trackid2output[child] = part.id;

                // 3. Record track ID => Output index mapping            
                output2trackid.push_back(inputLabel.part.trackid);

                // 4. Set the first and last step 
                auto const &first_pt = inputLabel.first_pt;
                auto const &last_pt = inputLabel.last_pt;
                if (first_pt.t != kINVALID_DOUBLE)
                    part.first_step = supera::Vertex(first_pt.x, first_pt.y, first_pt.z, first_pt.t);
                if (last_pt.t != kINVALID_DOUBLE)
                    part.last_step = supera::Vertex(last_pt.x, last_pt.y, last_pt.z, last_pt.t);
                LOG.VERBOSE() << "  true particle start: " << part.first_step.dump() << "\n"
                              << "                  end: " << part.last_step.dump() << "\n";
            }
        }

        // Set the parent IDs
        for (auto& inputLabel : inputLabels)
        {
            auto& part = inputLabel.part;
            part.parent_id = kINVALID_INSTANCEID;
            auto parent_index = this->InputIndex(part.parent_trackid);
            if(parent_index == kINVALID_INDEX) continue;
            part.parent_id = inputLabels[parent_index].part.id;
        }

        LOG.VERBOSE() << "trackid2output (i.e., map of track IDs to output IDs) contents:\n";
        for (std::size_t idx = 0; idx < trackid2output.size(); idx++)
            LOG.VERBOSE() << "   " << idx << " -> " << trackid2output[idx] << "\n";

    } // LArTPCMLReco3D::RegisterOutputParticles()

    // ------------------------------------------------------
    /*
    EventOutput LArTPCMLReco3D::BuildOutputLabels(std::vector<supera::ParticleLabel> &groupedInputLabels,
                                                  const std::vector<TrackID_t> &output2trackid,
                                                  const std::vector<int> &trackid2output,
                                                  const std::vector<TrackID_t> &trackid2index) const
    {
        EventOutput outputLabels;
        outputLabels.Particles().resize(output2trackid.size());
        
        for (size_t index = 0; index < output2trackid.size(); ++index)
        {
            TrackID_t trackid = output2trackid[index];
            auto &groupedInputLabel = groupedInputLabels[_mcpl.TrackIdToIndex()[trackid]];

            LOG.VERBOSE() << "Creating output cluster for group " << groupedInputLabel.part.id << " (" << groupedInputLabel.energy.size() << " voxels)\n";
            LOG.VERBOSE() << "Got here 1\n";
            // set semantic type
            supera::SemanticType_t semantic = groupedInputLabel.part.shape;
            if (semantic == kShapeUnknown)
            {
                LOG.FATAL() << "Unexpected type while assigning semantic class: " << groupedInputLabel.part.type << "\n";
                auto const &part = groupedInputLabel.part;
                LOG.FATAL() << "Particle ID " << part.id << " Type " << groupedInputLabel.part.type << " Valid " << groupedInputLabel.valid
                            << " Track ID " << part.trackid << " PDG " << part.pdg
                            << " " << part.process << " ... " << part.energy_init << " MeV => "
                            << part.energy_deposit << " MeV "
                            << groupedInputLabel.merged_v.size() << " children " << groupedInputLabel.energy.size() << " voxels " << groupedInputLabel.energy.sum()
                                 << " MeV\n";
                LOG.FATAL() << "  Parent " << part.parent_trackid << " PDG " << part.parent_pdg
                                 << " " << part.parent_process << " Ancestor " << part.ancestor_trackid
                                 << " PDG " << part.ancestor_pdg << " " << part.ancestor_process
                                 << "\n";


                throw std::exception();
            }
            // todo: how should this be fixed?  we don't have the LArSoft MCShowers
            //if(semantic == larcv::kShapeLEScatter && mcs_trackid_s.find(trackid) == mcs_trackid_s.end()) {
            //  LOG.FATAL() << "Unexpected particle to be stored with a shape kShapeLEScatter!" << std::endl;
            //  this->DumpHierarchy(grp.part.track_id(),part_grp_v);
            //  throw std::exception();
            //}

            // Now, we will re-classify some of non LEScatter showers based on pixel count
            // (BUG FIX: use pca or something better)
            if (groupedInputLabel.energy.size() < _compton_size)
            {
                LOG.DEBUG() << "Particle ID " << groupedInputLabel.part.id << " PDG " << groupedInputLabel.part.pdg << " "
                            << groupedInputLabel.part.process << "\n"
                            << "  ... type switching " << groupedInputLabel.part.shape << " => " << kShapeLEScatter
                            << " (voxel count " << groupedInputLabel.energy.size() << " < " << _compton_size << ")\n";
                semantic = kShapeLEScatter;
            }
            LOG.VERBOSE() << "Got here 2\n";
            // store the shape (semantic) type in particle
            groupedInputLabel.part.shape = semantic;
            // store the voxel count and energy deposit
            groupedInputLabel.part.energy_deposit = groupedInputLabel.energy.sum();

            // duplicate the particle to the output container
            outputLabels.Particles()[index] = groupedInputLabel;
            LOG.VERBOSE() << "Got here 3\n";

            // set the particle in the original container to 'invalid' so we don't accidentally use it again
            groupedInputLabel.valid = false;
            LOG.VERBOSE() << "Got here 4\n";
        } // for (index)
        LOG.VERBOSE() << "Got here 5\n";
        // now vacuum up any orphan particles into a top-level orphan particle
        // (anything that is still not grouped after all the cleanup steps).
        // the others *should* have gotten absorbed by the Merge() calls
        // in the various LArTPCMLReco3D::Merge...() methods...
        supera::ParticleLabel orphan;
        orphan.part.pdg = 0;
        LOG.VERBOSE() << "Got here 6\n";
        for (std::size_t trkid = 0; trkid < trackid2output.size(); trkid++)
        {
            LOG.VERBOSE() << "Got here 7\n";
            int outputIdx = trackid2output[trkid];
            LOG.VERBOSE() << "Got here 8\n";
            if (outputIdx >= 0)
                continue;
            LOG.VERBOSE() << "Got here 9" << trkid <<"   "<< trackid2index[trkid] <<"  " << groupedInputLabels.size() << "\n";
            orphan.Merge(groupedInputLabels[trackid2index[trkid]]);
            LOG.VERBOSE() << "Got here 10\n";
        } // for (idx)
        // only create an "orphan" particle if there was actually anything there
        LOG.VERBOSE() << "Got here last-1\n";
        if (orphan.merged_v.size() > 0)
          outputLabels.Particles().push_back(std::move(orphan));
        LOG.VERBOSE() << "Got here last\n";
        return outputLabels;
    } // LArTPCMLReco3D::BuildOutputClusters

*/
    // ------------------------------------------------------
    
    void LArTPCMLReco3D::DumpHierarchy(size_t trackid, const std::vector<supera::ParticleLabel>& inputLabels) const
    {
        assert(trackid < inputLabels.size());

        auto const &label = inputLabels[trackid];
        LOG.VERBOSE() << "\n#### Dumping particle record for track id "
                      << label.part.trackid << " ####\n";
        LOG.VERBOSE() << "id " << label.part.id << " from " << label.part.parent_id << "\n"
                      << "children: ";
        for (auto const &child : label.part.children_id)
            LOG.VERBOSE() <<  "   " << child;
        LOG.VERBOSE() << "\n" << label.part.dump() << "\n";

        size_t parent_trackid = label.part.parent_trackid;
        while (parent_trackid < inputLabels.size())
        {

            auto const &parent = inputLabels[parent_trackid];
            LOG.VERBOSE() << "Parent's group id: " << parent.part.group_id << " valid? " << parent.valid << "\n";
            LOG.VERBOSE() << "Parent's children: " ;
            for (auto const &child : parent.part.children_id)
                LOG.VERBOSE() << "    " << child;
            LOG.VERBOSE() << "\n" << parent.part.dump() << "\n";
            if (parent_trackid == parent.part.parent_trackid)
                break;
            if (parent_trackid == supera::kINVALID_TRACKID)
                break;
            parent_trackid = parent.part.parent_trackid;
        }
        LOG.VERBOSE() << "\n\n#### Dump done ####\n";
    } // LArTPCMLReco3D::DumpHierarchy()

    // ------------------------------------------------------

    void LArTPCMLReco3D::FixFirstStepInfo(std::vector<supera::ParticleLabel> &inputLabels,
                                          const supera::ImageMeta3D &meta,
                                          const std::vector<TrackID_t> &output2trackid)
    {
        for (std::size_t idx = 0; idx < output2trackid.size(); idx++)
        {
            auto &grp = inputLabels[idx];
            auto const &fs = grp.part.first_step;
            if (fs.pos.x != 0. || fs.pos.y != 0. || fs.pos.z != 0. || fs.time != 0.)
                continue;
            auto const vtx = grp.part.vtx.pos;
            double min_dist = fabs(kINVALID_DOUBLE);
            Point3D min_pt;
            for (auto const &vox : grp.energy.as_vector())
            {
                auto const pt = meta.position(vox.id());
                double dist = pt.squared_distance(vtx);
                if (dist > min_dist)
                    continue;
                min_dist = dist;
                min_pt = pt;
            }
            if (min_dist > (sqrt(3.) + 1.e-3))
                grp.part.first_step = {kINVALID_DOUBLE, kINVALID_DOUBLE, kINVALID_DOUBLE, kINVALID_DOUBLE};
            else
                grp.part.first_step = {min_pt.x, min_pt.y, min_pt.z, grp.part.vtx.time};

        }
    } // LArTPCMLReco3D::FixFirstStepInfo()

    // ------------------------------------------------------

    void LArTPCMLReco3D::FixInvalidParentShowerGroups(std::vector<supera::ParticleLabel> &inputLabels,
                                                      std::vector<TrackID_t> &output2trackid,
                                                      std::vector<int> &trackid2output) const
    {
        for (size_t out_index = 0; out_index < output2trackid.size(); ++out_index)
        {
            TrackID_t trackid = output2trackid[out_index];
            auto &grp = inputLabels[trackid];
            if (!grp.valid)
                continue;
            if (grp.part.parent_id != kINVALID_INSTANCEID)
                continue;
            if (grp.part.shape != kShapeShower)
                continue;
            LOG.DEBUG() << "Analyzing particle id " << out_index << " trackid " << trackid << "\n"
                        << grp.part.dump() << "\n";
            int parent_partid = -1;
            supera::TrackID_t parent_trackid;
            for (supera::TrackID_t idx : _mcpl.ParentTrackIdArray(grp.part.trackid))
            {
                parent_trackid = idx;
                if (trackid2output[parent_trackid] < 0 || !inputLabels[parent_trackid].valid)
                    continue;
                auto const &parent = inputLabels[parent_trackid].part;
                // shower parent can be either shower, michel, or delta
                if (parent.shape == kShapeMichel ||
                    parent.shape == kShapeDelta ||
                    parent.shape == kShapeShower)
                    parent_partid = static_cast<int>(parent.id);
                break;
            }
            /*
            int own_partid = grp.part.id;
            // initiate a search of parent in the valid output particle
            int parent_trackid = grp.part.parent_track_id();
            int parent_partid  = -1;
            while(1) {
          if(parent_trackid >= ((int)(trackid2index.size())) || trackid2index[parent_trackid] <0)
            break;
          if(parent_trackid < ((int)(trackid2output.size())) &&
             trackid2output[parent_trackid] >= 0 &&
             part_grp_v[parent_trackid].valid ) {
            //parent_partid = trackid2output[parent_trackid];
            parent_partid = part_grp_v[parent_trackid].part.id;
            break;
          }
          parent_trackid = larmcp_v[trackid2index[parent_trackid]].Mother();
            }
            */
            if (parent_partid >= 0)
            {
                // assert the group is same
                auto &parent = inputLabels[output2trackid[parent_partid]];
                if (grp.part.group_id == kINVALID_INSTANCEID)
                {
                    grp.part.group_id = parent.part.group_id;
                    for (auto const &child_id : grp.part.children_id)
                    {
                        auto &child = inputLabels[output2trackid[child_id]];
                        child.part.group_id = parent.part.group_id;
                    }
                }
                else
                {
                    assert(grp.part.group_id == inputLabels[output2trackid[parent_partid]].part.group_id);
                }
                grp.part.parent_id = parent_partid;
                inputLabels[parent_trackid].part.children_id.push_back(grp.part.id);
                LOG.DEBUG() << "PartID " << grp.part.id << " (output index " << out_index << ") assigning parent "
                            << parent_partid << "\n";
            }
            else
            {
                grp.part.parent_id = grp.part.id;
                if (grp.part.group_id == kINVALID_INSTANCEID)
                    grp.part.group_id = grp.part.id;
                for (auto const &child_id : grp.part.children_id)
                {
                    auto &child = inputLabels[output2trackid[child_id]];
                    child.part.group_id = grp.part.id;
                }
                LOG.DEBUG() << "PartID " << grp.part.id << " (output index " << out_index
                            << ") assigning itself as a parent...\n";
            } // else (original if: (parent_partid >= 0))
        } // for (out_index)
    } // LArTPCMLReco3D::FixInvalidParentShowerGroups

    // ------------------------------------------------------

    void LArTPCMLReco3D::FixOrphanNonShowerGroups(std::vector<supera::ParticleLabel> &inputLabels,
                                                  const std::vector<TrackID_t> &output2trackid,
                                                  std::vector<int> &trackid2output) const
    {
        LOG.VERBOSE() << "Examining outputs to find orphaned non-shower groups...\n";
        for (size_t out_index = 0; out_index < output2trackid.size(); ++out_index)
        {
            TrackID_t trackid = output2trackid[out_index];
            auto &grp = inputLabels[trackid];
            // these were fixed in FixOrphanShowerGroups()
            if (grp.part.shape == kShapeShower)
                continue;
            if (grp.part.group_id != kINVALID_INSTANCEID)
            {
                LOG.VERBOSE() << "  group for trackid " << trackid << "  has group id " << grp.part.group_id << " already.  Skipping\n";
                continue;
            }
            LOG.DEBUG() << " #### Non-shower ROOT SEARCH #### \n"
                         << " Analyzing a particle index " << out_index << " id " << grp.part.id << "\n" << grp.part.dump() << "\n";

            std::stringstream ss;
            ss << "   candidate ancestor track IDs:";
            for (const auto & trkid : _mcpl.ParentTrackIdArray(grp.part.trackid))
                ss << " " << trkid;
            LOG.VERBOSE() << ss.str() << "\n";
            size_t group_id = kINVALID_INSTANCEID;
            bool stop = false;
            for (auto const &parent_trackid : _mcpl.ParentTrackIdArray(grp.part.trackid))
            {
                auto const &parent = inputLabels[parent_trackid];
                LOG.VERBOSE() << "     considering ancestor: " << parent_trackid
                              << ", which has output index " << trackid2output[parent_trackid] << ":\n"
                              << parent.part.dump() << "\n";
                if (parent.part.pdg == 0)
                {
                    LOG.VERBOSE() << "      --> particle was removed from output (maybe a nuclear fragment?), keep looking\n";
                    continue;
                }
                switch (parent.part.shape)
                {
                    case kShapeShower:
                    case kShapeMichel:
                    case kShapeDelta:
                    case kShapeTrack:
                    case kShapeLEScatter:
                        // group candidate: check if it is "valid" = exists in the output
                        if (parent.valid && trackid2output[parent_trackid] >= 0)
                        {
                            LOG.VERBOSE() << "      -->  accepted\n";
                            group_id = trackid2output[parent_trackid];
                            // found the valid group: stop the loop
                            stop = true;
                        }
                        break;
                    case kShapeUnknown:
                    case kShapeGhost:
                        LOG.FATAL() << "Unexpected type found while searching for non-shower orphans's root!\n";
                        throw std::exception();
                        break;
                }
                if (stop)
                    break;
            }
            if (group_id == kINVALID_INSTANCEID)
            {
                LOG.DEBUG() << "Ignoring non-shower particle as its root particle (for group id) is not to be stored...\n"
                             << grp.part.dump() << "\n";
                continue;
            }
            LOG.DEBUG() << "Assigning a group ID " << group_id << " to non-shower orphan\n"
                         << "  Track ID " << grp.part.trackid << " PDG " << grp.part.pdg
                         << " " << grp.part.process << "\n";
            grp.part.group_id = group_id;
            // todo: is this correct?  if we don't, the parent_id points to a nonexistent particle group...
            grp.part.parent_id = group_id;

            trackid2output[trackid] = static_cast<int>(group_id);
        }
    } // LArTPCMLReco3D::FixOrphanNonShowerGroups()


    // ------------------------------------------------------

    void LArTPCMLReco3D::FixOrphanShowerGroups(std::vector<supera::ParticleLabel> &inputLabels,
                                               std::vector<supera::TrackID_t> &output2trackid,
                                               std::vector<int> &trackid2output) const
    {
        for (size_t out_index = 0; out_index < output2trackid.size(); ++out_index)
        {

            supera::TrackID_t trackid = output2trackid[out_index];
            auto &label = inputLabels[trackid];
            if (!label.valid)
                continue;
            if (label.part.group_id != kINVALID_INSTANCEID)
                continue;
            if (label.part.shape != kShapeShower)
                continue;
            LOG.DEBUG() << " #### SHOWER ROOT SEARCH: Analyzing a particle index " << out_index
                        << " track id " << label.part.trackid << "\n"
                        << label.part.dump()
                        << "      group type = " << label.part.type << "\n"
                        << "      group shape = " << label.part.shape << "\n"
                        << "      group is valid = " << label.valid << "\n"
                         << "      group is mapped to output index = " << trackid2output[trackid];

            std::stringstream ss;
            ss << "   candidate ancestor track IDs:";
            for (const auto & trkid : _mcpl.ParentTrackIdArray(label.part.trackid))
                ss << " " << trkid;
            LOG.VERBOSE() << "       " << ss.str();
            supera::TrackID_t root_id = label.part.id;
            supera::TrackID_t root_trackid = label.part.trackid;
            bool stop = false;
            std::vector<size_t> intermediate_trackid_v;
            intermediate_trackid_v.push_back(trackid);
            for (auto const &parent_trackid : _mcpl.ParentTrackIdArray(label.part.trackid))
            {
                auto const &parent = inputLabels[parent_trackid];
                LOG.VERBOSE() << "  ancestor track id " << parent_trackid << "\n"
                              << parent.part.dump()
                              << "      group type = " << parent.part.type << "\n"
                              << "      group shape = " << parent.part.shape << "\n"
                              << "      group is valid = " << parent.valid << "\n"
                              << "      group is mapped to output index = " << trackid2output[parent_trackid];

                switch (parent.part.shape)
                {
                    case kShapeShower:
                    case kShapeMichel:
                    case kShapeDelta:
                        // group candidate: check if it is "valid" = exists in the output
                        if (trackid2output[parent_trackid] >= 0 && parent.valid)
                        {
                            root_trackid = parent_trackid;
                            root_id = trackid2output[root_trackid];
                            // found the valid group: stop the loop
                            LOG.VERBOSE() << " found root ancestor: trkid " << root_trackid << " (particle id " << root_id << ")\n";
                            stop = true;
                            // If not, root_id will be a new output index
                        }
                        else
                        {
//              root_id = output2trackid.size();
                            LOG.VERBOSE() << "  ancestor trkid " << parent_trackid << " is also not in output.  keep looking...\n";
                            // If this particle is invalid, this also needs the group id.
                            // Add to intermediate_id_v list so we can set the group id for all of them
                            intermediate_trackid_v.push_back(root_trackid);
                        }
                        stop = (stop || parent.part.shape != kShapeShower);
                        break;
                    case kShapeTrack:
                        LOG.VERBOSE() << "  ancestor group is a 'track' shape.  Stop looking...\n";
                        stop = true;
                        break;
                    case kShapeUnknown:
                        LOG.VERBOSE() << "  ancestor group is unknown shape.  Stop looking... \n";
                        stop = true;
                        break;
                    case kShapeLEScatter:
                    case kShapeGhost:
                        /*
                        LOG.FATAL() << "Unexpected type found while searching for kShapeShower orphans's root!" << std::endl;
                        this->DumpHierarchy(trackid,part_grp_v);
                        throw std::exception();
                        */
                        break;
                }
                if (stop)
                    break;
            }
            LOG.VERBOSE() << " found root ancestor: trkid " << root_trackid << " (particle id()=" << root_id << ")\n";
            if (root_id < output2trackid.size() && trackid2output[root_trackid] != (int) (root_id))
            {
                LOG.FATAL() << "Logic error for the search of shower root particle for an orphan..." << "\n"
                            << "This particle id=" << out_index << " and track_id=" << trackid << "\n"
                            << "ROOT particle id=" << root_id << " and track_id=" << root_trackid
                            << "\n";
                DumpHierarchy(trackid, inputLabels);
                throw std::exception();
            }

            if (output2trackid.size() <= root_id)
            {
                output2trackid.push_back(root_trackid);
                // Register the root parent to the output
                LOG.DEBUG() << "Adding a new particle to the output to define a group...\n"
                             << "ROOT particle id=" << root_id << " and track_id=" << root_trackid << "\n"
                             << inputLabels[root_trackid].part.dump() << "\n";
            }
            assert((size_t) (root_id) < output2trackid.size());

            auto &root = inputLabels[root_trackid];
            //root.valid = true;
            assert(root.valid);
            root.part.id = root_id;
            root.part.group_id = root_id;
            trackid2output[root_trackid] = static_cast<int>(root_id);
            LOG.VERBOSE() << "Updating group " << root.part.group_id << "'s child groups to have the correct root group id...\n";
            for (auto const &child_id : root.part.children_id)
            {
                auto &child = inputLabels[output2trackid[child_id]];
                if (child.valid)
                    continue;
                LOG.VERBOSE() << "   group for trackid " << child.part.trackid << " had group: " << child.part.group_id << "\n";
                assert(child.part.group_id == kINVALID_INSTANCEID || child.part.group_id == root_id);
                child.part.group_id = root_id;
            }
            // Set the group ID for THIS + intermediate particles
            for (auto const &child_trackid : intermediate_trackid_v)
            {
                auto &child = inputLabels[child_trackid];
                if (!child.valid)
                    continue;
                assert(child.part.group_id == kINVALID_INSTANCEID || child.part.group_id == root_id);
                child.part.group_id = root_id;

                // todo: is this correct?  if we don't, the parent_id points to a nonexistent particle group...
                child.part.parent_id = root_id;
            }

            LOG.DEBUG() << "... after update ... \n" << inputLabels[trackid].part.dump() << "\n";
        }
    } // LArTPCMLReco3D::FixOrphanShowerGroups()

    // ------------------------------------------------------
    
    void LArTPCMLReco3D::FixUnassignedGroups(std::vector<supera::ParticleLabel> &inputLabels,
                                             std::vector<TrackID_t> &output2trackid) const
    {
        for (size_t output_index = 0; output_index < output2trackid.size(); ++output_index)
        {
            auto &label = inputLabels[output2trackid[output_index]];
            if (label.part.group_id != kINVALID_INSTANCEID)
                continue;
            auto shape = label.part.shape;
            auto parent_shape = kShapeUnknown;
            auto parent_partid = label.part.parent_id;
            //auto parent_groupid = larcv::kINVALID_INSTANCEID;
            // If delta, its own grouping

            switch (shape)
            {
                case kShapeLEScatter:
                    // if LEScatter, we handle later (next loop)
                    break;
                case kShapeDelta:
                case kShapeMichel:
                case kShapeTrack:
                    // If delta, Michel, or track, it's own group
                    label.part.group_id = output_index;
                    for (auto const &child_index : label.part.children_id)
                    {
                        inputLabels[output2trackid[child_index]].part.group_id = output_index;
                    }
                    break;

                case kShapeShower:
                    // If shower && no parent, consider it as a primary = assign group id for all children
                    if (parent_partid == kINVALID_INSTANCEID)
                    {
                        label.part.group_id = output_index;
                        for (auto const &child_index : label.part.children_id)
                            inputLabels[output2trackid[child_index]].part.group_id = output_index;
                        continue;
                    }
                    parent_shape = inputLabels[output2trackid[parent_partid]].part.shape;
                    switch (parent_shape)
                    {
                        case kShapeMichel:
                        case kShapeDelta:
                            label.part.group_id = parent_partid;
                            for (auto const &child_index : label.part.children_id)
                            {
                                inputLabels[output2trackid[child_index]].part.group_id = parent_partid;
                            }
                            break;
                        case kShapeTrack:
                            label.part.group_id = output_index;
                            for (auto const &child_index : label.part.children_id)
                            {
                                inputLabels[output2trackid[child_index]].part.group_id = output_index;
                            }
                            break;
                        case kShapeShower:
                            LOG.FATAL() << "Unexpected case: a shower has no group id while being a child of another shower...\n";
                            DumpHierarchy(label.part.trackid, inputLabels);
                            throw std::exception();
                            /*
                            // COMMENTED OUT as this is no longer expected
                            parent_groupid = part_grp_v[output2trackid[parent_partid]].part.group_id();
                            if(parent_groupid != larcv::kINVALID_INSTANCEID) {
                              grp.part.group_id(parent_groupid);
                              for(auto const& child_index : grp.part.children_id()) {
                                part_grp_v[output2trackid[child_index]].part.group_id(parent_groupid);
                              }
                            }
                            */
                            break;
                        case kShapeLEScatter:
                            LOG.FATAL() << "Logic error: shower parent shape cannot be LEScatter!\n";
                            throw std::exception();
                        default:
                            LOG.FATAL() << "Unexpected larcv::ShapeType_t encountered at " << __LINE__ << "\n";
                            throw std::exception();
                    } // switch (parent_shape)
                    break;
                case kShapeGhost:
                case kShapeUnknown:
                    LOG.FATAL() << "Unexpected larcv::ShapeType_t encountered at " << __LINE__ << "\n";
                    throw std::exception();
            } // switch(shape)
        } // for (output_index)
    } // LArTPCMLReco3D::FixUnassignedGroups()

    // ------------------------------------------------------

    void LArTPCMLReco3D::FixUnassignedLEScatterGroups(std::vector<supera::ParticleLabel> &inputLabels,
                                                               const std::vector<TrackID_t> & /*output2trackid*/) const
    {
        LOG.DEBUG() << "Inspecting LEScatter groups for invalid group ids...\n";
        for (auto & label : inputLabels)
//    for (size_t output_index = 0; output_index < output2trackid.size(); ++output_index)
        {
//      auto &grp = part_grp_v[output2trackid[output_index]];
            if (label.part.shape != kShapeLEScatter)
                continue;

            LOG.VERBOSE() << "   trackid=" << StringifyTrackID(label.part.trackid) << " group=" << StringifyInstanceID(label.part.group_id) << "\n";
            if (label.part.group_id != kINVALID_INSTANCEID)
            {
                LOG.VERBOSE() << "     --> group is valid; don't update.\n";
                continue;
            }

            // assign parent's group, otherwise leave as is = kINVALID_INSTANCEID
            auto parent_partid = label.part.parent_id;
            if (parent_partid == kINVALID_INSTANCEID)
            {
                LOG.VERBOSE() << "     --> invalid, but parent also has invalid parent id??  Can't fix...\n";
                continue;
            }

            // todo: I think there's a more efficient way to find this using
            //       one of the intermediate vectors, but I can't work it out at the moment
            auto parent_part = *std::find_if(inputLabels.begin(), inputLabels.end(),
                                             [&](const supera::ParticleLabel & searchGrp)
                                             {
                                                 return searchGrp.part.trackid == label.part.parent_trackid;
                                             });
            if (parent_part.part.group_id != supera::kINVALID_INSTANCEID)
            {
                LOG.VERBOSE() << "     --> rewrote group id to parent (trackid=" << StringifyTrackID(parent_part.part.trackid)
                              << ")'s group id = " << StringifyInstanceID(parent_part.part.group_id) << "\n";
                label.part.group_id = parent_part.part.group_id;
            }
            else
            {
                LOG.VERBOSE() << "     --> no valid parent.  Rewrote group id to its own particle ID ("
                              << label.part.id << ")\n";
                label.part.group_id = label.part.id;
            }
        } // for (label)
    } // LArTPCMLReco3D::FixUnassignedLEScatterGroups

    // ------------------------------------------------------

    void LArTPCMLReco3D::FixUnassignedParentGroups(std::vector<supera::ParticleLabel> &inputLabels,
                                                   std::vector<TrackID_t> &output2trackid,
                                                   std::vector<int> &trackid2output) const
    {
        LOG.DEBUG() << "Inspecting parent groups for unassigned entries...\n";
        for (TrackID_t output_index : output2trackid)
        {
            auto &grp = inputLabels[output_index];
            auto parent_trackid = grp.part.parent_trackid;
            auto parent_id = grp.part.parent_id;
            LOG.VERBOSE() << "  index=" << output_index
                          << "  id=" << StringifyInstanceID(grp.part.id)
                          << "  track id=" << StringifyTrackID(grp.part.trackid)
                          << "  parent trackid=" << StringifyTrackID(parent_trackid)
                          << "  parent id=" << StringifyInstanceID(parent_id)
                          << "\n";

            auto &parent = inputLabels[parent_trackid].part;
            // if parent_id is invalid, try if parent_trackid can help out
            if (parent_id == supera::kINVALID_INSTANCEID &&
                parent_trackid != supera::kINVALID_TRACKID &&
                trackid2output[parent_trackid] >= 0)
            {
                parent_id = trackid2output[parent_trackid];
                grp.part.parent_id = parent_id;
            }
            // note that for shower types, the parent_id was already reset to be the same as the id in FixInvalidParentShowerGroups()
            if (parent_id == kINVALID_INSTANCEID || parent_id == grp.part.id)
                continue;
            // if parent id is set, make sure this particle is in the children
            auto children = parent.children_id;
            bool add = true;
            for (auto const &child : children)
            {
                if (child != grp.part.id)
                    continue;
                add = false;
                break;
            }
            if (add)
            {
                children.push_back(grp.part.id);
                parent.children_id = children;
            }
        } // for (output_index)
    } // LArTPCMLReco3D::FixUnassignedParentGroups()

    // ------------------------------------------------------

    std::vector<supera::ParticleLabel>
    LArTPCMLReco3D::InitializeLabels(const EventInput &evtInput, const supera::ImageMeta3D &meta) const
    {
        // this default-constructs the whole lot of them, which fills their values with defaults/invalid values
        std::vector<supera::ParticleLabel> labels(evtInput.size());

        LOG.DEBUG() << "Initializing labels with incoming particles...\n";
        for (std::size_t idx = 0; idx < evtInput.size(); idx++)
        {
            auto& label = labels[idx];
            label.part  = evtInput[idx].part;
            label.part.parent_pdg = _mcpl.ParentPdgCode()[idx];

            if(label.part.parent_pdg != supera::kINVALID_PDG)
                label.valid = true;

            for (const supera::EDep & edep : evtInput[idx].pcloud)
            {
                auto vox_id = meta.id(edep);
                if(vox_id == supera::kINVALID_VOXELID || !_world_bounds.contains(edep)) {
                    LOG.VERBOSE() << "Skipping EDep from track ID " << label.part.trackid
                    << " E=" << edep.e
                    << " pos=" << edep.x << "," << edep.y << "," << edep.z << ")\n";
                    continue;
                }

                label.energy.emplace (vox_id, edep.e,    true);
                label.dedx.emplace   (vox_id, edep.dedx, true);
                label.AddEDep(edep);
            }

            LOG.VERBOSE() << label.dump() << "\n";

        }  // for (idx)

        return labels;
    }  // LArTPCMLReco3D::InitializeLabels()


    // ------------------------------------------------------

    void LArTPCMLReco3D::MergeShowerConversion(std::vector<supera::ParticleLabel>& labels) const
    {
        int merge_ctr = 0;
        int invalid_ctr = 0;
        do
        {
            merge_ctr = 0;
            for (auto &label : labels)
            {
                if (!label.valid) continue;
                //if(grp.part.type != supera::kIonization && grp.part.type != supera::kConversion) continue;
                if (label.part.type != supera::kConversion) continue;
                // merge to a valid "parent"
                bool parent_found = false;
                auto parent_trackid = label.part.parent_trackid;
                auto parent_trackid_before = label.part.trackid;
                while (true)
                {
                    LOG.DEBUG() << "Inspecting: trackid " << StringifyTrackID(label.part.trackid) << " => parent trackid " << StringifyTrackID(parent_trackid) << "\n";
                    auto const& parent_index = this->InputIndex(parent_trackid);
                    if (parent_index == supera::kINVALID_INDEX)
                    {
                        LOG.VERBOSE() << "Missing parent track id " << StringifyTrackID(parent_trackid)
                                      << " Could not find a parent for trackid " << StringifyTrackID(label.part.trackid) << " PDG " << label.part.pdg
                                      << " " << label.part.process << " E = " << label.part.energy_init
                                      << " (" << label.part.energy_deposit << ") MeV\n";
                        if(parent_trackid_before != supera::kINVALID_TRACKID)
                        {
                            LOG.VERBOSE() << "Previous parent trackid: " << StringifyTrackID(parent_trackid_before) << "\n";
                        }
                        parent_found = false;
                        invalid_ctr++;
                        break;
                        //throw std::exception();
                    }

                    auto const &parent = labels[parent_index];
                    parent_found = parent.valid;
                    if (parent_found) break;
                    else
                    {
                        auto ancestor_trackid = parent.part.parent_trackid;
                        if (ancestor_trackid == parent_trackid)
                        {
                            LOG.INFO() << "Trackid " << StringifyTrackID(parent_trackid) << " is root and invalid particle...\n";
                            LOG.INFO() << "PDG " << parent.part.pdg << " " << parent.part.process << "\n";
                            break;
                        }
                      parent_trackid_before = parent_trackid;
                      parent_trackid = ancestor_trackid;
                    }
                }
                // if parent is found, merge
                if (parent_found)
                {
                    this->MergeParticleLabel(labels,parent_trackid,label.part.trackid);
                    merge_ctr++;
                }
            }
            LOG.INFO() << "Merge counter: " << merge_ctr << " invalid counter: " << invalid_ctr << "\n";
        } while (merge_ctr > 0);
    }  // LArTPCMLReco3D::MergeShowerConversion()

    // ------------------------------------------------------
    
    void LArTPCMLReco3D::MergeDeltas(std::vector<supera::ParticleLabel>& labels) const
    {
        for (auto &label : labels)
        {
            //if(label.part.type != supera::kDelta) continue;
            if (label.part.shape != supera::kShapeDelta) continue;
            auto parent_trackid = label.part.parent_trackid;
            auto parent_index   = this->InputIndex(parent_trackid);
            if(parent_index == kINVALID_INDEX) continue;
            auto &parent = labels[parent_index];
            if (!parent.valid) continue;

            // allows the test on unique voxels to be put in the if() below
            // and only used if needed due to short-circuiting.
            const auto UniqueVoxelCount = [](const supera::ParticleLabel & grp, const supera::ParticleLabel & parent)
            {
                size_t unique_voxel_count = 0;
                for (auto const &vox : grp.energy.as_vector())
                {
                    if (parent.energy.find(vox.id()).id() == supera::kINVALID_VOXELID)
                        ++unique_voxel_count;
                }
                return unique_voxel_count;
            };

            // if voxel count is smaller than delta ray requirement, simply merge
            if (label.energy.size() < _delta_size || UniqueVoxelCount(label, parent) < _delta_size)
            {
                // if parent is found, merge
                LOG.INFO() << "Merging delta trackid " << StringifyTrackID(label.part.trackid) << " PDG " << label.part.pdg
                            << " " << label.part.process << " vox count " << label.energy.size() 
                            << " (unique " << UniqueVoxelCount(label, parent) << ")\n"
                            << " ... parent found " << parent.part.trackid
                            << " PDG " << parent.part.pdg << " " << parent.part.process << "\n";
                LOG.INFO() << "Time difference: " << label.part.first_step.time - parent.part.first_step.time << "\n";
                this->MergeParticleLabel(labels, parent.part.trackid, label.part.trackid);
            }
            else
            {
                LOG.INFO() << "NOT merging delta " << StringifyTrackID(label.part.trackid) << " PDG " << label.part.pdg
                            << " " << label.part.process << " vox count " << label.energy.size() << "\n"
                            <<" ... parent found " << parent.part.trackid
                            << " PDG " << parent.part.pdg << " " << parent.part.process << "\n";

            }
        }
    } // LArTPCMLReco3D::MergeShowerDeltas()
    // ------------------------------------------------------

    void LArTPCMLReco3D::MergeShowerFamilyTouching(const supera::ImageMeta3D& meta,
                                                   std::vector<supera::ParticleLabel>& labels) const
    {
        // Merge touching shower fragments
        // Direct parentage between kShapeShower => kShapeShower/kShapeDelta/kShapeMichel
        int merge_ctr = 0;
        int invalid_ctr = 0;
        do {
            merge_ctr = 0;
            for (auto& label : labels) {
                if (!label.valid) continue;
                if (label.part.shape != supera::kShapeShower) continue;
                if (label.part.parent_trackid == supera::kINVALID_TRACKID) continue;  // primaries can't have parents
                // search for a possible parent
                auto parent_trackid = kINVALID_TRACKID;
                LOG.VERBOSE() << "   Found particle group with shape 'shower', PDG=" << label.part.pdg
                              << "\n    track id=" << StringifyTrackID(label.part.trackid)
                              << ", and alleged parent track id=" << StringifyTrackID(label.part.parent_trackid) << "\n";
                // a direct parent ?
                auto parent_index = this->InputIndex(label.part.parent_trackid);
                if (parent_index != kINVALID_INDEX && labels[parent_index].valid)
                    parent_trackid = label.part.parent_trackid;
                else
                {
                    for (size_t shower_index = 0; shower_index < labels.size(); ++shower_index)
                    {
                        auto const &candidate_grp = labels[shower_index];
                        if (candidate_grp.part.trackid == label.part.parent_trackid || !candidate_grp.valid)
                            continue;
                        for (auto const &trackid : candidate_grp.merged_v)
                        {
                            if (trackid != label.part.parent_trackid)
                                continue;
                            parent_trackid = static_cast<TrackID_t>(candidate_grp.part.trackid);
                            break;
                        }
                        if (parent_trackid != kINVALID_TRACKID)
                            break;
                    }
                }
                if (parent_trackid == kINVALID_TRACKID || parent_trackid == label.part.trackid) continue;
                parent_index = this->InputIndex(parent_trackid);
                if(parent_index == kINVALID_INDEX) continue;
                auto& parent = labels[parent_index];
                //auto parent_type = labels[parent_trackid].part.type;
                //if(parent_type == supera::kTrack || parent_type == supera::kNeutron) continue;
                if (parent.part.shape != supera::kShapeShower && 
                    parent.part.shape != supera::kShapeDelta && 
                    parent.part.shape != supera::kShapeMichel)
                    continue;
                if (this->IsTouching(meta, label.energy, parent.energy)) {
                    // if parent is found, merge
                    this->MergeParticleLabel(labels, parent_trackid, label.part.trackid);
                    LOG.VERBOSE() << "   Merged to group w/ track id=" << StringifyTrackID(parent.part.trackid) << "\n";
                    merge_ctr++;
                }
            }
            LOG.DEBUG() << "Merge counter: " << merge_ctr << " invalid counter: " << invalid_ctr << "\n";
        } while (merge_ctr>0);
    } // LArTPCMLReco3D::MergeShowerFamilyTouching()


    // ------------------------------------------------------

    void LArTPCMLReco3D::MergeShowerIonizations(std::vector<supera::ParticleLabel>& labels) const
    {
        // Loop over particles of a type kIonization (=touching to its parent physically by definition)
        // If a parent is found, merge to the parent
        int merge_ctr = 0;
        int invalid_ctr = 0;
        do
        {
            merge_ctr = 0;
            for (auto &label : labels)
            {
                if (!label.valid) continue;
                if (label.part.type != supera::kIonization) continue;
                // merge to a valid "parent"
                /*
                bool parent_found          = false;
                auto parent_trackid        = label.part.parent_trackid;
                auto parent_trackid_before = label.part.trackid;
                while (true)
                {
                    //std::cout<< "Inspecting: " << StringifyTrackID(label.part.trackid) << " => " << parent_index << std::endl;
                    if (parent_trackid == supera::kINVALID_TRACKID)
                    {
                        LOG.ERROR() << "Invalid parent track id " << parent_trackid
                                    << " Could not find a parent for trackid " << StringifyTrackID(label.part.trackid) << " PDG " << label.part.pdg
                                    << " " << label.part.process << " E = " << label.part.energy_init
                                    << " (" << label.part.energy_deposit << ") MeV\n";

                        auto const &parent = labels[this->InputIndex(parent_trackid_before)].part;
                        std::cout << "Previous parent: trackid " << StringifyTrackID(parent.trackid) << " PDG " << parent.pdg
                                      << " " << parent.process << "\n";
                        parent_found = false;
                        invalid_ctr++;
                        break;
                    }
                    auto const &parent = labels[this->InputIndex(parent_trackid)];
                    parent_found = parent.valid;
                    if (parent_found) break;
                    else
                    {
                        auto ancestor_trackid = parent.part.parent_trackid;
                        if (ancestor_trackid == parent_trackid)
                        {
                            LOG.INFO() << "Particle w/ trackid " << StringifyTrackID(parent_trackid) << " is root and invalid particle...\n"
                                       << "PDG " << parent.part.pdg << " " << parent.part.process << "\n";
                            break;
                        }
                      parent_trackid_before = parent_trackid;
                      parent_trackid = ancestor_trackid;
                    }
                }
                */
                bool parent_found = false;
                auto parent_trackid = kINVALID_TRACKID;
                for(auto const& trackid : _mcpl.ParentTrackIdArray(label.part.trackid))
                {
                    parent_trackid = trackid;
                    auto parent_index = this->InputIndex(parent_trackid);
                    if(parent_index == kINVALID_INDEX) continue;
                    if(!labels[parent_index].valid) continue;
                    parent_found = true;
                    break;
                }
                // if parent is found, merge
                if (parent_found)
                {
                    this->MergeParticleLabel(labels,parent_trackid,label.part.trackid); 
                    merge_ctr++;
                }
            } // for (grp)
            LOG.DEBUG() << "Ionization merge counter: " << merge_ctr << " invalid counter: " << invalid_ctr << "\n";
        } while (merge_ctr > 0);
    } // LArTPCMLReco3D::MergeShowerIonizations()


    // ------------------------------------------------------
    void LArTPCMLReco3D::MergeShowerTouching(const supera::ImageMeta3D& meta,
                                             std::vector<supera::ParticleLabel>& labels) const
    {
        // Go over all pair-wise combination of two shower instances
        // For each shower, find all consecutive parents of shower/michel/delta type (break if track found)
        // If there is a common parent in two list AND if two showers are physically touching, merge
        int merge_ctr = 0;
        do
        {
            merge_ctr = 0;
            for (size_t i = 0; i < labels.size(); ++i)
            {
                auto &label_a = labels[i];
                if (!label_a.valid) continue;
                if (label_a.part.shape != supera::kShapeShower) continue;
                for (size_t j = 0; j < labels.size(); ++j)
                {
                    if (i == j) continue;
                    auto &label_b = labels[j];
                    if (!label_b.valid) continue;
                    if (label_b.part.shape != supera::kShapeShower) continue;

                    // check if these showers share the parentage
                    // list a's parents
                    std::set<size_t> parent_list_a;
                    std::set<size_t> parent_list_b;

                    auto parents_a = this->ParentShowerTrackIDs(label_a.part.trackid, labels);
                    for (auto const &parent_trackid : parents_a) parent_list_a.insert(parent_trackid);
                    parent_list_a.insert(label_a.part.trackid);

                    auto parents_b = this->ParentShowerTrackIDs(label_b.part.trackid, labels);
                    for (auto const &parent_trackid : parents_b) parent_list_b.insert(parent_trackid);
                    parent_list_b.insert(label_b.part.trackid);

                    bool same_family = false;
                    for (auto const &parent_trackid : parent_list_a)
                    {
                        if (parent_list_b.find(parent_trackid) != parent_list_b.end())
                            same_family = true;
                        if (same_family) break;
                    }
                    for (auto const &parent_trackid : parent_list_b)
                    {
                        if (parent_list_a.find(parent_trackid) != parent_list_a.end())
                            same_family = true;
                        if (same_family) break;
                    }

                    if (same_family && this->IsTouching(meta, label_a.energy, label_b.energy))
                    {
                        if (label_a.energy.size() < label_b.energy.size())
                            std::swap(label_a,label_b);
                        this->MergeParticleLabel(labels, label_a.part.trackid, label_b.part.trackid);
                        merge_ctr++;
                    }
                }
            }
            LOG.INFO() << "Merge counter: " << merge_ctr << "\n";
        } while (merge_ctr > 0);
    } // LArTPCMLReco3D::MergeShowerTouching()

    // ------------------------------------------------------

    void LArTPCMLReco3D::MergeShowerTouchingElectron(const supera::ImageMeta3D& meta,
                                                      std::vector<supera::ParticleLabel>& labels) const
    {
        size_t merge_ctr = 1;
        while (merge_ctr)
        {
            merge_ctr = 0;
            for (auto &label : labels)
            {
                //if (!label.valid || label.energy.size() < 1 || label.shape() != supera::kShapeLEScatter) continue;
                if( !label.valid || label.energy.size()<1 || 
                    label.energy.size()>_compton_size ||
                    std::abs(label.part.pdg) != 11)
                    continue;
                if( label.part.type != kPhotoElectron && 
                    label.part.type != kIonization && 
                    label.part.type != kCompton &&
                    label.part.type != kConversion)
                    continue;

                auto const &parents = _mcpl.ParentTrackIdArray(label.part.trackid);

                LOG.VERBOSE() << "Inspecting LEScatter Track ID " << StringifyTrackID(label.part.trackid)
                            << " PDG " << label.part.pdg
                            << " " << label.part.process << "\n";
                LOG.VERBOSE() << "  ... parents:\n";
                for(auto const& parent_trackid : parents)
                    LOG.VERBOSE() << "     "<< StringifyTrackID(parent_trackid) << "\n";

                for (auto const &parent_trackid : parents)
                {
                    auto parent_index = this->InputIndex(parent_trackid);
                    if(parent_index == kINVALID_INDEX) continue;
                    auto &parent = labels[parent_index];
                    if (!parent.valid || parent.energy.size() < 1) continue;
                    if (this->IsTouching(meta, label.energy, parent.energy))
                    {
                        LOG.VERBOSE() << "Merging LEScatter track id = " << StringifyTrackID(label.part.trackid)
                                    << " into touching parent shower group (id=" << StringifyInstanceID(parent.part.group_id) << ")"
                                    << " with track id = " << StringifyTrackID(parent.part.trackid) << "\n";
                        this->MergeParticleLabel(labels,parent_trackid,label.part.trackid);
                        merge_ctr++;
                        break;
                    }
                } // for (parent_trackid)
            } // for (grp)
        } // while (merge_ctr)
    } // LArTPCMLReco3D::MergeShowerTouchingElectron()

    // ------------------------------------------------------

    void LArTPCMLReco3D::MergeShowerTouchingLEScatter(const supera::ImageMeta3D& meta,
                                                      std::vector<supera::ParticleLabel>& labels) const
    {
        size_t merge_ctr = 1;
        while (merge_ctr)
        {
            merge_ctr = 0;
            for (auto &label : labels)
            {
                //if (!label.valid || label.energy.size() < 1 || label.shape() != supera::kShapeLEScatter) continue;
                if( !label.valid || label.energy.size()<1 || 
                    label.energy.size()>_lescatter_size ||
                    label.part.shape != supera::kShapeLEScatter)
                    continue;

                auto const &parents = _mcpl.ParentTrackIdArray(label.part.trackid);

                LOG.VERBOSE() << "Inspecting LEScatter Track ID " << StringifyTrackID(label.part.trackid)
                            << " PDG " << label.part.pdg
                            << " " << label.part.process << "\n";
                LOG.VERBOSE() << "  ... parents:\n";
                for(auto const& parent_trackid : parents)
                    LOG.VERBOSE() << "     "<< StringifyTrackID(parent_trackid) << "\n";

                for(auto &dest : labels) {
                    if(!dest.valid || dest.part.shape == supera::kShapeLEScatter)
                        continue;
                    if(this->IsTouching(meta, label.energy, dest.energy))
                    {
                        LOG.VERBOSE() << "Merging LEScatter track id = " << StringifyTrackID(label.part.trackid)
                                    << " into touching non-LESCatter group (id=" << StringifyInstanceID(dest.part.group_id) << ")"
                                    << " with track id = " << StringifyTrackID(dest.part.trackid) << "\n";
                        this->MergeParticleLabel(labels,dest.part.trackid,label.part.trackid);
                        merge_ctr++;
                        break;
                    }
                }
                /*
                for (auto const &parent_trackid : parents)
                {
                    auto parent_index = this->InputIndex(parent_trackid);
                    if(parent_index == kINVALID_INDEX) continue;
                    auto &parent = labels[parent_index];
                    if (!parent.valid || parent.energy.size() < 1) continue;
                    if (this->IsTouching(meta, label.energy, parent.energy))
                    {
                        LOG.VERBOSE() << "Merging LEScatter track id = " << StringifyTrackID(label.part.trackid)
                                    << " into touching parent shower group (id=" << StringifyInstanceID(parent.part.group_id) << ")"
                                    << " with track id = " << StringifyTrackID(parent.part.trackid) << "\n";
                        this->MergeParticleLabel(labels,parent_trackid,label.part.trackid);
                        merge_ctr++;
                        break;
                    }
                }
                */
            } // for (grp)
        } // while (merge_ctr)
    } // LArTPCMLReco3D::MergeShowerTouchingLEScatter()

    // ------------------------------------------------------

    bool LArTPCMLReco3D::IsTouching(const ImageMeta3D& meta, const VoxelSet& vs1, const VoxelSet& vs2) const
    {

        bool touching = false;
        size_t ix1, iy1, iz1;
        size_t ix2, iy2, iz2;
        size_t diffx, diffy, diffz;

        // Test1: is there an overlapping voxel?
        if(vs1.size()<vs2.size()) {
            for(auto const& vox : vs1.as_vector()) 
            {
                auto const& overlap = vs2.find(vox.id());
                if(overlap.id() != kINVALID_VOXELID)
                    return true;
            }
        }else{
            for(auto const& vox : vs2.as_vector()) 
            {
                auto const& overlap = vs1.find(vox.id());
                if(overlap.id() != kINVALID_VOXELID)
                    return true;
            }
        }


        // Test2: is there a voxel close enough? (distance calculation)
        for (auto const &vox1 : vs1.as_vector())
        {
            meta.id_to_xyz_index(vox1.id(), ix1, iy1, iz1);
            for (auto const &vox2 : vs2.as_vector())
            {
                meta.id_to_xyz_index(vox2.id(), ix2, iy2, iz2);
                if (ix1 > ix2) diffx = ix1 - ix2; else diffx = ix2 - ix1;
                if (iy1 > iy2) diffy = iy1 - iy2; else diffy = iy2 - iy1;
                if (iz1 > iz2) diffz = iz1 - iz2; else diffz = iz2 - iz1;
                touching = diffx <= _touch_threshold && diffy <= _touch_threshold && diffz <= _touch_threshold;
                if (touching)
                {
                    LOG.VERBOSE()<<"Touching ("<<ix1<<","<<iy1<<","<<iz1<<") ("<<ix2<<","<<iy2<<","<<iz2<<")\n";
                    break;
                }
            }
            if (touching) break;
        }

        return touching;
    } // LArTPCMLReco3D::IsTouching()


    // ------------------------------------------------------

    std::vector<supera::TrackID_t>
    LArTPCMLReco3D::ParentShowerTrackIDs(TrackID_t trackid,
                                         const std::vector<supera::ParticleLabel>& labels,
                                         bool include_lescatter) const
    {
        std::vector<supera::TrackID_t> result;
        auto target_index = this->InputIndex(trackid);
        if( target_index == kINVALID_INDEX )
            return result;
        auto const& parents = _mcpl.ParentTrackIdArray(trackid);
        result.reserve(parents.size());

        for(auto const& parent_trackid : parents) {

            auto parent_index = this->InputIndex(parent_trackid);

            if(parent_index == kINVALID_INDEX) continue;

            auto const& grp = labels[parent_index];

            if(grp.part.shape == supera::kShapeTrack ||
               grp.part.shape == supera::kShapeUnknown)
                break;

            if(!grp.valid) continue;

            if(grp.part.shape == supera::kShapeMichel ||
               grp.part.shape == supera::kShapeShower ||
               grp.part.shape == supera::kShapeDelta  ||
               (grp.part.shape == supera::kShapeLEScatter && include_lescatter))
                result.push_back(parent_trackid);
        }
        return result;
    } // LArTPCMLReco3D::ParentShowerTrackIDs()

    // ------------------------------------------------------




}
#endif
