#ifndef __SUPERATYPE_H__
#define __SUPERATYPE_H__

#include <limits>
#include <cstddef>
#include <string>
#include <algorithm>
#include "meatloaf.h"
namespace supera {

  typedef int PdgCode_t;
  typedef unsigned long Index_t;
  typedef unsigned long TrackID_t;
  typedef unsigned long VoxelID_t;
  typedef unsigned long InstanceID_t;

  const double        kINVALID_DOUBLE  = std::numeric_limits< double >::max();
  const float         kINVALID_FLOAT   = std::numeric_limits< float  >::max();
  const int           kINVALID_INT     = std::numeric_limits<int>::max();
  const unsigned int  kINVALID_UINT    = std::numeric_limits<unsigned int>::max();
  const unsigned long kINVALID_ULONG   = std::numeric_limits<unsigned long>::max();
  const long          kINVALID_LONG    = std::numeric_limits<long>::max();
  const size_t        kINVALID_SIZE    = std::numeric_limits< size_t >::max();

  const PdgCode_t kINVALID_PDG = kINVALID_INT;
  typedef unsigned int CUInt_t;
  const Index_t   kINVALID_INDEX   = kINVALID_ULONG;
  const TrackID_t kINVALID_TRACKID = kINVALID_ULONG;
  const VoxelID_t    kINVALID_VOXELID    = kINVALID_ULONG;
  const InstanceID_t kINVALID_INSTANCEID = kINVALID_ULONG;

  enum ProcessType_t
  {
    kTrack,
    kNeutron,
    kNucleus,
    kPhoton,
    kPrimary,
    kCompton,       // compton shower
    kDelta,         // knocked-off electron
    kConversion,    // gamma pair-production
    kIonization,    // ionization electron, same as DeltaRay but too low energy to be on its own
    kPhotoElectron, // photoelectron
    kDecay,         // decay particle
    kOtherShower,   // any other shower
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

  /// Namespace for supera message related types
  namespace msg {

    /// Message level
    enum Level_t { kVERBOSE, kDEBUG, kINFO, kWARNING, kERROR, kFATAL, kMSG_TYPE_MAX };

    /// Formatted message prefix per message level
    const std::string kStringPrefix[kMSG_TYPE_MAX] =
      {
        "   \033[94m[VERBOSE]\033[00m ",  ///< kVERBOSE message prefix
        "     \033[92m[DEBUG]\033[00m ",  ///< kDEBUG message prefix
        "      \033[95m[INFO]\033[00m ",  ///< kINFO message prefix
        "   \033[93m[WARNING]\033[00m ", ///< kWARNING message prefix
        "     \033[91m[ERROR]\033[00m ", ///< kERROR message prefix
        "     \033[5;1;33;41m[FATAL]\033[00m "  ///< kFATAL message prefix
      };
    ///< Prefix of message
    inline Level_t parseStringThresh(std::string threshStr)
    {
        std::for_each(threshStr.begin(), threshStr.end(), [](char &ch){ch = ::toupper(ch); });

        if (threshStr == "VERBOSE")
            return msg::Level_t::kVERBOSE;
        else if (threshStr == "DEBUG")
            return msg::Level_t::kDEBUG;
        else if (threshStr == "INFO")
            return msg::Level_t::kINFO;
        else if (threshStr == "WARNING")
            return msg::Level_t::kWARNING;
        else if (threshStr == "ERROR")
            return msg::Level_t::kERROR;
        else if (threshStr == "FATAL")
            return msg::Level_t::kFATAL;

        throw meatloaf("Unrecognized log threshold: " + threshStr);
    }
  }
}

#endif
