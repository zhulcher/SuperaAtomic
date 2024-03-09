#include "Neutrino.h"

#include <algorithm>
#include <iostream>
#include <map>
#include <sstream>
#include "supera/base/meatloaf.h"

namespace supera {


  std::string Neutrino::dump() const
  {
    std::stringstream ss;
    std::stringstream buf;
    ss  << "      \033[95m" << "Neutrino " << " (PdgCode,TrackID) = (" << pdg_code << "," << StringifyTrackID(track_id) << ")\033[00m " << std::endl;
    buf << "      ";

    ss << buf.str() << "Vertex   (x, y, z, t) = (" << vtx.pos.x << "," << vtx.pos.y << "," << vtx.pos.z << "," << vtx.time << ")" << std::endl
       << buf.str() << "Momentum (px, py, pz) = (" << px << "," << py << "," << pz << ")" << std::endl
       << buf.str() << "Initial Energy  = " << energy_init << std::endl
       << buf.str() << "Deposit  Energy  = " << energy_deposit << std::endl
       << buf.str() << "Creation Process = " << creation_process << std::endl;

    return ss.str();
  }


} // namespace supera

