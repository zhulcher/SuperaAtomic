/**
 * \file Neutrino.h
 *
 * \ingroup base
 *
 * \brief Class for information about GENIE neutrino
 *
 * @author Sindhu, copied from larcv
 */

/** \addtogroup base

    @{*/
#ifndef __SUPERA_NEUTRINO_H__
#define __SUPERA_NEUTRINO_H__

#include <array>
#include <iostream>
#include <vector>

#include "supera/base/Point.h"
#include "supera/base/SuperaType.h"
#include "supera/base/Voxel.h"
namespace supera {

  /**
     \class Neutrino
     \brief Neutrino/Interaction-wise truth information data from GENIE
  */
  class Neutrino{

  public:

    /// Default constructor
    Neutrino()
      : id         (kINVALID_INSTANCEID)
      , interaction_id      (kINVALID_INSTANCEID)
			, nu_track_id         (kINVALID_TRACKID)
			, lepton_track_id  (kINVALID_TRACKID)
			, current_type     (-1)
			, interaction_mode (-1)
			, interaction_type (-1)
			, target           (-1)
			, nucleon          (-1)
			, quark            (-1)
			, hadronic_invariant_mass                (0.)
			, bjorken_x        (0.)
			, inelasticity     (0.)
			, momentum_transfer(0.)
			, momentum_transfer_mag (0.)
			, energy_transfer(0.)
			, theta            (0.)
      , pdg_code              (kINVALID_PDG)
      , lepton_pdg_code       (kINVALID_PDG)
      , px               (0.)
      , py               (0.)
      , pz               (0.)
      , lepton_p         (0.)
      , dist_travel      (-1)
      , energy_init      (0.)
      , energy_deposit   (0.)
      , creation_process          ("")
      , num_voxels       (0)
    {}

    /// Default destructor
    ~Neutrino(){}
    inline double p() const { return sqrt(pow(px,2)+pow(py,2)+pow(pz,2)); }
    std::string dump() const;
      
  public:

    InstanceID_t id; ///< "ID" of this neutrino interaction, unique in file
    InstanceID_t interaction_id; ///< Original generator ID, if different from Geant4 one (e.g.: GENIE particle ID)

    TrackID_t nu_track_id;     ///< Geant4 track id
		TrackID_t lepton_track_id;
    short current_type;       ///< if neutrino, shows interaction GENIE current type. else kINVALID_USHORT
		short interaction_mode;   ///< if neutrino, shows interaction GENIE mode (QE / 1-pi / DIS / ...)
    short interaction_type;   ///< if neutrino, shows interaction GENIE code. else kINVALID_USHORT
		int target;
		int nucleon;           ///< if neutrino, shows nucleon hit 2212 (proton) or 2112 (neutron)
		int quark;              ///< if neutrino and DIS event, shows quark PDG code
		double hadronic_invariant_mass;
		double bjorken_x;         ///< if neutrino, Bjorken variable x
		double inelasticity;
		double momentum_transfer; ///< if neutrino, momentum transfer Q^2 [MeV^2]
		double momentum_transfer_mag; ///< if neutrino,maginitude of momentum transfer [MeV]
		double energy_transfer; ///< if neutrino, energy transfer [MeV]
		double theta; // angle between neutrino and outgoing lepton

    PdgCode_t    pdg_code;         ///< PDG code   
    PdgCode_t    lepton_pdg_code;         ///< PDG code of outgoing lepton
    double       px,py,pz;  ///< (x,y,z) component of particle's initial momentum    
    double       lepton_p; // outgoing lepton's momentum
    Vertex       vtx;         ///< (x,y,z,t) of particle's vertex information
    double       dist_travel; ///< filled only if MCTrack origin: distance measured along the trajectory
    double       energy_init; ///< initial energy of the particle
    double       energy_deposit; ///< deposited energy of the particle in the detector
    std::string  creation_process;     ///< string identifier of the particle's creation process from Geant4
    int num_voxels; ///< Number of voxels in the particle's 3D cluster.

    std::vector<supera::InstanceID_t> children_id; ///< "ID" of the children particles in ParticleSet collection
		std::vector<double> traj_x;
		std::vector<double> traj_y;
		std::vector<double> traj_z;
		std::vector<double> traj_t;
		std::vector<double> traj_px;
		std::vector<double> traj_py;
		std::vector<double> traj_pz;
		std::vector<double> traj_e;
  };

}
#endif
/** @} */ // end of doxygen group
