
#include "base_pybind.h"
//#include "SuperaEvent.h"
#include "SuperaType.h"

#include "supera/pybind_mkdoc.h"

void init_base(pybind11::module& m)
{
/*
  pybind11::class_<supera::ParticleLabel>(m, "ParticleGroup")
  .def(pybind11::init<size_t&>())
  .def(pybind11::init<size_t&>(), pybind11::arg("num_planes")=0)
  ;

  pybind11::class_<supera::ImageLabel>(m, "ImageLabel")
  ;
*/
  pybind11::enum_<supera::SemanticType_t>(m, "SemanticType", DOC(supera, SemanticType_t))
      .value("Shower", supera::kShapeShower,  DOC(supera, SemanticType_t, kShapeShower))
      .value("Track", supera::kShapeTrack, DOC(supera, SemanticType_t, kShapeTrack))
      .value("Michel", supera::kShapeMichel, DOC(supera, SemanticType_t, kShapeMichel))
      .value("Delta", supera::kShapeDelta, DOC(supera, SemanticType_t, kShapeDelta))
      .value("LEScatter", supera::kShapeLEScatter, DOC(supera, SemanticType_t, kShapeLEScatter))
      .value("Ghost", supera::kShapeGhost, DOC(supera, SemanticType_t, kShapeGhost))
      .value("Unknown", supera::kShapeUnknown, DOC(supera, SemanticType_t, kShapeUnknown))
      .export_values();

  pybind11::enum_<supera::ProcessType>(m, "ProcessType", DOC(supera, ProcessType))
      .value("kTrack", supera::kTrack, DOC(supera, ProcessType, kTrack))
      .value("kNeutron", supera::kNeutron, DOC(supera, ProcessType, kNeutron))
      .value("kPhoton", supera::kPhoton, DOC(supera, ProcessType, kPhoton))
      .value("kPrimary", supera::kPrimary, DOC(supera, ProcessType, kPrimary))
      .value("kCompton", supera::kCompton, DOC(supera, ProcessType, kCompton))
      .value("kComptonHE", supera::kComptonHE, DOC(supera, ProcessType, kComptonHE))
      .value("kDelta", supera::kDelta, DOC(supera, ProcessType, kDelta))
      .value("kConversion", supera::kConversion, DOC(supera, ProcessType, kConversion))
      .value("kIonization", supera::kIonization, DOC(supera, ProcessType, kIonization))
      .value("kPhotoElectron", supera::kPhotoElectron, DOC(supera, ProcessType, kPhotoElectron))
      .value("kDecay", supera::kDecay, DOC(supera, ProcessType, kDecay))
      .value("kOtherShower", supera::kOtherShower, DOC(supera, ProcessType, kOtherShower))
      .value("kOtherShowerHE", supera::kOtherShowerHE, DOC(supera, ProcessType, kOtherShowerHE))
      .value("kInvalidProcess", supera::kInvalidProcess, DOC(supera, ProcessType, kInvalidProcess))
      .export_values();


  m.attr("kINVALID_SIZE")   = supera::kINVALID_SIZE;
  m.attr("kINVALID_DOUBLE") = supera::kINVALID_DOUBLE;
  m.attr("kINVALID_FLOAT") = supera::kINVALID_FLOAT;
  m.attr("kINVALID_UINT") = supera::kINVALID_UINT;
  m.attr("kINVALID_ULONG") = supera::kINVALID_ULONG;
  m.attr("kINVALID_LONG") = supera::kINVALID_LONG;

  m.attr("kINVALID_PDG") = supera::kINVALID_PDG;
  m.attr("kINVALID_INDEX") = supera::kINVALID_INDEX;
  m.attr("kINVALID_TRACKID") = supera::kINVALID_TRACKID;
  m.attr("kINVALID_VOXELID") = supera::kINVALID_VOXELID;
  m.attr("kINVALID_INSTANCEID") = supera::kINVALID_INSTANCEID;

}
