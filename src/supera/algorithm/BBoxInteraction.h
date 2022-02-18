/**
 * \file BBoxInteraction.h
 *
 * \ingroup algorithm
 *
 * \brief Class def header for a class BBoxInteraction
 *
 * @author kazuhiro
 */

/** \addtogroup algorithm
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
        \class BBoxInteraction
        An implementation of BBoxAlgorithm for three basic options.
        1) A user may specify the exact image bounding box coordinate with number of pixels along each axis.\n
        2) Or a user may let algorithm figure out the boundaries based image "active regions", defined as a rectangular \n
           box that contains all energy depositions provided in EventInput instance. The sub-region is selected if the  \n
           active region is larger than the specified image size by a user. This selection is done by randomly sampling \n
           the specified image side length within the active region. If the active region is smaller than the specified \n
           image size, then the image central position is randomly set under the constraint of containing the full active \n
           region.
        3) A user may also specify the "world boundary" in the process of 2). If specified, the image boundary is set so \n
           that the recorded image stays within the world boundary. This may be useful for users who want to randomly sample \n
           a particular part of an imaging detector. For instance, you can use this to avoid sampling outside the detector \n
           volume where energy deposition may still happen and therefore the algorithm may recognize an "active region".
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
