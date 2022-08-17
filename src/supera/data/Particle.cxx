#include "Particle.h"

#include <algorithm>
#include <iostream>
#include <map>
#include <sstream>

namespace supera {


  std::string Particle::dump() const
  {
    std::stringstream ss;
    std::stringstream buf;
    ss  << "      \033[95m" << "Particle " << " (PdgCode,TrackID) = (" << pdg << "," << trackid << ")\033[00m "
        << "... with Parent (" << parent_pdg << "," << parent_trackid << ")" << std::endl;
    buf << "      ";

    ss << buf.str() << "Vertex   (x, y, z, t) = (" << vtx.pos.x << "," << vtx.pos.y << "," << vtx.pos.z << "," << vtx.time << ")" << std::endl
       << buf.str() << "Momentum (px, py, pz) = (" << px << "," << py << "," << pz << ")" << std::endl
       << buf.str() << "Initial Energy  = " << energy_init << std::endl
       << buf.str() << "Deposit  Energy  = " << energy_deposit << std::endl
       << buf.str() << "Creation Process = " << process << std::endl
       << buf.str() << "Group ID = " << group_id << std::endl
       << buf.str() << "Shape = " << shape << std::endl;
    ss << buf.str() << "Children =  ";
    for (const auto & child :  children_id)
      ss << " " << child;
    ss << std::endl;

    return ss.str();

  }


  std::string Particle::dump2cpp(const std::string &instanceName) const
  {
    std::stringstream ss;
    
    ss << "supera::Particle " << instanceName << ";\n";

    ss << instanceName << ".id = " << id << ";\n";
    ss << instanceName << ".shape = static_cast<supera::SemanticType_t>(" << shape << ");\n";
    ss << instanceName << ".trackid = " << trackid << ";\n";
    ss << instanceName << ".pdg = " << pdg << ";\n";
    ss << instanceName << ".px = " << px << ";\n";
    ss << instanceName << ".py = " << py << ";\n";
    ss << instanceName << ".pz = " << pz << ";\n";
    ss << instanceName << ".vtx = {" << vtx.pos.x << ", "
                                     << vtx.pos.y << ", "
                                     << vtx.pos.z << ", "
                                     << vtx.time << "};\n";
    ss << instanceName << ".end_pt = {" << end_pt.pos.x << ", "
                                        << end_pt.pos.y << ", "
                                        << end_pt.pos.z << ", "
                                        << end_pt.time << "};\n";
    ss << instanceName << ".first_step = {" << first_step.pos.x << ", "
                                            << first_step.pos.y << ", "
                                            << first_step.pos.z << ", "
                                            << first_step.time << "};\n";
    ss << instanceName << ".last_step = {" << last_step.pos.x << ", "
                                           << last_step.pos.y << ", "
                                           << last_step.pos.z << ", "
                                           << last_step.time << "};\n";
    ss << instanceName << ".dist_travel = " << dist_travel << ";\n";
    ss << instanceName << ".energy_init = " << energy_init << ";\n";
    ss << instanceName << ".energy_deposit = " << energy_deposit << ";\n";
    ss << instanceName << ".process = \"" << process << "\";\n";

    ss << instanceName << ".parent_trackid = " << parent_trackid << ";\n";
    ss << instanceName << ".parent_pdg = " << parent_pdg << ";\n";
    ss << instanceName << ".parent_vtx = {" << parent_vtx.pos.x << ", "
                                            << parent_vtx.pos.y << ", "
                                            << parent_vtx.pos.z << ", "
                                            << parent_vtx.time << "};\n";

    // this particle is at the top so it's its own ancestor
    ss << instanceName << ".ancestor_trackid = " << ancestor_trackid << ";\n";
    ss << instanceName << ".ancestor_pdg = " << ancestor_pdg << ";\n";
    ss << instanceName << ".ancestor_vtx = {" << ancestor_vtx.pos.x << ", "
                                              << ancestor_vtx.pos.y << ", "
                                              << ancestor_vtx.pos.z << ", "
                                              << ancestor_vtx.time << "};\n";
    ss << instanceName << ".ancestor_process = \"" << ancestor_process << "\";\n";

    ss << instanceName << ".parent_process = \"" << parent_process << "\";\n";
    ss << instanceName << ".parent_id = " << parent_id << ";\n";

    ss << instanceName << ".children_id = { ";
    for (const supera::InstanceID_t & chid : children_id)
      ss << chid << (chid != children_id.back() ? ", " : "");
    ss << " };\n";

    ss << instanceName << ".group_id = " << group_id << ";\n";
    ss << instanceName << ".interaction_id = " << interaction_id << ";\n";

    return ss.str();
  }


  bool Particle::operator==(const Particle &rhs) const
  {
    // Particles are the same if their data is the same...
    return (id == rhs.id) &&
           (shape == rhs.shape) &&
           (trackid == rhs.trackid) &&
           (pdg == rhs.pdg) &&
           (px == rhs.px) &&
           (py == rhs.py) &&
           (pz == rhs.pz) &&
           (vtx == rhs.vtx) &&
           (end_pt == rhs.end_pt) &&
           (first_step == rhs.first_step) &&
           (last_step == rhs.last_step) &&
           (dist_travel == rhs.dist_travel) &&
           (energy_init == rhs.energy_init) &&
           (energy_deposit == rhs.energy_deposit) &&
           (process == rhs.process) &&
           (parent_trackid == rhs.parent_trackid) &&
           (parent_pdg == rhs.parent_pdg) &&
           (parent_vtx == rhs.parent_vtx) &&
           (ancestor_trackid == rhs.ancestor_trackid) &&
           (ancestor_pdg == rhs.ancestor_pdg) &&
           (ancestor_vtx == rhs.ancestor_vtx) &&
           (ancestor_process == rhs.ancestor_process) &&
           (parent_process == rhs.parent_process) &&
           (parent_id == rhs.parent_id) &&
           (children_id == rhs.children_id) &&
           (group_id == rhs.group_id) &&
           (interaction_id == rhs.interaction_id);
  }

  // --------------------------------------------------------

  std::string ParticleInput::dump2cpp(const std::string &instanceName) const
  {
    std::stringstream ss;

    ss << "supera::ParticleInput " << instanceName << ";\n";

    ss << instanceName << ".pcloud.reserve(" << pcloud.size() << ");\n";
    for (std::size_t idx = 0; idx < pcloud.size(); idx++)
    {
      std::string edepInstance = instanceName + "_edep" + std::to_string(idx);
      std::string edepCode = pcloud[idx].dump2cpp(edepInstance);
      ss << edepCode;
      ss << instanceName << ".pcloud.emplace_back(std::move(" << edepInstance << "));\n";
    }

    ss << instanceName << ".valid = " << valid << ";\n";
    ss << instanceName << ".type = static_cast<supera::ProcessType>(" << type << ");\n";

    std::string partInstance = instanceName + "_particle";
    ss << part.dump2cpp(partInstance);
    ss << instanceName << ".part = std::move(" << partInstance << ");\n";

    return ss.str();
  }

  // --------------------------------------------------------

  ParticleLabel::ParticleLabel()
  : valid(false)
  , add_to_parent(false)
  , type(supera::kInvalidProcess)
  {}

  void ParticleLabel::AddEDep(const EDep& pt)
  { 
    if(pt.x == supera::kINVALID_DOUBLE) return; 
    if(pt.t < first_pt.t) first_pt = pt; 
    if(pt.t > last_pt.t) last_pt = pt;
  }

  void ParticleLabel::SizeCheck() const
  {

    if(dedx.size() && energy.size() != dedx.size()) {
      std::cerr << "Size mismatch: " << energy.size() << " v.s. " << dedx.size() << std::endl;
      throw std::exception();
    }

  }


  size_t ParticleLabel::Size() const
  { 
    SizeCheck();
    return energy.size();
  }

  void ParticleLabel::Merge(ParticleLabel& child,bool verbose) {

    for(auto const& vox : child.energy.as_vector())
      this->energy.emplace(vox.id(),vox.value(),true);

    for(auto const& vox : child.dedx.as_vector())
      this->dedx.emplace(vox.id(),vox.value(),true);
    
    if(verbose) {
      /*
      std::cout<<"Parent track id " << this->part.track_id() 
      << " PDG " << this->part.pdg << " " << this->part.creation_process() << std::endl
      << "  ... merging " << child.part.track_id()
      << " PDG " << child.part.pdg << " " << child.part.creation_process() << std::endl;
      */
    }

    this->AddEDep(child.last_pt);
    this->AddEDep(child.first_pt);
    //this->trackid_v.push_back(child.part.track_id());

    for(auto const& trackid : child.trackid_v)
      this->trackid_v.push_back(trackid);
    /*
    for(size_t plane_id=0; plane_id < vs2d_v.size(); ++plane_id) {
      auto& vs2d = vs2d_v[plane_id];
      auto& child_vs2d = child.vs2d_v[plane_id];
      for(auto const& vox : child_vs2d.as_vector())
        vs2d.emplace(vox.id(),vox.value(),true);
      child_vs2d.clear_data();
    }
    */
    child.energy.clear_data();
    child.dedx.clear_data();
    
    child.valid=false;
  }

  // semantic classification (supera::SemanticType_t)
  
  supera::SemanticType_t ParticleLabel::shape() const
  {

    // identify delta ray
    if(type == kInvalidProcess) return supera::kShapeUnknown;
    if(type == kDelta) return supera::kShapeDelta;
    if(type == kNeutron) //return supera::kShapeUnknown;
    return supera::kShapeLEScatter;
    if(part.pdg == 11 || part.pdg == 22 || part.pdg == -11) {
      if(type == kComptonHE || type == kPhoton || type == kPrimary || type == kConversion || type==kOtherShowerHE)
        return supera::kShapeShower;
      if(type == kDecay) {
        if(part.parent_pdg == 13 || part.parent_pdg == -13)
          return supera::kShapeMichel;
        else
          return supera::kShapeShower;
      }
      return supera::kShapeLEScatter;
    }else
    return supera::kShapeTrack;
  }


  std::string ParticleLabel::dump2cpp(const std::string &instanceName) const
  {
    std::stringstream ss;

    ss << "supera::ParticleLabel " << instanceName << ";\n";

    std::string partInstance = instanceName + "_part";
    ss << part.dump2cpp(partInstance);
    ss << instanceName << ".part = std::move(" << partInstance << ");\n";

    ss << instanceName << ".valid = " << valid << ";\n";
    ss << instanceName << ".add_to_parent = " << add_to_parent << ";\n";
    ss << instanceName << ".type = static_cast<supera::ProcessType>(" << type << ");\n";

    ss << instanceName << ".trackid_v = { ";
    for (std::size_t tkid : trackid_v)
      ss << tkid << (tkid == trackid_v.back() ? "" : ", ");
    ss << " };\n";

    std::string energyInstance = instanceName + "_energyVoxSet";
    ss << energy.dump2cpp(energyInstance);
    ss << instanceName << ".energy = std::move(" << energyInstance << ");\n";

    std::string dedxInstance = instanceName + "_dedxVoxSet";
    ss << dedx.dump2cpp(dedxInstance);
    ss << instanceName << ".dedx = std::move(" << dedxInstance << ");\n";

    std::string firstptInstance = instanceName + "_firstEdep";
    ss << first_pt.dump2cpp(firstptInstance);
    ss << instanceName << ".first_pt = std::move(" << firstptInstance << ");\n";

    std::string lastptInstance = instanceName + "_lastEdep";
    ss << last_pt.dump2cpp(lastptInstance);
    ss << instanceName << ".last_pt = std::move(" << lastptInstance << ");\n";

    return ss.str();
  }

  bool ParticleLabel::operator==(const ParticleLabel &rhs) const
  {
    // ParticleLabels equivalent if all their data match...
    return (part == rhs.part) &&
           (valid == rhs.valid) &&
           (add_to_parent == rhs.add_to_parent) &&
           (type == rhs.type) &&
           (trackid_v == rhs.trackid_v) &&
           (energy == rhs.energy) &&
           (dedx == rhs.dedx) &&
           (first_pt == rhs.first_pt) &&
           (last_pt == rhs.last_pt);

  }

  // --------------------------------------------------------

  const supera::VoxelSet &EventOutput::VoxelDeDxs() const
  {
    // recompute only if particle list has changed under us
    if (IsDirty(DIRTY_FLAG::kDeDx))
    {
      // we want an energy-weighted mean here.
      // first add all the dEdXs up, weighted by their energies...
      supera::VoxelSet dEdXs;
      for (const supera::ParticleLabel & part : Particles())
      {
        for (const supera::Voxel & dedx : part.dedx.as_vector())
          dEdXs.emplace(dedx.id(), dedx.value() * VoxelEnergies().find(dedx.id()).value(), true);
      }

      // now renormalize them...
      for (const supera::Voxel & dedx : dEdXs.as_vector())
      {
        auto energy = VoxelEnergies().find(dedx.id()).value();
        if (energy > 0)
          dEdXs.emplace(dedx.id(), dedx.value() / energy, false);
      }

      _dEdXs = std::move(dEdXs);
    } // if (IsDirty(...))

    return _dEdXs;
  }

  // --------------------------------------------------------

  const supera::VoxelSet &EventOutput::VoxelEnergies() const
  {
    // recompute only if particle list has changed under us
    if (IsDirty(DIRTY_FLAG::kEnergy))
    {
      supera::VoxelSet energies;
      for (const supera::ParticleLabel & part : Particles())
        energies.emplace(part.energy, true);
      _energies = std::move(energies);
    }

    return _energies;
  }

  // --------------------------------------------------------

  const supera::VoxelSet &
  EventOutput::VoxelLabels(const std::vector<supera::SemanticType_t> &semanticPriority) const
  {
    // recompute only if particle list has changed under us
    if (IsDirty(DIRTY_FLAG::kLabel))
    {
      supera::VoxelSet semantics;
      for (const supera::ParticleLabel & part : Particles())
      {
        auto const &vs = part.energy;
        SemanticType_t semantic = part.shape();
        for (auto const &vox : vs.as_vector())
        {
          auto const &prev = semantics.find(vox.id());
          if (prev.id() == supera::kINVALID_VOXELID)
            semantics.emplace(vox.id(), semantic, false);
          else
          {
            // todo: what if the new voxel has 10x the energy??
            SemanticType_t prioritized_semantic = EventOutput::_SemanticPriority(static_cast<SemanticType_t>(prev.value()), semantic, semanticPriority);
            if (prioritized_semantic != static_cast<SemanticType_t>(prev.value()))
              semantics.emplace(vox.id(), semantic, false);
          }
        } // for (vox)
      } // for (part)

      _semanticLabels = std::move(semantics);
    } // if (IsDirty(...))

    return _semanticLabels;
  }

  // --------------------------------------------------------

  supera::SemanticType_t EventOutput::_SemanticPriority(supera::SemanticType_t a, supera::SemanticType_t b,
                                                        const std::vector<supera::SemanticType_t> & semanticPriority)
  {
    if (a == b)
      return a;
    for (auto const &semantic : semanticPriority)
    {
      if (a == semantic)
        return a;
      if (b == semantic)
        return b;
    }
    return a;
  }

  // --------------------------------------------------------

  bool EventOutput::operator==(const EventOutput &rhs) const
  {
    // the event outputs are the same if their ParticleLabels are the same.

    // definitely different if different lengths
    if (_particles.size() != rhs._particles.size())
      return false;

    // the ParticleLabels might not be in the same order,
    // but if the *contents* are the same, they're still equivalent.
    // we'll sort them by GEANT trackID using a map from track id -> vector index
    // (since maps are inherently sorted).
    // then we can walk through the track indices, check those first,
    // and only if they are equal do the actual comparison between ParticleList objects.
    std::map<supera::TrackID_t, std::size_t> lhs_tracks, rhs_tracks;
    for (std::size_t idx = 0; idx < _particles.size(); idx++)
    {
      lhs_tracks[_particles[idx].part.trackid] = idx;
      rhs_tracks[rhs._particles[idx].part.trackid] = idx;
    }

    auto it_lhs = lhs_tracks.begin();
    auto it_rhs = rhs_tracks.begin();
    for (; it_lhs != lhs_tracks.end() && it_rhs != rhs_tracks.end(); it_lhs++, it_rhs++ )
    {
      // since the map is sorted, any mismatch in track IDs already means they're not equal
      if (it_lhs->first != it_rhs->first)
        return false;

      // if we have to, check the objects themselves
      if (_particles[it_lhs->second] != rhs._particles[it_rhs->second])
        return false;
    }

    return true;

  }

  // --------------------------------------------------------

  std::string EventOutput::dump2cpp(const std::string &instanceName) const
  {
    std::stringstream ss;

    ss << "supera::EventOutput " << instanceName << ";\n";

    ss << instanceName << ".Particles().reserve(" << _particles.size() << ");\n";
    for (std::size_t idx = 0; idx < _particles.size(); idx++)
    {
      const supera::ParticleLabel & part = _particles[idx];
      std::string partInstance = instanceName + "_part" + std::to_string(idx) + "_label";
      ss << part.dump2cpp(partInstance);
      ss << instanceName << ".Particles().push_back(std::move(" << partInstance << "));\n";
    }

    return ss.str();
  }
} // namespace supera
