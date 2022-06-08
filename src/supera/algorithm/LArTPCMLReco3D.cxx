#ifndef __LARTPCMLRECO3D_CXX__
#define __LARTPCMLRECO3D_CXX__

#include "LArTPCMLReco3D.h"

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
        this->MergeShowerTouching(meta, labels, particles);
        this->MergeShowerDeltas(labels);

        // output containers
        std::vector<int> trackid2output(trackid2index.size(), -1);
        std::vector<int> output2trackid;

        // Assign output IDs and relationships
        this->AssignParticleGroupIDs(trackid2index, output2trackid, labels, trackid2output);


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
                    LOG.DEBUG() << "  Dropping below-threshold voxel " << vox.id() << " with edep = " << vox.value();
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
                //std::cout<<"Track ID "<<grp.part.track_id()<<" high energy compton"<<std::endl;
                label.type = supera::kComptonHE;
            } else if (label.type == supera::kOtherShower && label.energy.size() > _compton_size)
            {
                //std::cout<<"Track ID "<<grp.part.track_id()<<" high energy compton"<<std::endl;
                label.type = supera::kOtherShowerHE;
            }
        }
    } // LArTPCMLReco3D::ApplyEnergyThreshold()


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

    void LArTPCMLReco3D::MergeShowerConversion(std::vector<supera::ParticleLabel>& labels)
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
                    LOG.DEBUG() << "Inspecting: " << label.part.trackid << " => " << parent_index;
                    if (parent_index == supera::kINVALID_UINT)
                    {
                        LOG.DEBUG() << "Invalid parent track id " << parent_index
                                      << " Could not find a parent for " << label.part.trackid << " PDG " << label.part.pdg
                                      << " " << label.part.process << " E = " << label.part.energy_init
                                      << " (" << label.part.energy_deposit << ") MeV";
                        auto const &parent = labels[parent_index_before].part;
                        LOG.DEBUG() << "Previous parent: " << parent.trackid << " PDG " << parent.pdg
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

    void LArTPCMLReco3D::MergeShowerFamilyTouching(const supera::ImageMeta3D& meta,
                                                   std::vector<supera::ParticleLabel>& labels)
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
                LOG.DEBUG() << "   Found particle group with shape 'shower', PDG=" << label.part.pdg
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
                //auto parent_type = part_grp_v[parent_trackid].type;
                //if(parent_type == supera::kTrack || parent_type == supera::kNeutron) continue;
                if (parent.shape() != supera::kShapeShower && parent.shape() != supera::kShapeDelta && parent.shape() != supera::kShapeMichel) continue;
                if (this->IsTouching(meta, label.energy, parent.energy)) {
                    // if parent is found, merge
                    parent.Merge(label);
                    LOG.DEBUG() << "   Merged to group w/ track id=" << parent.part.trackid;
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
                    //std::cout<< "Inspecting: " << label.part.track_id() << " => " << parent_index << std::endl;
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

    void LArTPCMLReco3D::MergeShowerTouchingLEScatter(const supera::ImageMeta3D& meta,
                                                      std::vector<supera::ParticleLabel>& labels)
    {
        size_t merge_ctr = 1;
        while (merge_ctr)
        {
            merge_ctr = 0;
            for (auto &label : labels)
            {
                if (!label.valid || label.energy.size() < 1 || label.shape() != supera::kShapeLEScatter) continue;

                auto const &parents = this->ParentTrackIDs(label.part.trackid);

                LOG.DEBUG() << "Inspecting LEScatter Track ID " << label.part.trackid
                            << " PDG " << label.part.pdg
                            << " " << label.part.process;
                LOG.DEBUG() << "  ... parents:";
                for(auto const& parent_trackid : parents)
                    LOG.DEBUG() << "     "<< parent_trackid;

                for (auto const &parent_trackid : parents)
                {
                    auto &parent = labels[parent_trackid];
                    if (!parent.valid || parent.energy.size() < 1) continue;
                    if (this->IsTouching(meta, label.energy, parent.energy))
                    {
                        LOG.DEBUG() << "Merging LEScatter track id = " << label.part.trackid
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
                    LOG.DEBUG()<<"Touching ("<<ix1<<","<<iy1<<","<<iz1<<") ("<<ix2<<","<<iy2<<","<<iz2<<")";
                    break;
                }
            }
            if (touching) break;
        }

        return touching;
    } // LArTPCMLReco3D::IsTouching()

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
