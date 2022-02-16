/**
 * \file BBoxInteraction.h
 *
 * \ingroup Package_Name
 *
 * \brief Class def header for a class BBoxInteraction
 *
 * @author kazuhiro
 */

/** \addtogroup Package_Name
    @{*/
#ifndef __BBoxInteraction_H__
#define __BBoxInteraction_H__
//#ifndef __CINT__
//#ifndef __CLING__
#include "SuperaBase.h"
#include "supera/base/BBox.h"

namespace supera {

  /**
     \class ProcessBase
     User defined class BBoxInteraction ... these comments are used to generate
     doxygen documentation!
  */
  class BBoxInteraction {

  public:

    /// Default constructor
    BBoxInteraction(const std::string name = "BBoxInteraction");

    /// Default destructor
    ~BBoxInteraction() {}

  private:

    unsigned short _origin;
    double _xlen, _ylen, _zlen;
    double _xvox, _yvox, _zvox;
    supera::BBox3D _world_bounds;
    supera::BBox3D _bbox;
    std::vector<std::string> _cluster3d_labels;
    std::vector<std::string> _tensor3d_labels;

    bool _use_fixed_bbox;
    std::vector<double> _bbox_bottom;

    bool update_bbox(supera::BBox3D& bbox, const supera::Point3D& pt);
    void randomize_bbox_center(supera::BBox3D& bbox);
  };


}
//#endif
//#endif
#endif
/** @} */ // end of doxygen group
