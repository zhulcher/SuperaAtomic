
#include "base_pybind.h"
#include "SuperaType.h"
#include "Point.h"

#include "pybind11/operators.h"
#include "supera/pybind_mkdoc.h"

void init_base(pybind11::module& m)
{
  using namespace pybind11::literals;

  // types & consts from SuperaType.h
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

  // ----------------------------------------------------------------

  // types & consts from Point.h
  pybind11::class_<supera::Point3D>(m, "Point3D", DOC(supera, Point3D))
      // constructors
      .def(pybind11::init<>(), DOC(supera, Point3D, Point3D))
      .def(pybind11::init<double, double, double>(), DOC(supera, Point3D, Point3D, 2),
                          "xv"_a, "yv"_a, "zv"_a)
      .def(pybind11::init<const supera::Point3D&>(), DOC(supera, Point3D, Point3D, 3),
                          "pt"_a)

      // overloaded operators
      .def(pybind11::self == pybind11::self)
      .def(pybind11::self != pybind11::self)
      .def(pybind11::self /= double())
      .def(pybind11::self *= double())
      .def(pybind11::self += pybind11::self)
      .def(pybind11::self -= pybind11::self)
      .def(pybind11::self - pybind11::self)
      .def(pybind11::self + pybind11::self)
      .def(pybind11::self * double())
      .def(pybind11::self / double())

      // other methods
      .def("squared_distance", &supera::Point3D::squared_distance, DOC(supera, Point3D, squared_distance), "pt"_a)
      .def("distance", &supera::Point3D::distance, DOC(supera, Point3D, distance), "pt"_a)
      .def("direction", &supera::Point3D::direction, DOC(supera, Point3D, direction), "pt"_a)

      // member data
      .def_readwrite("x", &supera::Point3D::x, DOC(supera, Point3D, x))
      .def_readwrite("y", &supera::Point3D::y, DOC(supera, Point3D, y))
      .def_readwrite("z", &supera::Point3D::z, DOC(supera, Point3D, z));

  pybind11::class_<supera::EDep, supera::Point3D>(m, "EDep", DOC(supera, EDep))
      .def(pybind11::init<>(), DOC(supera, EDep, EDep))
      .def_readwrite("t", &supera::EDep::t, DOC(supera, EDep, t))
      .def_readwrite("e", &supera::EDep::e, DOC(supera, EDep, e))
      .def_readwrite("dedx", &supera::EDep::t, DOC(supera, EDep, dedx));

  pybind11::class_<supera::Vertex>(m, "Vertex", DOC(supera, Vertex))
      // constructors
      .def(pybind11::init<>(), DOC(supera, Vertex, Vertex))
      .def(pybind11::init<double, double, double, double>(), DOC(supera, Vertex, Vertex, 2))

      // overloaded operators
      .def(pybind11::self == pybind11::self)
      .def(pybind11::self != pybind11::self)
      .def(pybind11::self < pybind11::self)

      // other methods
      .def("approx", &supera::Vertex::approx, DOC(supera, Vertex, approx), "power"_a=6)
      .def("dump", &supera::Vertex::dump, DOC(supera, Vertex, dump))

      // data members
      .def_readwrite("pos", &supera::Vertex::pos, DOC(supera, Vertex, pos))
      .def_readwrite("time", &supera::Vertex::time, DOC(supera, Vertex, time));


}
