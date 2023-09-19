#ifndef __BBOXINTERACTION_CXX__
#define __BBOXINTERACTION_CXX__

#include <sys/time.h>
#include <assert.h>
#include "BBoxInteraction.h"
#include "supera/data/Event.h"

namespace supera {


  void BBoxInteraction::_configure(const YAML::Node& cfg)
  {

    int seed = -1;
    if(cfg["Seed"]) seed = cfg["Seed"].as<int>();
    if(seed < 0) {
      // use time seed
      struct timeval time; 
      gettimeofday(&time,NULL);
      _seed = (size_t)(time.tv_sec * 100 + time.tv_usec / 100);
    }else
    _seed = (size_t)(seed);

    auto bbox_size = cfg["BBoxSize"].as<std::vector<double> >();
    assert(bbox_size.size() == 3);
    _xlen = bbox_size.at(0);
    _ylen = bbox_size.at(1);
    _zlen = bbox_size.at(2);

    auto voxel_size = cfg["VoxelSize"].as<std::vector<double> >();
    assert(voxel_size.size() == 3);
    _xvox = voxel_size.at(0);
    _yvox = voxel_size.at(1);
    _zvox = voxel_size.at(2);

    std::vector<double> bbox_bottom;
    if(cfg["BBoxBottom"])
      bbox_bottom = cfg["BBoxBottom"].as<std::vector<double>>();
    assert(bbox_bottom.size()<1 || bbox_bottom.size() == 3);
    if(bbox_bottom.size()==3) {
      _bbox_bottom.x = bbox_bottom[0];
      _bbox_bottom.y = bbox_bottom[1];
      _bbox_bottom.z = bbox_bottom[2];
      bbox_bottom_set=true;
    }

    std::vector<double> world_min(3, -std::numeric_limits<double>::max());
    std::vector<double> world_max(3, std::numeric_limits<double>::max());
    if(cfg["WorldBoundBottom"])
      world_min = cfg["WorldBoundMin"].as<std::vector<double> >();
    if(cfg["WorldBoundTop"])
      world_max = cfg["WorldBoundMax"].as<std::vector<double> >();

    assert(world_min.size() == 3);
    assert(world_max.size() == 3);

    _world_min.x = world_min[0]; _world_min.y = world_min[1]; _world_min.z = world_min[2];
    _world_max.x = world_max[0]; _world_max.y = world_max[1]; _world_max.z = world_max[2];

  }


  ImageMeta3D BBoxInteraction::Generate(const EventInput& data) const
  {

    LOG_DEBUG() << "starting" << std::endl;
    ImageMeta3D meta;

    if (_xvox==kINVALID_DOUBLE||_yvox==kINVALID_DOUBLE||_zvox==kINVALID_DOUBLE)
    {
      LOG_ERROR() << "Voxel length for BBox (_xvox) not set in config" << "\n";
      throw meatloaf("Voxel length for BBox (_xvox) not set in config");
    }
    if (_xlen==kINVALID_DOUBLE||_ylen==kINVALID_DOUBLE||_zlen==kINVALID_DOUBLE)
    {
      LOG_ERROR() << "length for BBox (_xlen) not set in config" << "\n";
      throw meatloaf("length for BBox (_xlen) not set in config");
    }

    size_t xnum = _xlen/_xvox;
    size_t ynum = _ylen/_yvox;
    size_t znum = _zlen/_zvox;

  // If using fixed bounding box, set as specified
    if(_bbox_bottom.x != kINVALID_DOUBLE) {
      LOG_DEBUG() << _xvox << " " << _yvox << " " << _zvox << "   setting size of voxels" << std::endl
      << "set meta with cfg bbox" << std::endl;
      meta.set(_bbox_bottom.x, _bbox_bottom.y, _bbox_bottom.z,
        _bbox_bottom.x+_xlen, _bbox_bottom.y+_ylen, _bbox_bottom.z+_zlen,
        xnum, ynum, znum
        );
      return meta;
    }
    //else if ((data[0].edep_bottom_left.x!=std::numeric_limits<double>::max()&&data[0].edep_top_right.x!=-std::numeric_limits<double>::max())||(_world_min.x != -std::numeric_limits<double>::max()&&_world_max.x != std::numeric_limits<double>::max()))
    else if (bbox_bottom_set == false)
    {
    // otherwise:
    // 1. loop over all energy deposition locations and define the "active region"
    // 2. find the overlap with the world boundary defined by _world_min and _world_max.
    // 3. for each coordinate (x,y,z), if the overlap region from 2 is smaller than
    //    the specified box size, center the box location to be the center of the overlap
    //    region. If the overlap region is larger than the specified box size, do a random
    //    draw to decide the box center location.

    // Step 1: define the active region
      LOG_DEBUG() << "generating bbox from edep information" << std::endl;
      Point3D active_min_pt, active_max_pt;
      active_min_pt.x = active_min_pt.y = active_min_pt.z = std::numeric_limits< double >::max();
      active_max_pt.x = active_max_pt.y = active_max_pt.z = std::numeric_limits< double >::min();

      for(auto const& label : data) {
        for(auto const& pt : label.pcloud ) {
          active_min_pt.x = std::min(active_min_pt.x, pt.x);
          active_min_pt.y = std::min(active_min_pt.y, pt.y);
          active_min_pt.z = std::min(active_min_pt.z, pt.z);
          active_max_pt.x = std::max(active_max_pt.x, pt.x);
          active_max_pt.y = std::max(active_max_pt.y, pt.y);
          active_max_pt.z = std::max(active_max_pt.z, pt.z);
        }
      }

      // Step 2: define the overlap
      Point3D min_pt, max_pt;
      min_pt.x = std::max(_world_min.x, active_min_pt.x);
      min_pt.y = std::max(_world_min.y, active_min_pt.y);
      min_pt.z = std::max(_world_min.z, active_min_pt.z);
      max_pt.x = std::min(_world_max.x, active_max_pt.x);
      max_pt.y = std::min(_world_max.y, active_max_pt.y);
      max_pt.z = std::min(_world_max.z, active_max_pt.z);
      assert(min_pt.x <= max_pt.x && min_pt.y <= max_pt.y && min_pt.z <= max_pt.z);

      Point3D box_center;
      box_center.x = min_pt.x + (max_pt.x - min_pt.x)/2.;
      box_center.y = min_pt.y + (max_pt.y - min_pt.y)/2.;
      box_center.z = min_pt.z + (max_pt.z - min_pt.z)/2.;

      std::mt19937 mt;
      mt.seed(_seed);

      size_t xnum = _xlen / _xvox;
      size_t ynum = _ylen / _yvox;
      size_t znum = _zlen / _zvox;

      if( (max_pt.x-min_pt.x) > _xlen ) {
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

      LOG_DEBUG() << "           " << _xlen << " " << _ylen << " " << _zlen << " lengths " << std::endl
      << "meta set" << box_center.x - _xlen / 2. << " " << box_center.y - _ylen / 2. << " " << box_center.z - _zlen / 2. << " "
      << box_center.x + _xlen / 2. << " " << box_center.y + _ylen / 2. << " " << box_center.z + _zlen / 2.<<" "
      << xnum << " " << ynum << " " << znum<<std::endl;
      meta.set(box_center.x - _xlen/2., box_center.y - _ylen/2., box_center.z - _zlen/2.,
        box_center.x + _xlen/2., box_center.y + _ylen/2., box_center.z + _zlen/2.,
        xnum, ynum, znum);
      return meta;
    }
    throw meatloaf("World boundary is not set and there is no energy deposition to define a bounding box.");
  }

}


#endif
