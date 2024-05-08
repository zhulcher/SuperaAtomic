#ifndef __LARTPCMLRECO3D_CXX__
#define __LARTPCMLRECO3D_CXX__

#include "LArTPCMLReco3D.h"
#include <algorithm>
#include <cassert>
#include <set>
#include <string>
#include <sstream>
#include <cmath>
#include <cfloat>

namespace supera {

    LArTPCMLReco3D::LArTPCMLReco3D(std::string name)
    : LabelAlgorithm(name)
    // , _debug(0)
    {}

    // --------------------------------------------------------------------
    void LArTPCMLReco3D::_configure(const YAML::Node& cfg)
    {
        _semantic_priority.clear();
        if(cfg["SemanticPriority"])
            _semantic_priority = cfg["SemanticPriority"].as<std::vector<size_t> >();
        this->SetSemanticPriority(_semantic_priority);

        _touch_threshold = 1;
        if(cfg["TouchDistance"]) 
            _touch_threshold = cfg["TouchDistance"].as<size_t>();

        _edep_threshold = 0.01;
        if(cfg["EnergyDepositThreshold"]) 
            _edep_threshold = cfg["EnergyDepositThreshold"].as<double>();

        _delta_size = 3;
        if(cfg["DeltaSize"])
            _delta_size = cfg["DeltaSize"].as<size_t>();

        _compton_size = 10;
        if(cfg["ComptonSize"])
            _compton_size = cfg["ComptonSize"].as<size_t>();

        _lescatter_size = 2;
        if(cfg["LEScatterSize"])
            _lescatter_size = cfg["LEScatterSize"].as<size_t>();

        _store_lescatter = true;
        if(cfg["StoreLEScatter"])
            _store_lescatter = cfg["StoreLEScatter"].as<bool>();

        _rewrite_interactionid = true;
        if(cfg["RewriteInteractionID"])
            _rewrite_interactionid = cfg["RewriteInteractionID"].as<bool>();

        std::vector<double> min_coords(3,std::numeric_limits<double>::lowest());
        std::vector<double> max_coords(3,std::numeric_limits<double>::max());
        if(cfg["WorldBoundMin"])
            min_coords = cfg["WorldBoundMin"].as<std::vector<double> >();
        if(cfg["WorldBoundMax"])
            max_coords = cfg["WorldBoundMax"].as<std::vector<double> >();

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
                LOG_FATAL() << "SemanticPriority received an unsupported semantic type " << type << "\n";
                throw meatloaf(std::to_string(__LINE__));
            }
            bool ignore = false;
            for(auto const& used : result) {
                if(used != type) continue;
                ignore = true;
            }
            if(ignore) {
                LOG_FATAL() << "Duplicate SemanticPriority received for type " << type << "\n";
                throw meatloaf(std::to_string(__LINE__));
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
            LOG_FATAL() << "Logic error!\n";
            throw meatloaf(std::to_string(__LINE__));
        }
        order = result;
    }

    EventOutput LArTPCMLReco3D::Generate(const EventInput& data, const ImageMeta3D& meta)
    {
        LOG_DEBUG() << "starting" << std::endl;

        EventOutput result;

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

        // maybe the user has already set these upstream
        // (like in DUNE ND-LAr case)
        if (_rewrite_interactionid)
          this->SetInteractionID(labels);

        // Convert unassociated energy depositions into voxel set 
        supera::VoxelSet unass;
        size_t invalid_unass_ctr=0;
        unass.reserve(data.unassociated_edeps.size());
        for(auto const& edep : data.unassociated_edeps){
            auto vox_id = meta.id(edep);
            if(vox_id == supera::kINVALID_VOXELID) {
                invalid_unass_ctr++;
                continue;
            }
            unass.emplace(vox_id, edep.e, true);
        }
        if(invalid_unass_ctr){
            LOG_WARNING() << invalid_unass_ctr << "/" << data.unassociated_edeps.size()
            << " unassociated packets are ignored (outside BBox)" << std::endl;
        }

        // We're finally to fill in the output container.
        // There are two things we need:
        //  (1) labels for each voxel (what semantic type is each one?)
        //  (2) labels for particle groups.
        // The output format is an object containing a collection of supera::ParticleLabel,
        // each of which has voxels attached to it, so that covers both things.
        // EventOutput computes VoxelSets with the sum across all particles
        // for voxel energies and semantic labels
        this->BuildOutputLabels(labels,result,output2trackid,unass);

        return result;
    }

    // --------------------------------------------------------------------

    void LArTPCMLReco3D::BuildOutputLabels(std::vector<supera::ParticleLabel>& labels,
        supera::EventOutput& result, 
        const std::vector<TrackID_t>& output2trackid,
        const supera::VoxelSet& unass) const
    {
        LOG_DEBUG() << "starting" << std::endl;
        // Build the outupt
        std::vector<supera::ParticleLabel> output_particles;
        output_particles.reserve(output2trackid.size());
        for(auto const& trackid : output2trackid) {
            auto index = this->InputIndex(trackid);
            output_particles.emplace_back(std::move(labels[index]));
            labels[index].valid=false;
        }

        // Fill the energy and semantic label tensor for unassociated 3D points
        result._energies.reserve(unass.size());
        result._semanticLabels.reserve(unass.size());
        for(auto const& vox : unass.as_vector()) {
            result._energies.emplace(vox.id(),vox.value(),true);
            result._semanticLabels.emplace(vox.id(),supera::kShapeLEScatter,false);
        }

        // Semantic label
        // dedx energy semanticlabels
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
                    if(std::isnan(input_energy[i].value())) {
                        LOG_ERROR() << "NAN found (HE)" << std::endl;
                        LOG_ERROR() << label.dump() << std::endl;
                        throw meatloaf();
                    }
                }
            }
            // If this is LEScatter type, and if _store_lescatter == false, make sure to add here
            if(stype == supera::kShapeLEScatter && !_store_lescatter) 
            {
                for(auto& label : labels){
                    
                    if(!label.valid) continue;

                    if(label.part.shape != supera::kShapeUnknown) {
                        LOG_FATAL() << "Unexpected (logic error): valid particle remaining that is not kShapeUnknown shape...\n"
                        << label.dump() << "\n";
                        throw meatloaf(std::to_string(__LINE__));
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
                        if(std::isnan(input_energy[i].value())) {
                            LOG_ERROR() << "NAN found (LE)" << std::endl;
                            LOG_ERROR() << label.dump() << std::endl;
                            throw meatloaf();
                        }
                    }
                }
            }
        }

        result = std::move(output_particles);
        result._unassociated_voxels = unass;
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
        LOG_DEBUG() << "starting" << std::endl;
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
                            LOG_FATAL() << "Delta ray with an invalid parent is not allowed!\n";
                            throw meatloaf(std::to_string(__LINE__));
                        }
                        part.group_id = labels[parent_index].part.id;
                        break;

                    case kShapeShower:
                        part.group_id = part.id;
                        for(auto const& parent_trackid : _mcpl.ParentTrackIdArray(label.part.trackid))
                        {
                            parent_index = this->InputIndex(parent_trackid);
                            if(parent_index == kINVALID_INDEX)
                                continue;
                            if(!labels[parent_index].valid)
                                continue;
                            if(labels[parent_index].part.shape == kShapeLEScatter)
                                continue;
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
                        LOG_FATAL() << " Unexpected shape type " << part.shape << "\n";
                        throw meatloaf(std::to_string(__LINE__));
                        break;
                }
            }
        }
    }

    // ------------------------------------------------------

    void LArTPCMLReco3D::SetAncestorAttributes(std::vector<supera::ParticleLabel>& labels) const
    {
        LOG_DEBUG() << "starting" << std::endl;
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
                LOG_FATAL() << "Logic error: the parent track ID " << parent_trackid
                << " != the first in the ancestory track IDs " << parent_trackid_v.front() << "\n";
                throw meatloaf(std::to_string(__LINE__));
            }

            if(!parent_trackid_v.empty() && parent_trackid_v.back() != ancestor_trackid) {
                LOG_FATAL() << "Logic error: the ancestor track ID " << ancestor_trackid
                << " != the most distant parent ID " << parent_trackid_v.back() << "\n";
                throw meatloaf(std::to_string(__LINE__));
            }

            // Now parent_trackid must be filled unless the input data was insufficient
            if(parent_trackid == kINVALID_TRACKID){
                LOG_FATAL() << "Parent track ID missing for a particle track ID " 
                << label.part.trackid << "\n"
                << "Check the input data and make sure all particles have a parent track ID\n";
                throw meatloaf(std::to_string(__LINE__));
            }

            // If ancestor_trackid is invalid, set it to the parent.
            if(ancestor_trackid == kINVALID_TRACKID) {
                LOG_INFO() << "Ancestor track ID not set for a particle track ID "
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
        LOG_DEBUG() << "starting" << std::endl;
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
        LOG_DEBUG() << "starting" << std::endl;
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
                    LOG_FATAL() << "Unmatched voxel ID between dE/dX and energy voxels \n";
                    throw meatloaf(std::to_string(__LINE__));
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
        LOG_DEBUG() << "starting" << std::endl;
        for(auto& label : labels) {
            if(!label.valid) continue;

            switch(label.part.type) {
                case kInvalidProcess:
                label.part.shape = supera::kShapeUnknown;
                throw meatloaf("'kInvalidProcess' particle process encountered:\n" + label.dump());

                case kTrack:
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
                case kNeutron:
                    label.part.shape = kShapeLEScatter;
                    break;

                case kPhoton:
                    label.part.shape = kShapeShower;
                    break;

                case kConversion:
                case kCompton:
                case kOtherShower:
                    if (std::abs(label.part.pdg) == 11 || label.part.pdg == 22)
                    {
                        if (label.energy.size() > _compton_size)
                            label.part.shape = kShapeShower;
                        else
                        {
                            label.part.shape = kShapeLEScatter;
                            // LOG_WARNING() << "Assigned to kShapeLEScatter " << std::endl << label.dump() << std::endl;
                        }
                    }
                    else
                    {
                        label.part.shape = kShapeTrack;
                    }
                    break;

                case kNucleus:
                    if (label.energy.size() > _compton_size)
                        label.part.shape = kShapeTrack;
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
        LOG_DEBUG() << "starting" << std::endl;
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
        LOG_VERBOSE() << "Considering incoming particles:\n";
        for (size_t label_index=0; label_index<inputLabels.size(); ++label_index)
        {
            auto& inputLabel = inputLabels[label_index];
            LOG_VERBOSE() << " Particle ID=" << inputLabel.part.id << " Track ID=" << inputLabel.part.trackid << "\n";
            LOG_VERBOSE() << "     PDG=" << inputLabel.part.pdg << "\n";
            LOG_VERBOSE() << "     Edep=" << inputLabel.part.energy_deposit << "\n";

            inputLabel.part.energy_deposit = inputLabel.energy.size() ? inputLabel.energy.sum() : 0.;

            if (!inputLabel.valid)
            {
                LOG_VERBOSE() << "   --> invalid particle (i.e. already merged), skipping \n";
                continue;
            }
            /*
            if (inputLabel.Size() < 1)
            {
                LOG_VERBOSE() << "   --> no voxels, skipping \n";
                continue;
            }
            */
            if (inputLabel.part.trackid == supera::kINVALID_TRACKID) 
            {
                LOG_VERBOSE() << "   --> Invalid TrackID, skipping\n";
                continue;
            }
            if (inputLabel.part.shape == supera::kShapeLEScatter) {
                LOG_VERBOSE() << "   --> LEScatter, skipping in the first loop\n";
                lescatter_index_v.push_back(label_index);
                continue;
            }
            if (inputLabel.part.shape == supera::kShapeUnknown) {
                LOG_FATAL()   << "   --> ShapeUnknown found and unexpected!\n"
                << inputLabel.dump() << "\n";
                throw meatloaf(std::to_string(__LINE__));
            }

            auto &part = inputLabel.part;
            // 1. Record output ID
            part.id = output2trackid.size();
            LOG_VERBOSE() << "   --> Assigned output id = " << part.id << "\n";

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
            LOG_VERBOSE() << "  true particle start: " << part.first_step.dump() << "\n"
                          << "                  end: " << part.last_step.dump() << "\n";

        }

        // If LEScatter particles are meant to be kept, assign the output ID
        if(_store_lescatter) {
            for(auto const& label_index : lescatter_index_v) {
                auto& inputLabel = inputLabels[label_index];
                auto& part = inputLabel.part;
                // 1. Record output ID
                part.id = output2trackid.size();
                LOG_VERBOSE() << "   --> Assigned output id = " << part.id << "\n";

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
                LOG_VERBOSE() << "  true particle start: " << part.first_step.dump() << "\n"
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

        LOG_VERBOSE() << "trackid2output (i.e., map of track IDs to output IDs) contents:\n";
        for (std::size_t idx = 0; idx < trackid2output.size(); idx++)
            LOG_VERBOSE() << "   " << idx << " -> " << trackid2output[idx] << "\n";

    } // LArTPCMLReco3D::RegisterOutputParticles()

    // ------------------------------------------------------
    
    void LArTPCMLReco3D::DumpHierarchy(size_t trackid, const std::vector<supera::ParticleLabel>& inputLabels) const
    {
        assert(trackid < inputLabels.size());

        auto const &label = inputLabels[trackid];
        LOG_VERBOSE() << "\n#### Dumping particle record for track id "
                      << label.part.trackid << " ####\n";
        LOG_VERBOSE() << "id " << label.part.id << " from " << label.part.parent_id << "\n"
                      << "children: ";
        for (auto const &child : label.part.children_id)
            LOG_VERBOSE() <<  "   " << child;
        LOG_VERBOSE() << "\n" << label.part.dump() << "\n";

        size_t parent_trackid = label.part.parent_trackid;
        while (parent_trackid < inputLabels.size())
        {

            auto const &parent = inputLabels[parent_trackid];
            LOG_VERBOSE() << "Parent's group id: " << parent.part.group_id << " valid? " << parent.valid << "\n";
            LOG_VERBOSE() << "Parent's children: " ;
            for (auto const &child : parent.part.children_id)
                LOG_VERBOSE() << "    " << child;
            LOG_VERBOSE() << "\n" << parent.part.dump() << "\n";
            if (parent_trackid == parent.part.parent_trackid)
                break;
            if (parent_trackid == supera::kINVALID_TRACKID)
                break;
            parent_trackid = parent.part.parent_trackid;
        }
        LOG_VERBOSE() << "\n\n#### Dump done ####\n";
    } // LArTPCMLReco3D::DumpHierarchy()

    // ------------------------------------------------------

    /*
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
    */

    std::vector<supera::ParticleLabel>
    LArTPCMLReco3D::InitializeLabels(const EventInput &evtInput, const supera::ImageMeta3D &meta) const
    {
        LOG_DEBUG() << "starting" << std::endl;
        // this default-constructs the whole lot of them, which fills their values with defaults/invalid values
        std::vector<supera::ParticleLabel> labels(evtInput.size());

        LOG_DEBUG() << "Initializing labels with incoming particles...\n";
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
                    LOG_VERBOSE() << "Skipping EDep from track ID " << label.part.trackid
                    << " E=" << edep.e
                    << " pos=" << edep.x << "," << edep.y << "," << edep.z << ")\n";
                    continue;
                }

                label.energy.emplace (vox_id, edep.e,    true);
                label.dedx.emplace   (vox_id, edep.dedx, true);
                label.UpdateFirstPoint(edep);
                label.UpdateLastPoint(edep);
            }

            LOG_VERBOSE() << label.dump() << "\n";

        }  // for (idx)

        return labels;
    }  // LArTPCMLReco3D::InitializeLabels()


    // ------------------------------------------------------

    void LArTPCMLReco3D::MergeShowerConversion(std::vector<supera::ParticleLabel>& labels) const
    {
        LOG_DEBUG() << "starting" << std::endl;
        int merge_ctr = 0;
        //int invalid_ctr = 0;
        do
        {
            merge_ctr = 0;
            for (auto &label : labels)
            {
                if (!label.valid) continue;
                //if(grp.part.type != supera::kIonization && grp.part.type != supera::kConversion) continue;
                if (label.part.type != supera::kConversion) continue;
                if (std::abs(label.part.pdg) != 11) {
                    LOG_FATAL() << "Unexpected: type kConversion for a particle that is not electron!\n";
                    throw meatloaf(std::to_string(__LINE__));
                }

                auto const& parent_trackid_v = _mcpl.ParentTrackIdArray(label.part.trackid);
                TrackID_t found_trackid = kINVALID_TRACKID;
                for(auto const& parent_trackid : parent_trackid_v) 
                {
                    LOG_DEBUG() << "Inspecting: trackid " << StringifyTrackID(label.part.trackid)
                    << " => parent trackid " << StringifyTrackID(parent_trackid) << "\n";
                    auto const& parent_index = this->InputIndex(parent_trackid);
                    if (parent_index == supera::kINVALID_INDEX || !labels[parent_index].valid)
                    {
                        LOG_VERBOSE() << "Missing/Invalid parent particle with a track id " << StringifyTrackID(parent_trackid) << "\n"
                                      << "Could not find a parent for trackid " << StringifyTrackID(label.part.trackid) 
                                      << " PDG " << label.part.pdg
                                      << " " << label.part.process << " E = " << label.part.energy_init
                                      << " (" << label.part.energy_deposit << ") MeV\n";
                        continue;
                    }
                    found_trackid = parent_trackid;
                    break;
                }
                if (found_trackid != kINVALID_TRACKID) {
                    this->MergeParticleLabel(labels,found_trackid,label.part.trackid);
                    merge_ctr++;
                }
            }

            LOG_INFO() << "Merge counter: " << merge_ctr << "\n";
        } while (merge_ctr > 0);
    }  // LArTPCMLReco3D::MergeShowerConversion()

    // ------------------------------------------------------
    
    void LArTPCMLReco3D::MergeDeltas(std::vector<supera::ParticleLabel>& labels) const
    {
        LOG_DEBUG() << "starting" << std::endl;
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
                LOG_INFO() << "Merging delta trackid " << StringifyTrackID(label.part.trackid) << " PDG " << label.part.pdg
                            << " " << label.part.process << " vox count " << label.energy.size() 
                            << " (unique " << UniqueVoxelCount(label, parent) << ")\n"
                            << " ... parent found " << parent.part.trackid
                            << " PDG " << parent.part.pdg << " " << parent.part.process << "\n";
                LOG_INFO() << "Time difference: " << label.part.first_step.time - parent.part.first_step.time << "\n";
                this->MergeParticleLabel(labels, parent.part.trackid, label.part.trackid);
            }
            else
            {
                LOG_INFO() << "NOT merging delta " << StringifyTrackID(label.part.trackid) << " PDG " << label.part.pdg
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
        LOG_DEBUG() << "starting" << std::endl;
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
                LOG_VERBOSE() << "   Found particle group with shape 'shower', PDG=" << label.part.pdg
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
                if (!parent.valid) continue;
                if (this->IsTouching(meta, label.energy, parent.energy)) {
                    // if parent is found, merge
                    this->MergeParticleLabel(labels, parent_trackid, label.part.trackid);
                    LOG_VERBOSE() << "   Merged to group w/ track id=" << StringifyTrackID(parent.part.trackid) << "\n";
                    merge_ctr++;
                }
            }
            LOG_DEBUG() << "Merge counter: " << merge_ctr << " invalid counter: " << invalid_ctr << "\n";
        } while (merge_ctr>0);
    } // LArTPCMLReco3D::MergeShowerFamilyTouching()


    // ------------------------------------------------------

    void LArTPCMLReco3D::MergeShowerIonizations(std::vector<supera::ParticleLabel>& labels) const
    {
        LOG_DEBUG() << "starting" << std::endl;
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
            LOG_DEBUG() << "Ionization merge counter: " << merge_ctr << " invalid counter: " << invalid_ctr << "\n";
        } while (merge_ctr > 0);
    } // LArTPCMLReco3D::MergeShowerIonizations()


    // ------------------------------------------------------
    void LArTPCMLReco3D::MergeShowerTouching(const supera::ImageMeta3D& meta,
                                             std::vector<supera::ParticleLabel>& labels) const
    {
        LOG_DEBUG() << "starting" << std::endl;
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
                    std::set<supera::TrackID_t> parent_list_a;
                    std::set<supera::TrackID_t> parent_list_b;

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
                        if (label_a.energy.size() > label_b.energy.size())
                            this->MergeParticleLabel(labels, label_a.part.trackid, label_b.part.trackid);
                        else
                            this->MergeParticleLabel(labels, label_b.part.trackid, label_a.part.trackid);
                        merge_ctr++;
                    }
                }
            }
            LOG_INFO() << "Merge counter: " << merge_ctr << "\n";
        } while (merge_ctr > 0);
    } // LArTPCMLReco3D::MergeShowerTouching()

    // ------------------------------------------------------

    void LArTPCMLReco3D::MergeShowerTouchingElectron(const supera::ImageMeta3D& meta,
                                                      std::vector<supera::ParticleLabel>& labels) const
    {
        LOG_DEBUG() << "starting" << std::endl;
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

                LOG_VERBOSE() << "Inspecting LEScatter Track ID " << StringifyTrackID(label.part.trackid)
                            << " PDG " << label.part.pdg
                            << " " << label.part.process << "\n";
                LOG_VERBOSE() << "  ... parents:\n";
                for(auto const& parent_trackid : parents)
                    LOG_VERBOSE() << "     "<< StringifyTrackID(parent_trackid) << "\n";

                for (auto const &parent_trackid : parents)
                {
                    auto parent_index = this->InputIndex(parent_trackid);
                    if(parent_index == kINVALID_INDEX) continue;
                    auto &parent = labels[parent_index];
                    if (!parent.valid || parent.energy.size() < 1) continue;
                    if (this->IsTouching(meta, label.energy, parent.energy))
                    {
                        LOG_VERBOSE() << "Merging LEScatter track id = " << StringifyTrackID(label.part.trackid)
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
        LOG_DEBUG() << "starting" << std::endl;
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

                if (label.part.type == supera::kNeutron || label.part.type == supera::kNucleus)
                    continue;

                auto const &parents = _mcpl.ParentTrackIdArray(label.part.trackid);

                LOG_VERBOSE() << "Inspecting LEScatter Track ID " << StringifyTrackID(label.part.trackid)
                            << " PDG " << label.part.pdg
                            << " " << label.part.process << "\n";
                LOG_VERBOSE() << "  ... parents:\n";
                for(auto const& parent_trackid : parents)
                    LOG_VERBOSE() << "     "<< StringifyTrackID(parent_trackid) << "\n";

                for(auto &dest : labels) {
                    if(!dest.valid || dest.part.shape == supera::kShapeLEScatter)
                        continue;
                    if(this->IsTouching(meta, label.energy, dest.energy))
                    {
                        LOG_VERBOSE() << "Merging LEScatter track id = " << StringifyTrackID(label.part.trackid)
                                    << " into touching non-LESCatter group (id=" << StringifyInstanceID(dest.part.group_id) << ")"
                                    << " with track id = " << StringifyTrackID(dest.part.trackid) << "\n";
                        this->MergeParticleLabel(labels,dest.part.trackid,label.part.trackid);
                        merge_ctr++;
                        break;
                    }
                }
            } // for (grp)
        } // while (merge_ctr)
    } // LArTPCMLReco3D::MergeShowerTouchingLEScatter()

    // ------------------------------------------------------

    bool LArTPCMLReco3D::IsTouching(const ImageMeta3D& meta, const VoxelSet& vs1, const VoxelSet& vs2) const
    {
        LOG_DEBUG() << "starting" << std::endl;
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
                    LOG_VERBOSE()<<"Touching ("<<ix1<<","<<iy1<<","<<iz1<<") ("<<ix2<<","<<iy2<<","<<iz2<<")\n";
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
        LOG_DEBUG() << "starting" << std::endl;
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
