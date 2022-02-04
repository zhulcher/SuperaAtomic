#ifndef __SUPERADATA_H__ 
#define __SUPERADATA_H__ 

#include "SuperaType.h"
#include "larcv3/core/dataformat/Voxel.h"


namespace supera {

  typedef larcv3::VoxelSet VoxelSet;

  /*
    3D point information with energy deposit and dE/dX
  */
  class EDep {
  public:
    EDep()
    { x = y = z = t = e = dedx = supera::kINVALID_DOUBLE; }

    double x,y,z,t,e,dedx; ///< (x,y,z) position, time, energy, dE/dX in respective order
  };

  /*
    Particle information
  */
  class ParticleGroup {
  public:
    ParticleGroup(size_t num_planes=0);
    void AddEDep(const EDep& pt);
    void SizeCheck() const;
    size_t Size() const;
    void Merge(ParticleGroup& child,bool verbose=false);
    /*
    larcv::ShapeType_t shape() const; ///< to be moved into Supera2LArCV
    */

    bool valid; ///< a state flag whether this particle should be ignored or not
    bool add_to_parent; ///< a state flag whether this particle should be merged into its parent
    ProcessType type; ///< type of this particle for ML reco chain
    std::vector<size_t> trackid_v; ///< track ID of descendent particles
    
    supera::VoxelSet vs;   ///< 3D voxels (energy deposition)
    supera::VoxelSet dedx; ///< 3D voxels (dE/dX)
    std::vector<supera::VoxelSet> vs2d_v; ///< 3D voxels from projected planes (for wire LArTPC)

    EDep first_pt; ///< first energy deposition point (not voxel)
    EDep last_pt;  ///< last energy deposition point (not voxel)
  };


  
}

#endif