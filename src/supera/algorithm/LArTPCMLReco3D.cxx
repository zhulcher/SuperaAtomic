#ifndef __LARTPCMLRECO3D_CXX__
#define __LARTPCMLRECO3D_CXX__

#include "LArTPCMLReco3D.h"

#include <cassert>
#include <set>

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


        // For shower orphans, we need to register the most base shower particle in the output (for group)
        this->FixOrphanShowerGroups(particles, output2trackid, labels, trackid2output);


        // For LEScatter orphans, we need to register the immediate valid (=to be stored) particle
        this->FixOrphanNonShowerGroups(particles, output2trackid, labels, trackid2output);


        // for shower particles with invalid parent ID, attempt a search
        this->FixInvalidParentShowerGroups(particles, labels, trackid2output, output2trackid);


        // Now sort out all parent IDs where it's simply not assigned
        // (it's ok to have invalid parent id if parent track id is not stored)
        this->FixUnassignedParentGroups(labels, trackid2output, output2trackid);

        // Now loop over otuput particle list and check if any remaining group id needs to be assigned
        // Use its parent to group...
        this->FixUnassignedGroups(labels, output2trackid);

        // Next handle LEScatter group id if not assigned yet
        this->FixUnassignedLEScatterGroups(labels, output2trackid);

        // Next loop over to find any particle for which first_step is not defined
        this->FixFirstStepInfo(labels, meta3d, output2trackid);


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
                    LOG.VERBOSE() << "  Dropping below-threshold voxel " << vox.id() << " with edep = " << vox.value();
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
        LOG.VERBOSE() << "Considering incoming particle groups:";
        for (auto & inputLabel : inputLabels)
        {
            LOG.VERBOSE() << " Particle ID=" << inputLabel.part.id << " Track ID=" << inputLabel.part.trackid;
            LOG.VERBOSE() << "     Edep=" << inputLabel.part.energy_deposit;
            size_t output_counter = output2trackid.size();
            if (!inputLabel.valid)
            {
                LOG.VERBOSE() << "   --> invalid group (i.e. already merged), skipping";
                continue;
            }
            if (inputLabel.part.process != "primary" && inputLabel.Size() < 1)
            {
                LOG.VERBOSE() << "   --> no voxels, skipping";
                continue;
            }
            // Also define particle "first step" and "last step"
            auto &part = inputLabel.part;
            auto const &first_pt = inputLabel.first_pt;
            auto const &last_pt = inputLabel.last_pt;
            LOG.VERBOSE() << "      examining true particle start:" << first_pt.x<< " " << first_pt.y << " " << first_pt.z;
            if (first_pt.t != kINVALID_DOUBLE)
                part.first_step = supera::Vertex(first_pt.x, first_pt.y, first_pt.z, first_pt.t);
            if (last_pt.t != kINVALID_DOUBLE)
                part.last_step = supera::Vertex(last_pt.x, last_pt.y, last_pt.z, last_pt.t);
            LOG.VERBOSE() << "     true particle start: " << inputLabel.part.first_step.dump()
                          << "                   end: " << inputLabel.part.last_step.dump();
            inputLabel.part.energy_deposit = inputLabel.energy.size() ? inputLabel.energy.sum() : 0.;


            //if (grp.part.process != "primary" && grp.shape() == kShapeLEScatter)
            //{
            //  LOG.VERBOSE() << "   --> LEScatter shape, skipping" << std::endl;
            //  continue;
            //}

            inputLabel.part.id = output_counter;
            LOG.VERBOSE() << "   --> Assigned output group id = " << inputLabel.part.id;
            trackid2output[inputLabel.part.trackid] = static_cast<int>(output_counter);
            for (auto const &child : inputLabel.trackid_v)
                trackid2output[child] = static_cast<int>(output_counter);
            output2trackid.push_back(static_cast<int>(inputLabel.part.trackid));
            ++output_counter;
        }

        LOG.VERBOSE() << "trackid2output (i.e., map of track IDs to output group IDs) contents:";
        for (std::size_t idx = 0; idx < trackid2output.size(); idx++)
            LOG.VERBOSE() << "   " << idx << " -> " << trackid2output[idx];

        // now assign relationships
        LOG.VERBOSE() << "Assigning group relationships:";
        for (auto const &trackid : output2trackid)
        {
            auto &inputLabel = inputLabels[trackid];
            LOG.VERBOSE() << "  Group for trackid=" << trackid
                          << " (pdg = " << inputLabel.part.pdg << ", particle id=" << inputLabel.part.id << ")";
            if (std::abs(inputLabel.part.pdg) != 11 && std::abs(inputLabel.part.pdg) != 22)
            {
                LOG.VERBOSE() << "    ---> not EM, leaving alone (parent id=" << inputLabel.part.parent_id
                              << " and group id=" << inputLabel.part.group_id << ")";
                continue;
            }

            unsigned int parent_trackid = inputLabel.part.parent_trackid;
            LOG.VERBOSE() << "   initial parent track id:" << parent_trackid;

            if (parent_trackid != supera::kINVALID_UINT && trackid2output[parent_trackid] >= 0)
            {
                LOG.VERBOSE() << "   --> assigning group for trackid " << trackid << " to parent trackid: " << parent_trackid;
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
                              << " (group id=" << inputLabel.part.group_id << ")";
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
            LOG.VERBOSE() << "Assigned primary particle's own ID to its group and parent IDs:\n" << part.dump();
        }

    } // LArTPCMLReco3D::AssignParticleGroupIDs()


    // ------------------------------------------------------
    
    void LArTPCMLReco3D::DumpHierarchy(size_t trackid, const std::vector<supera::ParticleLabel>& inputLabels) const
    {
        assert(trackid < inputLabels.size());

        auto const &label = inputLabels[trackid];
        LOG.VERBOSE() << "\n#### Dumping particle record for track id "
                      << label.part.trackid << " ####";
        LOG.VERBOSE() << "id " << label.part.id << " from " << label.part.parent_id << "\n"
                      << "children: ";
        for (auto const &child : label.part.children_id)
            LOG.VERBOSE() <<  "   " << child;
        LOG.VERBOSE() << label.part.dump();

        size_t parent_trackid = label.part.parent_trackid;
        while (parent_trackid < inputLabels.size())
        {

            auto const &parent = inputLabels[parent_trackid];
            LOG.VERBOSE() << "Parent's group id: " << parent.part.group_id << " valid? " << parent.valid;
            LOG.VERBOSE() << "Parent's children: " ;
            for (auto const &child : parent.part.children_id)
                LOG.VERBOSE() << "    " << child;
            LOG.VERBOSE() << parent.part.dump();
            if (parent_trackid == parent.part.parent_trackid)
                break;
            if (parent_trackid == supera::kINVALID_UINT)
                break;
            parent_trackid = parent.part.parent_trackid;
        }
        LOG.VERBOSE() << "\n\n#### Dump done ####";
    } // LArTPCMLReco3D::DumpHierarchy()

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
                    LOG.VERBOSE() << "Inspecting: " << label.part.trackid << " => " << parent_index;
                    if (parent_index == supera::kINVALID_UINT)
                    {
                        LOG.VERBOSE() << "Invalid parent track id " << parent_index
                                      << " Could not find a parent for " << label.part.trackid << " PDG " << label.part.pdg
                                      << " " << label.part.process << " E = " << label.part.energy_init
                                      << " (" << label.part.energy_deposit << ") MeV";
                        auto const &parent = labels[parent_index_before].part;
                        LOG.VERBOSE() << "Previous parent: " << parent.trackid << " PDG " << parent.pdg
                                      << " " << parent.process;
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
                            LOG.INFO() << "Particle " << parent_index << " is root and invalid particle...";
                            LOG.INFO() << "PDG " << parent.part.pdg << " " << parent.part.process;
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
            LOG.INFO() << "Merge counter: " << merge_ctr << " invalid counter: " << invalid_ctr;
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
                              << " PDG " << parent.part.pdg << " " << parent.part.process;
                LOG.VERBOSE() << "Time difference: " << label.part.first_step.time - parent.part.first_step.time;
                parent.Merge(label, parent.shape() != kShapeTrack);  // a delta ray is unlikely to extend a *track* in the up- or downstream directions
            }
            else
            {
                LOG.VERBOSE() << "NOT merging delta " << label.part.trackid << " PDG " << label.part.pdg
                              << " " << label.part.process << " vox count " << label.energy.size() << "\n"
                              <<" ... parent found " << parent.part.trackid
                              << " PDG " << parent.part.pdg << " " << parent.part.process;

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
                              << ", and alleged parent track id=" << label.part.parent_trackid;
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
                    LOG.VERBOSE() << "   Merged to group w/ track id=" << parent.part.trackid;
                    merge_ctr++;
                }
            }
            LOG.INFO() << "Merge counter: " << merge_ctr << " invalid counter: " << invalid_ctr;
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
                                  << " (" << label.part.energy_deposit << ") MeV";
                        auto const &parent = labels[parent_index_before].part;
                        std::cout << "Previous parent: " << parent.trackid << " PDG " << parent.pdg
                                      << " " << parent.process;
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
                                         << "PDG " << parent.part.pdg << " " << parent.part.process;
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
            LOG.INFO() << "Ionization merge counter: " << merge_ctr << " invalid counter: " << invalid_ctr;
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
            LOG.INFO() << "Merge counter: " << merge_ctr;
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
                            << " " << label.part.process;
                LOG.VERBOSE() << "  ... parents:";
                for(auto const& parent_trackid : parents)
                    LOG.VERBOSE() << "     "<< parent_trackid;

                for (auto const &parent_trackid : parents)
                {
                    auto &parent = labels[parent_trackid];
                    if (!parent.valid || parent.energy.size() < 1) continue;
                    if (this->IsTouching(meta, label.energy, parent.energy))
                    {
                        LOG.VERBOSE() << "Merging LEScatter track id = " << label.part.trackid
                                    << " into touching parent shower group (id=" << parent.part.group_id << ")"
                                    << " with track id = " << parent.part.trackid;
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
                    LOG.VERBOSE()<<"Touching ("<<ix1<<","<<iy1<<","<<iz1<<") ("<<ix2<<","<<iy2<<","<<iz2<<")";
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
                LOG.FATAL() << "LOOP-LOGIC-ERROR for ParentTrackIDs for track id " << trackid;
                for (size_t parent_cand_idx = 0; parent_cand_idx < result.size(); ++parent_cand_idx)
                {
                    auto const &parent_cand_trackid = result[parent_cand_idx];
                    LOG.FATAL() << "Parent " << parent_cand_idx
                                << " Track ID " << parent_trackid
                                << " PDG " << _mcpl.PdgCode()[trackid2index[parent_trackid]]
                                << " Mother " << _mcpl.ParentTrackId()[trackid2index[parent_trackid]];
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
