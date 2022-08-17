#ifndef __SUPERATYPE_H__
#define __SUPERATYPE_H__

#include <limits>
#include <cstddef>
#include <string>

namespace supera {


  typedef long PdgCode_t;
  typedef unsigned long Index_t;
  typedef unsigned long TrackID_t;
  typedef unsigned long VoxelID_t;
  typedef unsigned long InstanceID_t;

  const double        kINVALID_DOUBLE  = std::numeric_limits< double >::max();
  const float         kINVALID_FLOAT   = std::numeric_limits< float  >::max();
  const unsigned int  kINVALID_UINT    = std::numeric_limits<unsigned int>::max();
  const unsigned long kINVALID_ULONG   = std::numeric_limits<unsigned long>::max();
  const long          kINVALID_LONG    = std::numeric_limits<long>::max();
  const size_t        kINVALID_SIZE    = std::numeric_limits< size_t >::max();

  const PdgCode_t kINVALID_PDG     = kINVALID_LONG;
  const Index_t   kINVALID_INDEX   = kINVALID_ULONG;
  const TrackID_t kINVALID_TRACKID = kINVALID_ULONG;
  const VoxelID_t    kINVALID_VOXELID    = kINVALID_ULONG;
  const InstanceID_t kINVALID_INSTANCEID = kINVALID_ULONG;

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

  inline std::string StringifyIndex(supera::Index_t idx) { return (idx == kINVALID_INDEX ? "kINVALID_INDEX" : std::to_string(idx)); }
  inline std::string StringifyTrackID(supera::TrackID_t id) { return (id == kINVALID_TRACKID ? "kINVALID_TRACKID" : std::to_string(id)); }
  inline std::string StringifyVoxelID(supera::VoxelID_t id) { return (id == kINVALID_VOXELID ? "kINVALID_VOXELID" : std::to_string(id)); }
  inline std::string StringifyInstanceID(supera::InstanceID_t id) { return (id == kINVALID_INSTANCEID ? "kINVALID_INSTANCEID" : std::to_string(id)); }
}

#endif
