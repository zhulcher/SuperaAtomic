#include "Particle.h"

#include <algorithm>
#include <iostream>
#include <map>
#include <sstream>
#include "supera/base/meatloaf.h"

namespace supera {


  std::string Particle::dump() const
  {
    std::stringstream ss;
    std::stringstream buf;
    ss  << "      \033[95m" << "Particle " << " (PdgCode,TrackID) = (" << pdg << "," << StringifyTrackID(trackid) << ")\033[00m " << std::endl
        << "      ... with Parent (" << parent_pdg << "," << StringifyTrackID(parent_trackid) << ")" << std::endl
        << "      ... + Ancestor  (" << ancestor_pdg << "," << StringifyTrackID(ancestor_trackid) << ")" << std::endl;

    buf << "      ";

    ss << buf.str() << "Vertex   (x, y, z, t) = (" << vtx.pos.x << "," << vtx.pos.y << "," << vtx.pos.z << "," << vtx.time << ")" << std::endl
       << buf.str() << "Momentum (px, py, pz) = (" << px << "," << py << "," << pz << ")" << std::endl
       << buf.str() << "Initial  Energy  = " << energy_init << std::endl
       << buf.str() << "Deposit  Energy  = " << energy_deposit << std::endl
       << buf.str() << "Creation Process = " << process << std::endl
       << buf.str() << "Instance ID      = " << StringifyInstanceID(id) << std::endl
       << buf.str() << "Group ID         = " << StringifyInstanceID(group_id) << std::endl
       << buf.str() << "Interaction ID   = " << StringifyInstanceID(interaction_id) << std::endl
       << buf.str() << "Type     = " << type << std::endl
       << buf.str() << "Shape    = " << shape << std::endl;
    ss << buf.str() << "Children = ";
    for (const auto & child :  children_id)
      ss << " " << StringifyInstanceID(child);
    ss << std::endl;

    return ss.str();

  }


  std::string Particle::dump2cpp(const std::string &instanceName) const
  {
    std::stringstream ss;
    
    ss << "supera::Particle " << instanceName << ";\n";

    ss << instanceName << ".id = " << id << ";\n";
    ss << instanceName << ".shape = static_cast<supera::SemanticType_t>(" << shape << ");\n";
    ss << instanceName << ".trackid = " << StringifyTrackID(trackid) << ";\n";
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

    ss << instanceName << ".parent_trackid = " << StringifyTrackID(parent_trackid) << ";\n";
    ss << instanceName << ".parent_pdg = " << parent_pdg << ";\n";
    ss << instanceName << ".parent_vtx = {" << parent_vtx.pos.x << ", "
                                            << parent_vtx.pos.y << ", "
                                            << parent_vtx.pos.z << ", "
                                            << parent_vtx.time << "};\n";

    // this particle is at the top so it's its own ancestor
    ss << instanceName << ".ancestor_trackid = " << StringifyTrackID(ancestor_trackid) << ";\n";
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
      ss << StringifyInstanceID(chid) << (chid != children_id.back() ? ", " : "");
    ss << " };\n";

    ss << instanceName << ".group_id = " << StringifyInstanceID(group_id) << ";\n";
    ss << instanceName << ".interaction_id = " << StringifyInstanceID(interaction_id) << ";\n";

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

    std::string partInstance = instanceName + "_particle";
    ss << part.dump2cpp(partInstance);
    ss << instanceName << ".part = std::move(" << partInstance << ");\n";

    return ss.str();
  }

  // --------------------------------------------------------

  ParticleLabel::ParticleLabel()
  : valid(false)
  , merge_id(supera::kINVALID_TRACKID)
  {}

  void ParticleLabel::UpdateFirstPoint(const EDep& pt)
  { 
    if(pt.x == supera::kINVALID_DOUBLE) return; 
    if(pt.t < first_pt.t || first_pt.t == supera::kINVALID_DOUBLE ) 
      { 
        /*
        std::cout << "Updating track " << part.trackid << " first pt " << first_pt.t << " => " << pt.t << std::endl;
        std::cout << "    " << first_pt.x << "," << first_pt.y << "," << first_pt.z 
        << " => "
        << pt.x << "," << pt.y << "," << pt.z 
        << std::endl;
        */
        first_pt = pt; 
      } 
  }

  void ParticleLabel::UpdateLastPoint(const EDep& pt)
  { 
    if(pt.x == supera::kINVALID_DOUBLE) return; 
    if(pt.t > last_pt.t  || last_pt.t  == supera::kINVALID_DOUBLE ) 
      { 
        /*
        std::cout << "Updating track " << part.trackid << " last  pt " << last_pt.t << " => " << pt.t << std::endl;
        std::cout << "    " << last_pt.x << "," << last_pt.y << "," << last_pt.z 
        << " => "
        << pt.x << "," << pt.y << "," << pt.z 
        << std::endl;
        */
        last_pt  = pt; 
      }
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
    //std::cout<<"Merging child track " << child.part.trackid << " into parent track " << part.trackid << std::endl;
    if(!(this->valid)) {
      std::cerr<<"Cannot merge into an invalid parent!" << std::endl;
      std::cerr<<"Parent info..."<<std::endl<<this->dump()<<std::endl;
      std::cerr<<"Child info..."<<std::endl<<child.dump()<<std::endl;
      throw meatloaf();
    }
    
    for(auto const& vox : child.energy.as_vector())
      this->energy.emplace(vox.id(),vox.value(),true);
    for(auto const& vox : child.dedx.as_vector())
      this->dedx.emplace(vox.id(),vox.value(),true);
    
    if(verbose) {
      std::cout<<"Parent track id " << this->part.trackid
      << " PDG " << this->part.pdg << " proc " << this->part.process << " shape " << this->part.shape << std::endl
      << "  ... merging " << child.part.trackid
      << " PDG " << child.part.pdg << " proc " << child.part.process << " shape " << child.part.shape << std::endl;
    }

    if(child.part.shape == kShapeTrack || child.part.shape == kShapeShower)
      this->UpdateFirstPoint(child.first_pt);
    if(child.part.shape == kShapeTrack) {
      //std::cout<<child.dump2cpp()<<std::endl;
      this->UpdateLastPoint(child.last_pt);
    }
    this->merged_v.push_back(child.part.trackid);
    for(auto const& trackid : child.merged_v)
      this->merged_v.push_back(trackid);
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
    child.merge_id=this->part.trackid;
  }

  // semantic classification (supera::SemanticType_t)
  
  /*
  supera::SemanticType_t ParticleLabel::shape() const
  {

    if(type == kInvalidProcess) return supera::kShapeUnknown;
    if(type == kDelta) return supera::kShapeDelta;
    if(type == kNeutron) //return supera::kShapeUnknown;

    if(part.pdg == 11 || part.pdg == 22 || part.pdg == -11) {
      if(type == kCompton || type == kPhoton || type == kPrimary || type == kConversion || type==kOtherShower)
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
*/

  std::string ParticleLabel::dump2cpp(const std::string &instanceName) const
  {
    std::stringstream ss;

    ss << "supera::ParticleLabel " << instanceName << ";\n";

    std::string partInstance = instanceName + "_part";
    ss << part.dump2cpp(partInstance);
    ss << instanceName << ".part = std::move(" << partInstance << ");\n";
    ss << instanceName << ".valid = " << valid << ";\n";
    ss << instanceName << ".merged_v = { ";
    for (std::size_t tkid : merged_v)
      ss << tkid << (tkid == merged_v.back() ? "" : ", ");
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
           (merged_v == rhs.merged_v) &&
           (energy == rhs.energy) &&
           (dedx == rhs.dedx) &&
           (first_pt == rhs.first_pt) &&
           (last_pt == rhs.last_pt);

  }

  std::string ParticleLabel::dump() const
  {
    std::stringstream st;

    st << "ParticleLabel object:\n";
    st << "  valid = " << valid << "; \n";
    st << "  num voxels: " << energy.size() << "\n";
    st << "  merged into track ID: " << merge_id << "\n";
    st << "  merged track IDs: ";
    for (const TrackID_t tk : merged_v)
      st << tk << " ";
    st << "\n";
    st << "  Particle:\n";
    st << part.dump() << "\n";

    return st.str();
  }

} // namespace supera
