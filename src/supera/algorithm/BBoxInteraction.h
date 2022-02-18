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

#include <random>
#include "BBoxBase.h"
#include "supera/base/Point.h"

namespace supera {

  /**
     \class ProcessBase
     User defined class BBoxInteraction ... these comments are used to generate
     doxygen documentation!
  */
  class BBoxInteraction : public BBoxAlgorithm {

  public:

    /// Default constructor
    BBoxInteraction() {}

    void Configure(const PSet& p);

    ImageMeta3D Generate(const EventInput& data) const;

    /// Default destructor
    ~BBoxInteraction() {}

  private:
    unsigned short _origin;
    double _xlen, _ylen, _zlen;
    double _xvox, _yvox, _zvox;
    supera::Point3D _world_min, _world_max, _bbox_bottom;
    size_t _seed;
  };


}
//#endif
//#endif
#endif
/** @} */ // end of doxygen group
