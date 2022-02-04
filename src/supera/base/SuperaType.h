#ifndef __SUPERATYPE_H__
#define __SUPERATYPE_H__

#include <limits>
#include <cstddef>

namespace supera {

  const double kINVALID_DOUBLE = std::numeric_limits< double >::max();
  const size_t kINVALID_SIZE   = std::numeric_limits< size_t >::max();

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
}

#endif