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

    // --------------------------------------------------------------------

    EventOutput LArTPCMLReco3D::Generate(const EventInput& data, const ImageMeta3D& meta)
    {
        EventOutput result;

        // fill in the working structures that link the list of particles and its genealogy
        _mcpl.InferParentage(data);
        std::vector<supera::Index_t> const& trackid2index = _mcpl.TrackIdToIndex();

        // Assign the initial labels for each particle.
        // They will be grouped together in various ways in the subsequent steps.
        std::vector<supera::ParticleLabel> labels = this->InitializeLabels(data);

        // Now group the labels together in certain cases
        // (e.g.: electromagnetic showers, neutron clusters, ...)
        // There are lots of edge cases so the logic is spread out over many methods.
        this->MergeShowerIonizations(labels);
        this->MergeShowerTouchingLEScatter(meta, labels);  //todo: this was *also* being run at the *end* of the 'MergeShower...()' bunch.  which is right?
        this->ApplyEnergyThreshold(labels);
        this->MergeShowerConversion(labels);
        this->MergeShowerFamilyTouching(meta, labels);
        this->MergeShowerTouching(meta, labels);
        this->MergeDeltas(labels);

        // Now that we have grouped the true particles together,
        // at this point we're ready to build a new set of labels
        // which contain only the top particle of each merged group.
        // The first step will be to create a mapping
        // from the original GEANT4 trackids to the new groups.
        std::vector<int> trackid2output(trackid2index.size(), -1);  // index: original GEANT4 trackid.  stored value: output group index.
        std::vector<supera::TrackID_t> output2trackid;  // reverse of above.
        this->AssignParticleGroupIDs(trackid2index, labels, output2trackid, trackid2output);

        // Next, we need to clean up a number of edge cases that don't always get assigned correctly.
        this->FixOrphanShowerGroups(labels, output2trackid, trackid2output);
        this->FixOrphanNonShowerGroups(labels, output2trackid, trackid2output);
        this->FixInvalidParentShowerGroups(labels, output2trackid, trackid2output);
        this->FixUnassignedParentGroups(labels, output2trackid, trackid2output);
        this->FixUnassignedGroups(labels, output2trackid);
        this->FixUnassignedLEScatterGroups(labels, output2trackid);
        LArTPCMLReco3D::FixFirstStepInfo(labels, meta, output2trackid);

        // We're finally to fill in the output container.
        // There are two things we need:
        //  (1) labels for each voxel (what semantic type is each one?)
        //  (2) labels for particle groups.
        // The output format is an object containing a collection of supera::ParticleLabels,
        // each of which has voxels attached to it, so that covers both things.
        // EventOutput computes VoxelSets with the sum across all particles
        // for voxel energies, dE/dxs, and semantic labels
        // upon demand (see EventOutput::VoxelEnergies() etc.).
        result = this->BuildOutputLabels(labels, output2trackid, trackid2output, trackid2index);

        return result;
    }

    // --------------------------------------------------------------------
    // --------------------------------------------------------------------

    // ------------------------------------------------------
    void LArTPCMLReco3D::ApplyEnergyThreshold(std::vector<supera::ParticleLabel>& labels) const
    {
        // Loop again and eliminate voxels that has energy below threshold
        for (auto &label : labels)
        {
            supera::VoxelSet energies, dEdXs;
            energies.reserve(label.energy.size());
            dEdXs.reserve(label.dedx.size());
            if (energies.size() != dEdXs.size())
                throw meatloaf("Inconsistent energy (" + std::to_string(energies.size()) + ") & dE/dX (" + std::to_string(dEdXs.size()) + ") voxel counts in voxel set");

            const auto energy_vec =  label.energy.as_vector();
            const auto dedx_vec =  label.dedx.as_vector();
            for (std::size_t idx = 0; idx < energy_vec.size(); idx++)
            {
                const auto & vox = energy_vec[idx];
                if (vox.value() < _edep_threshold)
                {
                    LOG.VERBOSE() << "  Dropping below-threshold voxel " << vox.id() << " with edep = " << vox.value() << "\n";
                    continue;
                }
                energies.emplace(vox.id(), vox.value(), true);
                dEdXs.emplace(dedx_vec[idx].id(), dedx_vec[idx].value(), true);
            }
            label.energy = std::move(energies);
            label.dedx = std::move(dEdXs);

            // If compton, here decide whether it should be supera::kComptonHE (high energy)
            if (label.type == supera::kCompton && label.energy.size() > _compton_size)
            {
                //std::cout<<"Track ID "<<grp.part.trackid<<" high energy compton"<<std::endl;
                label.type = supera::kComptonHE;
            } else if (label.type == supera::kOtherShower && label.energy.size() > _compton_size)
            {
                //std::cout<<"Track ID "<<grp.part.trackid<<" high energy compton"<<std::endl;
                label.type = supera::kOtherShowerHE;
            }
        }
    } // LArTPCMLReco3D::ApplyEnergyThreshold()

    // ------------------------------------------------------
    
    void LArTPCMLReco3D::AssignParticleGroupIDs(const std::vector<TrackID_t> &trackid2index,
                                                std::vector<supera::ParticleLabel> &inputLabels,
                                                std::vector<TrackID_t> &output2trackid,
                                                std::vector<int> &trackid2output) const
    {
        // first create the track id list
        output2trackid.resize(trackid2index.size());
        output2trackid.clear();

        // assign particle group ID numbers and make sure they have all info set
        LOG.VERBOSE() << "Considering incoming particle groups:\n";
        for (auto & inputLabel : inputLabels)
        {
            LOG.VERBOSE() << " Particle ID=" << inputLabel.part.id << " Track ID=" << inputLabel.part.trackid << "\n";
            LOG.VERBOSE() << "     Edep=" << inputLabel.part.energy_deposit << "\n";
            size_t output_counter = output2trackid.size();
            if (!inputLabel.valid)
            {
                LOG.VERBOSE() << "   --> invalid group (i.e. already merged), skipping \n";
                continue;
            }
            if (inputLabel.part.process != "primary" && inputLabel.Size() < 1)
            {
                LOG.VERBOSE() << "   --> no voxels, skipping \n";
                continue;
            }
            // Also define particle "first step" and "last step"
            auto &part = inputLabel.part;
            auto const &first_pt = inputLabel.first_pt;
            auto const &last_pt = inputLabel.last_pt;
            LOG.VERBOSE() << "      examining true particle start:" << first_pt.x<< " " << first_pt.y << " " << first_pt.z << "\n";
            if (first_pt.t != kINVALID_DOUBLE)
                part.first_step = supera::Vertex(first_pt.x, first_pt.y, first_pt.z, first_pt.t);
            if (last_pt.t != kINVALID_DOUBLE)
                part.last_step = supera::Vertex(last_pt.x, last_pt.y, last_pt.z, last_pt.t);
            LOG.VERBOSE() << "     true particle start: " << inputLabel.part.first_step.dump() << "\n"
                          << "                   end: " << inputLabel.part.last_step.dump() << "\n";
            inputLabel.part.energy_deposit = inputLabel.energy.size() ? inputLabel.energy.sum() : 0.;


            //if (grp.part.process != "primary" && grp.shape() == kShapeLEScatter)
            //{
            //  LOG.VERBOSE() << "   --> LEScatter shape, skipping" << std::endl;
            //  continue;
            //}

            inputLabel.part.id = output_counter;
            LOG.VERBOSE() << "   --> Assigned output group id = " << inputLabel.part.id << "\n";
            trackid2output[inputLabel.part.trackid] = static_cast<int>(output_counter);
            for (auto const &child : inputLabel.trackid_v)
                trackid2output[child] = static_cast<int>(output_counter);
            output2trackid.push_back(static_cast<int>(inputLabel.part.trackid));
            ++output_counter;
        }

        LOG.VERBOSE() << "trackid2output (i.e., map of track IDs to output group IDs) contents:\n";
        for (std::size_t idx = 0; idx < trackid2output.size(); idx++)
            LOG.VERBOSE() << "   " << idx << " -> " << trackid2output[idx] << "\n";

        // now assign relationships
        LOG.VERBOSE() << "Assigning group relationships:\n";
        for (auto const &trackid : output2trackid)
        {
            auto &inputLabel = inputLabels[trackid];
            LOG.VERBOSE() << "  Group for trackid=" << trackid
                          << " (pdg = " << inputLabel.part.pdg << ", particle id=" << inputLabel.part.id << ")\n";
            if (std::abs(inputLabel.part.pdg) != 11 && std::abs(inputLabel.part.pdg) != 22)
            {
                LOG.VERBOSE() << "    ---> not EM, leaving alone (parent id=" << inputLabel.part.parent_id
                              << " and group id=" << inputLabel.part.group_id << ")\n";
                continue;
            }

            unsigned int parent_trackid = inputLabel.part.parent_trackid;
            LOG.VERBOSE() << "   initial parent track id:" << parent_trackid << "\n";

            if (parent_trackid != supera::kINVALID_UINT && trackid2output[parent_trackid] >= 0)
            {
                LOG.VERBOSE() << "   --> assigning group for trackid " << trackid << " to parent trackid: " << parent_trackid << "\n";
                /*
                if(trackid2output[parent_trackid] < 0)
            grp.part.parent_id(grp.part.id());
                else {
                */
                inputLabel.part.parent_id = trackid2output[parent_trackid];
                int parent_output_id = trackid2output[parent_trackid];
                TrackID_t parent_id = output2trackid[parent_output_id];
                if (inputLabels[parent_id].valid)
                    inputLabels[parent_id].part.children_id.push_back(inputLabel.part.id);
            } // if (parent_trackid != larcv::kINVALID_UINT)
            else
            {
                LOG.VERBOSE() << "     --> no valid ancestor.  Assigning this particle's parent IDs to itself.  "
                              << " (group id=" << inputLabel.part.group_id << ")\n";
                // otherwise checks in CheckParticleValidity() will fail (parent ID will point to something not in output)
                inputLabel.part.parent_id = inputLabel.part.id;
            }
        }

        // make sure the primary particles' parent and group id are set (they are themselves)
        for (auto &grp : inputLabels)
        {
            auto &part = grp.part;
            if (part.parent_trackid != supera::kINVALID_UINT)
                continue;
            part.group_id = part.id;
            part.parent_id = part.id;
            LOG.VERBOSE() << "Assigned primary particle's own ID to its group and parent IDs:\n" << part.dump() << "\n";
        }

    } // LArTPCMLReco3D::AssignParticleGroupIDs()

    // ------------------------------------------------------

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

            // set semantic type
            supera::SemanticType_t semantic = groupedInputLabel.shape();
            if (semantic == kShapeUnknown)
            {
                LOG.FATAL() << "Unexpected type while assigning semantic class: " << groupedInputLabel.type << "\n";
                auto const &part = groupedInputLabel.part;
                LOG.FATAL() << "Particle ID " << part.id << " Type " << groupedInputLabel.type << " Valid " << groupedInputLabel.valid
                            << " Track ID " << part.trackid << " PDG " << part.pdg
                            << " " << part.process << " ... " << part.energy_init << " MeV => "
                            << part.energy_deposit << " MeV "
                            << groupedInputLabel.trackid_v.size() << " children " << groupedInputLabel.energy.size() << " voxels " << groupedInputLabel.energy.sum()
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

            // store the shape (semantic) type in particle
            groupedInputLabel.part.shape = semantic;
            // store the voxel count and energy deposit
            groupedInputLabel.part.energy_deposit = groupedInputLabel.energy.sum();

            // duplicate the particle to the output container
            outputLabels.Particles()[index] = groupedInputLabel;

            // set the particle in the original container to 'invalid' so we don't accidentally use it again
            groupedInputLabel.valid = false;
        } // for (index)

        // now vacuum up any orphan particles into a top-level orphan particle
        // (anything that is still not grouped after all the cleanup steps).
        // the others *should* have gotten absorbed by the Merge() calls
        // in the various LArTPCMLReco3D::Merge...() methods...
        supera::ParticleLabel orphan;
        orphan.part.pdg = 0;
        for (std::size_t trkid = 0; trkid < trackid2output.size(); trkid++)
        {
            int outputIdx = trackid2output[trkid];
            if (outputIdx >= 0)
                continue;

            orphan.Merge(groupedInputLabels[trackid2index[trkid]]);
        } // for (idx)
        outputLabels.Particles().push_back(std::move(orphan));

        return outputLabels;
    } // LArTPCMLReco3D::BuildOutputClusters


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
            if (parent_trackid == supera::kINVALID_UINT)
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
            if (grp.shape() != kShapeShower)
                continue;
            LOG.DEBUG() << "Analyzing particle id " << out_index << " trackid " << trackid << "\n"
                        << grp.part.dump() << "\n";
            int parent_partid = -1;
            unsigned int parent_trackid;
            auto parent_trackid_v = ParentTrackIDs(trackid);
            for (unsigned int idx : parent_trackid_v)
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
            if (grp.shape() == kShapeShower)
                continue;
            if (grp.part.group_id != kINVALID_INSTANCEID)
            {
                LOG.VERBOSE() << "  group for trackid " << trackid << "  has group id " << grp.part.group_id << " already.  Skipping\n";
                continue;
            }
            LOG.DEBUG() << " #### Non-shower ROOT SEARCH #### \n"
                         << " Analyzing a particle index " << out_index << " id " << grp.part.id << "\n" << grp.part.dump() << "\n";

            auto parent_trackid_v = ParentTrackIDs(trackid);
            std::stringstream ss;
            ss << "   candidate ancestor track IDs:";
            for (const auto & trkid : parent_trackid_v)
                ss << " " << trkid;
            LOG.VERBOSE() << ss.str() << "\n";
            size_t group_id = kINVALID_INSTANCEID;
            bool stop = false;
            for (auto const &parent_trackid : parent_trackid_v)
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
                switch (parent.shape())
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
            if (label.shape() != kShapeShower)
                continue;
            LOG.DEBUG() << " #### SHOWER ROOT SEARCH: Analyzing a particle index " << out_index
                        << " track id " << label.part.trackid << "\n"
                        << label.part.dump()
                        << "      group type = " << label.type << "\n"
                        << "      group shape = " << label.shape() << "\n"
                        << "      group is valid = " << label.valid << "\n"
                         << "      group is mapped to output index = " << trackid2output[trackid];

            auto parent_trackid_v = ParentTrackIDs(trackid);
            std::stringstream ss;
            ss << "   candidate ancestor track IDs:";
            for (const auto & trkid : parent_trackid_v)
                ss << " " << trkid;
            LOG.VERBOSE() << "       " << ss.str();
            supera::TrackID_t root_id = label.part.id;
            supera::TrackID_t root_trackid = label.part.trackid;
            bool stop = false;
            std::vector<size_t> intermediate_trackid_v;
            intermediate_trackid_v.push_back(trackid);
            for (auto const &parent_trackid : parent_trackid_v)
            {
                auto const &parent = inputLabels[parent_trackid];
                LOG.VERBOSE() << "  ancestor track id " << parent_trackid << "\n"
                              << parent.part.dump()
                              << "      group type = " << parent.type << "\n"
                              << "      group shape = " << parent.shape() << "\n"
                              << "      group is valid = " << parent.valid << "\n"
                              << "      group is mapped to output index = " << trackid2output[parent_trackid];

                switch (parent.shape())
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
                        stop = (stop || parent.shape() != kShapeShower);
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
            auto shape = label.shape();
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
                    parent_shape = inputLabels[output2trackid[parent_partid]].shape();
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
                                                               const std::vector<TrackID_t> &output2trackid) const
    {
        LOG.DEBUG() << "Inspecting LEScatter groups for invalid group ids...\n";
        for (auto & label : inputLabels)
//    for (size_t output_index = 0; output_index < output2trackid.size(); ++output_index)
        {
//      auto &grp = part_grp_v[output2trackid[output_index]];
            if (label.shape() != kShapeLEScatter)
                continue;

            LOG.VERBOSE() << "   trackid=" << label.part.trackid << " group=" << label.part.group_id << "\n";
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
                LOG.VERBOSE() << "     --> rewrote group id to parent (trackid=" << parent_part.part.trackid
                              << ")'s group id = " << parent_part.part.group_id << "\n";
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
                          << "  id=" << grp.part.id
                          << "  track id=" << grp.part.trackid
                          << "  parent trackid=" << parent_trackid
                          << "  parent id=" << parent_id
                          << "\n";

            auto &parent = inputLabels[parent_trackid].part;
            // if parent_id is invalid, try if parent_trackid can help out
            if (parent_id == supera::kINVALID_INSTANCEID &&
                parent_trackid != supera::kINVALID_UINT &&
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

    std::vector<supera::ParticleLabel> LArTPCMLReco3D::InitializeLabels(const std::vector<ParticleInput> & evtInput) const
    {
        // this default-constructs the whole lot of them, which fills their values with defaults/invalid values
        std::vector<supera::ParticleLabel> labels(evtInput.size());

        for (std::size_t idx = 0; idx < evtInput.size(); idx++)
        {
            auto & label = labels[idx];
            label.part = evtInput[idx].part;

            auto mother_index = _mcpl.ParentIndex()[idx];
            if (mother_index >= 0 && label.part.parent_pdg == supera::kINVALID_PDG)
                label.part.parent_pdg = _mcpl.ParentPdgCode()[idx];

            label.valid = true;

            auto pdg_code = label.part.pdg;
            if (pdg_code == 22) {
                // photon:
                // reset first, last, and end position, since the photon presumably has traveled
                // and they won't be useful.
                // (set them to a guaranteed
                label.type = supera::kPhoton;
                const supera::Vertex invalidVertex(supera::kINVALID_DOUBLE, supera::kINVALID_DOUBLE,
                                                   supera::kINVALID_DOUBLE, supera::kINVALID_DOUBLE);
                label.part.first_step = invalidVertex;
                label.part.last_step = invalidVertex;
                label.part.end_pt = invalidVertex;
            }
            else if (pdg_code == 11)
            {

                const std::string & prc = label.part.process;
                if (prc == "muIoni" || prc == "hIoni" || prc == "muPairProd")
                    label.type = supera::kDelta;
                else if (prc == "muMinusCaptureAtRest" || prc == "muPlusCaptureAtRest" || prc == "Decay")
                    label.type = supera::kDecay;
                else if (prc == "compt")
                    label.type = supera::kCompton;
                else if (prc == "phot")
                    label.type = supera::kPhotoElectron;
                else if (prc == "eIoni")
                    label.type = supera::kIonization;
                else if (prc == "conv")
                    label.type = supera::kConversion;
                else if (prc == "primary")
                    label.type = supera::kPrimary;
                else
                    label.type = supera::kOtherShower;
            }
            else
            {
                label.type = supera::kTrack;
                if (label.part.pdg == 2112)
                    label.type = supera::kNeutron;
            }

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
                //if(grp.type != supera::kIonization && grp.type != supera::kConversion) continue;
                if (label.type != supera::kConversion) continue;
                // merge to a valid "parent"
                bool parent_found = false;
                unsigned int parent_index = label.part.parent_trackid;
                unsigned int parent_index_before = label.part.trackid;
                while (true)
                {
                    LOG.VERBOSE() << "Inspecting: " << label.part.trackid << " => " << parent_index << "\n";
                    if (parent_index == supera::kINVALID_UINT)
                    {
                        LOG.VERBOSE() << "Invalid parent track id " << parent_index
                                      << " Could not find a parent for " << label.part.trackid << " PDG " << label.part.pdg
                                      << " " << label.part.process << " E = " << label.part.energy_init
                                      << " (" << label.part.energy_deposit << ") MeV\n";
                        auto const &parent = labels[parent_index_before].part;
                        LOG.VERBOSE() << "Previous parent: " << parent.trackid << " PDG " << parent.pdg
                                      << " " << parent.process << "\n";
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
                        unsigned int ancestor_index = parent.part.parent_trackid;
                        if (ancestor_index == parent_index)
                        {
                            LOG.INFO() << "Particle " << parent_index << " is root and invalid particle...\n";
                            LOG.INFO() << "PDG " << parent.part.pdg << " " << parent.part.process << "\n";
                            break;
                        }
                        parent_index_before = parent_index;
                        parent_index = ancestor_index;
                    }
                }
                // if parent is found, merge
                if (parent_found)
                {
                    auto &parent = labels[parent_index];
                    parent.Merge(label);
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
            //if(label.type != supera::kDelta) continue;
            if (label.shape() != supera::kShapeDelta) continue;
            unsigned int parent_trackid = label.part.parent_trackid;
            auto &parent = labels[parent_trackid];
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
                LOG.VERBOSE() << "Merging delta " << label.part.trackid << " PDG " << label.part.pdg
                              << " " << label.part.process << " vox count " << label.energy.size() << "\n"
                              << " ... parent found " << parent.part.trackid
                              << " PDG " << parent.part.pdg << " " << parent.part.process << "\n";
                LOG.VERBOSE() << "Time difference: " << label.part.first_step.time - parent.part.first_step.time << "\n";
                parent.Merge(label, parent.shape() != kShapeTrack);  // a delta ray is unlikely to extend a *track* in the up- or downstream directions
            }
            else
            {
                LOG.VERBOSE() << "NOT merging delta " << label.part.trackid << " PDG " << label.part.pdg
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
                if (label.shape() != supera::kShapeShower) continue;
                if (label.part.parent_trackid == supera::kINVALID_UINT) continue;  // primaries can't have parents
                // search for a possible parent
                int parent_trackid = -1;
                LOG.VERBOSE() << "   Found particle group with shape 'shower', PDG=" << label.part.pdg
                              << "\n    track id=" << label.part.trackid
                              << ", and alleged parent track id=" << label.part.parent_trackid << "\n";
                // a direct parent ?
                if (labels[label.part.parent_trackid].valid)
                    parent_trackid = static_cast<int>(label.part.parent_trackid);
                else
                {
                    for (size_t shower_trackid = 0; shower_trackid < labels.size(); ++shower_trackid)
                    {
                        auto const &candidate_grp = labels[shower_trackid];
                        if (shower_trackid == label.part.parent_trackid || !candidate_grp.valid)
                            continue;
                        for (auto const &trackid : candidate_grp.trackid_v)
                        {
                            if (trackid != label.part.parent_trackid)
                                continue;
                            parent_trackid = static_cast<int>(shower_trackid);
                            break;
                        }
                        if (parent_trackid >= 0)
                            break;
                    }
                }
                if (parent_trackid < 0 || parent_trackid == (int)(label.part.trackid)) continue;
                auto& parent = labels[parent_trackid];
                //auto parent_type = labels[parent_trackid].type;
                //if(parent_type == supera::kTrack || parent_type == supera::kNeutron) continue;
                if (parent.shape() != supera::kShapeShower && parent.shape() != supera::kShapeDelta && parent.shape() != supera::kShapeMichel) continue;
                if (this->IsTouching(meta, label.energy, parent.energy)) {
                    // if parent is found, merge
                    parent.Merge(label);
                    LOG.VERBOSE() << "   Merged to group w/ track id=" << parent.part.trackid << "\n";
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
                if (label.type != supera::kIonization) continue;
                // merge to a valid "parent"
                bool parent_found = false;
                unsigned int parent_index = label.part.parent_trackid;
                unsigned int parent_index_before = label.part.trackid;
                while (true)
                {
                    //std::cout<< "Inspecting: " << label.part.trackid << " => " << parent_index << std::endl;
                    if (parent_index == supera::kINVALID_UINT)
                    {
                        LOG.ERROR() << "Invalid parent track id " << parent_index
                                  << " Could not find a parent for " << label.part.trackid << " PDG " << label.part.pdg
                                  << " " << label.part.process << " E = " << label.part.energy_init
                                  << " (" << label.part.energy_deposit << ") MeV\n";
                        auto const &parent = labels[parent_index_before].part;
                        std::cout << "Previous parent: " << parent.trackid << " PDG " << parent.pdg
                                      << " " << parent.process << "\n";
                        parent_found = false;
                        invalid_ctr++;
                        break;
                    }
                    auto const &parent = labels[parent_index];
                    parent_found = parent.valid;
                    if (parent_found) break;
                    else
                    {
                        unsigned int ancestor_index = parent.part.parent_trackid;
                        if (ancestor_index == parent_index)
                        {
                            LOG.INFO() << "Particle " << parent_index << " is root and invalid particle...\n"
                                         << "PDG " << parent.part.pdg << " " << parent.part.process << "\n";
                            break;
                        }
                        parent_index_before = parent_index;
                        parent_index = ancestor_index;
                    }
                }
                // if parent is found, merge
                if (parent_found)
                {
                    auto &parent = labels[parent_index];
                    parent.Merge(label);
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
                auto &lbl_a = labels[i];
                if (!lbl_a.valid) continue;
                if (lbl_a.shape() != supera::kShapeShower) continue;
                for (size_t j = 0; j < labels.size(); ++j)
                {
                    if (i == j) continue;
                    auto &lbl_b = labels[j];
                    if (!lbl_b.valid) continue;
                    if (lbl_b.shape() != supera::kShapeShower) continue;

                    // check if these showers share the parentage
                    // list a's parents
                    size_t trackid = i;
                    std::set<size_t> parent_list_a;
                    std::set<size_t> parent_list_b;
                    /*
                    while(1){
                      auto const& parent_a = labels[trackid];
                      if(parent_a.part.parent_trackid >= labels.size())
                        break;
                      if(parent_a.part.parent_trackid == parent_a.part.trackid)
                        break;
                      trackid = parent_a.part.parent_trackid;
                      if(parent_a.shape() == larcv::kShapeMichel ||
                         parent_a.shape() == larcv::kShapeShower ||
                         parent_a.shape() == larcv::kShapeDelta )
                        parent_list_a.insert(trackid);
                      else if(parent_a.shape() == larcv::kShapeTrack ||
                        parent_a.shape() == larcv::kShapeUnknown)
                        break;
          
                      if(trackid < labels.size() && labels[trackid].part.parent_trackid == trackid)
                        break;
                    }
                    */
                    auto parents_a = this->ParentShowerTrackIDs(trackid, labels);
                    for (auto const &parent_trackid : parents_a) parent_list_a.insert(parent_trackid);
                    parent_list_a.insert(trackid);

                    trackid = j;
                    /*
                    while(1){
                      auto const& parent_b = labels[trackid];
                      if(parent_b.part.parent_trackid >= labels.size())
                        break;
                      if(parent_b.part.parent_trackid == parent_b.part.trackid)
                        break;
                      trackid = parent_b.part.parent_trackid;
                      if(parent_b.shape() == larcv::kShapeMichel ||
                         parent_b.shape() == larcv::kShapeShower ||
                         parent_b.shape() == larcv::kShapeDelta )
                        parent_list_b.insert(trackid);
                      else if(parent_b.shape() == larcv::kShapeTrack ||
                        parent_b.shape() == larcv::kShapeUnknown)
                        break;
                      if(trackid < labels.size() && labels[trackid].part.parent_trackid == trackid)
                        break;
                    }
                    */
                    auto parents_b = this->ParentShowerTrackIDs(trackid, labels);
                    for (auto const &parent_trackid : parents_b) parent_list_b.insert(parent_trackid);
                    parent_list_b.insert(trackid);

                    bool merge = false;
                    for (auto const &parent_trackid : parent_list_a)
                    {
                        if (parent_list_b.find(parent_trackid) != parent_list_b.end())
                            merge = true;
                        if (merge) break;
                    }
                    for (auto const &parent_trackid : parent_list_b)
                    {
                        if (parent_list_a.find(parent_trackid) != parent_list_a.end())
                            merge = true;
                        if (merge) break;
                    }

                    if (merge && this->IsTouching(meta, lbl_a.energy, lbl_b.energy))
                    {
                        if (lbl_a.energy.size() < lbl_b.energy.size())
                            lbl_b.Merge(lbl_a);
                        else
                            lbl_a.Merge(lbl_b);
                        merge_ctr++;
                    }
                }
            }
            LOG.DEBUG() << "Merge counter: " << merge_ctr << "\n";
        } while (merge_ctr > 0);
    } // LArTPCMLReco3D::MergeShowerTouching()

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
                if (!label.valid || label.energy.size() < 1 || label.shape() != supera::kShapeLEScatter) continue;

                auto const &parents = this->ParentTrackIDs(label.part.trackid);

                LOG.VERBOSE() << "Inspecting LEScatter Track ID " << label.part.trackid
                            << " PDG " << label.part.pdg
                            << " " << label.part.process << "\n";
                LOG.VERBOSE() << "  ... parents:\n";
                for(auto const& parent_trackid : parents)
                    LOG.VERBOSE() << "     "<< parent_trackid << "\n";

                for (auto const &parent_trackid : parents)
                {
                    auto &parent = labels[parent_trackid];
                    if (!parent.valid || parent.energy.size() < 1) continue;
                    if (this->IsTouching(meta, label.energy, parent.energy))
                    {
                        LOG.VERBOSE() << "Merging LEScatter track id = " << label.part.trackid
                                    << " into touching parent shower group (id=" << parent.part.group_id << ")"
                                    << " with track id = " << parent.part.trackid << "\n";
                        parent.Merge(label);
                        merge_ctr++;
                        break;
                    }
                } // for (parent_trackid)
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

        for (auto const &vox1 : vs1.as_vector())
        {
            meta.id_to_xyz_index(vox1.id(), ix1, iy1, iz1);
            for (auto const &vox2 : vs2.as_vector())
            {
                meta.id_to_xyz_index(vox2.id(), ix2, iy2, iz2);
                if (ix1 > ix2) diffx = ix1 - ix2; else diffx = ix2 - ix1;
                if (iy1 > iy2) diffy = iy1 - iy2; else diffy = iy2 - iy1;
                if (iz1 > iz2) diffz = iz1 - iz2; else diffz = iz2 - iz1;
                touching = diffx <= 1 && diffy <= 1 && diffz <= 1;
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

    std::vector<unsigned int>
    LArTPCMLReco3D::ParentShowerTrackIDs(size_t trackid,
                                         const std::vector<supera::ParticleLabel>& labels,
                                         bool include_lescatter) const
    {
        auto parents = this->ParentTrackIDs(trackid);
        std::vector<unsigned int> result;
        result.reserve(parents.size());

        for(auto const& parent_id : parents) {

            if(parent_id >= labels.size()) continue;

            auto const& grp = labels[parent_id];
            if(!grp.valid) continue;

            if(grp.shape() == supera::kShapeTrack ||
               grp.shape() == supera::kShapeUnknown)
                break;

            if(grp.shape() == supera::kShapeMichel ||
               grp.shape() == supera::kShapeShower ||
               grp.shape() == supera::kShapeDelta  ||
               (grp.shape() == supera::kShapeLEScatter && include_lescatter))
                result.push_back(parent_id);
        }
        return result;
    } // LArTPCMLReco3D::ParentShowerTrackIDs()

    // ------------------------------------------------------

    std::vector<unsigned int> LArTPCMLReco3D::ParentTrackIDs(size_t trackid) const
    {
        auto const &trackid2index = _mcpl.TrackIdToIndex();
        std::vector<unsigned int> result;

        if (trackid >= trackid2index.size() || trackid2index[trackid] < 0)
            return result;

        unsigned int parent_trackid = _mcpl.ParentTrackId()[trackid2index[trackid]];
        std::set<unsigned int> accessed;
        while ((size_t) (parent_trackid) < trackid2index.size() && trackid2index[parent_trackid] >= 0)
        {
            if (accessed.find(parent_trackid) != accessed.end())
            {
                LOG.FATAL() << "LOOP-LOGIC-ERROR for ParentTrackIDs for track id " << trackid << ": repeated ancestor!\n";
                LOG.FATAL() << "Ancestors found:\n";
                for (size_t parent_cand_idx = 0; parent_cand_idx < result.size(); ++parent_cand_idx)
                {
                    auto const &parent_cand_trackid = result[parent_cand_idx];
                    LOG.FATAL() << "Ancestor #" << parent_cand_idx
                                << " Track ID " << parent_cand_trackid
                                << " PDG " << _mcpl.PdgCode()[trackid2index[parent_cand_trackid]]
                                << " Its mother " << _mcpl.ParentTrackId()[trackid2index[parent_cand_trackid]]
                                << "\n";
                }
                throw meatloaf();
            }

            result.push_back(parent_trackid);
            accessed.insert(parent_trackid);
            if (_mcpl.ParentTrackId()[trackid2index[parent_trackid]] == parent_trackid) break;
            parent_trackid = _mcpl.ParentTrackId()[trackid2index[parent_trackid]];
        }
        return result;
    }


}
#endif
