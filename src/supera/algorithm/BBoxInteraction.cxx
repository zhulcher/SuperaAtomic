#ifndef __BBOXINTERACTION_CXX__
#define __BBOXINTERACTION_CXX__

#include <sys/time.h>
#include <assert.h>
#include "BBoxInteraction.h"

namespace supera {


  void BBoxInteraction::Configure(const PSet& cfg)
  {

    int seed = cfg.get<int>("Seed",-1);
    if(seed < 0) {
      // use time seed
      struct timeval time; 
      gettimeofday(&time,NULL);
      _seed = (size_t)(time.tv_sec * 100 + time.tv_usec / 100);
    }else
      _seed = (size_t)(seed);

    auto bbox_size = cfg.get<std::vector<double> >("BBoxSize");
    assert(bbox_size.size() == 3);
    _xlen = bbox_size.at(0);
    _ylen = bbox_size.at(1);
    _zlen = bbox_size.at(2);
    
    auto voxel_size = cfg.get<std::vector<double> >("VoxelSize");
    assert(voxel_size.size() == 3);
    _xvox = voxel_size.at(0);
    _yvox = voxel_size.at(1);
    _zvox = voxel_size.at(2);

    std::vector<double> bbox_bottom;
    bbox_bottom = cfg.get<std::vector<double>>("BBoxBottom", bbox_bottom);
    assert(bbox_bottom.size()<1 || bbox_bottom.size() == 3);
    if(bbox_bottom.size()) {
      _bbox_bottom.x = bbox_bottom[0];
      _bbox_bottom.y = bbox_bottom[1];
      _bbox_bottom.z = bbox_bottom[2];
    }

    std::vector<double> world_min(3, std::numeric_limits<double>::min());
    std::vector<double> world_max(3, std::numeric_limits<double>::max());
    world_min = cfg.get<std::vector<double> >("WorldBoundBottom", world_min);
    world_max = cfg.get<std::vector<double> >("WorldBoundTop",    world_max);
    assert(world_min.size() == 3);
    assert(world_max.size() == 3);

    _world_min.x = world_min[0]; _world_min.y = world_min[1]; _world_min.z = world_min[2];
    _world_max.x = world_max[0]; _world_max.y = world_max[1]; _world_max.z = world_max[2];

  }


  ImageMeta3D BBoxInteraction::Generate(const EventInput& data) const
  {

    ImageMeta3D meta;
    size_t xnum = _xlen/_xvox;
    size_t ynum = _ylen/_yvox;
    size_t znum = _zlen/_zvox;

  // If using fixed bounding box, set as specified
    if(_bbox_bottom.x != kINVALID_DOUBLE) {
      meta.set(_bbox_bottom.x, _bbox_bottom.y, _bbox_bottom.z,
        _bbox_bottom.x+_xlen, _bbox_bottom.y+_ylen, _bbox_bottom.z+_zlen,
        xnum, ynum, znum
        );
    }else{
    // otherwise:
    // 1. loop over all energy deposition locations and define the "active region"
    // 2. find the overlap with the world boundary defined by _world_min and _world_max.
    // 3. for each coordinate (x,y,z), if the overlap region from 2 is smaller than
    //    the specified box size, center the box location to be the center of the overlap
    //    region. If the overlap region is larger than the specified box size, do a random
    //    draw to decide the box center location.

    // Step 1: define the active region
      Point3D active_min_pt, active_max_pt;
      bool active_region_valid = false;
      active_min_pt.x = active_min_pt.y = active_min_pt.z = std::numeric_limits<double>::max();
      active_max_pt.x = active_max_pt.y = active_max_pt.z = std::numeric_limits<double>::min();
      for(auto const& input_unit : data) {
        auto const& p = input_unit.part;
        active_region_valid = true;
        active_min_pt.x = std::min(p.first_step.pos.x, active_min_pt.x);
        active_min_pt.y = std::min(p.first_step.pos.y, active_min_pt.y);
        active_min_pt.z = std::min(p.first_step.pos.z, active_min_pt.z);
        active_max_pt.x = std::max(p.last_step.pos.x,  active_max_pt.x);
        active_max_pt.y = std::max(p.last_step.pos.y,  active_max_pt.y);
        active_max_pt.z = std::max(p.last_step.pos.z,  active_max_pt.z);
      }

    // Step 1.1: at this point, make sure either the world bound or active region is valid.
      if(!active_region_valid && 
        _world_min.x == std::numeric_limits<double>::min() &&
        _world_max.x == std::numeric_limits<double>::max())
      {
        throw meatloaf("World boundary is not set and there is no energy deposition to define a bounding box.");
      }

    // Step 2: define the overlap
      Point3D min_pt, max_pt;
      if(active_region_valid) {
        min_pt.x = std::max(_world_min.x, active_min_pt.x);
        min_pt.y = std::max(_world_min.y, active_min_pt.y);
        min_pt.z = std::max(_world_min.z, active_min_pt.z);
        max_pt.x = std::min(_world_max.x, active_max_pt.x);
        max_pt.y = std::min(_world_max.y, active_max_pt.y);
        max_pt.z = std::min(_world_max.z, active_max_pt.z);
      }else{
        min_pt = _world_min;
        max_pt = _world_max;
      }

      assert(min_pt.x <= max_pt.x && min_pt.y <= max_pt.y && min_pt.z <= max_pt.z);

      Point3D box_center;
      box_center.x = min_pt.x + (max_pt.x - min_pt.x)/2.;
      box_center.y = min_pt.y + (max_pt.y - min_pt.y)/2.;
      box_center.z = min_pt.z + (max_pt.z - min_pt.z)/2.;

      std::mt19937 mt;
      mt.seed(_seed);

      if( (max_pt.x - min_pt.x) > _xlen ) {
        double offset = (max_pt.x - min_pt.x)/2.;
        std::uniform_real_distribution<> dis(-offset, offset);
        box_center.x += dis(mt);
      }

      if( (max_pt.y - min_pt.y) > _ylen ) {
        double offset = (max_pt.y - min_pt.y)/2.;
        std::uniform_real_distribution<> dis(-offset, offset);
        box_center.y += dis(mt);
      }

      if( (max_pt.z - min_pt.z) > _zlen ) {
        double offset = (max_pt.z - min_pt.z)/2.;
        std::uniform_real_distribution<> dis(-offset, offset);
        box_center.z += dis(mt);
      }

      meta.set(box_center.x - _xlen/2., box_center.y - _ylen/2., box_center.z - _zlen/2.,
        box_center.x + _xlen/2., box_center.y + _ylen/2., box_center.z + _zlen/2.,
        xnum, ynum, znum);
    }
    
    return meta;
  }

}


#endif