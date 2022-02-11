#ifndef __SUPERATYPE_H__
#define __SUPERATYPE_H__

#include <limits>
#include <cstddef>

namespace supera {

  typedef size_t VoxelID_t;
  typedef size_t InstanceID_t;

  const double kINVALID_DOUBLE  = std::numeric_limits< double >::max();
  const float  kINVALID_FLOAT   = std::numeric_limits< float  >::max();
  const size_t kINVALID_SIZE    = std::numeric_limits< size_t >::max();
  const size_t kINVALID_VOXELID = std::numeric_limits<size_t>::max();

  enum ProcessType {
    kTrack,
    kNeutron,
    kPhoton,
    kPrimary,
    kCompton,    // detach low E shower
    kComptonHE,  // detach high E shower
    kDelta,      // attach-mu low E special
    kConversion, // detach high E gamma
    kIonization, // attach-e low E 
    kPhotoElectron, // detatch low E
    kDecay,      // attach high E
    kOtherShower,   // anything else (low E)
    kOtherShowerHE, // anything else (high E)
    kInvalidProcess
  };

  /// Object appearance type in LArTPC, used for semantic type classification
  enum SemanticType_t {
    kShapeShower,    ///< Shower
    kShapeTrack,     ///< Track
    kShapeMichel,    ///< Michel
    kShapeDelta,     ///< Delta ray
    kShapeLEScatter, ///< low energy scattering (e.g. low-E compton)
    kShapeGhost,     ///< ghost 3d point
    kShapeUnknown    ///< LArbys
  };
}

#endif