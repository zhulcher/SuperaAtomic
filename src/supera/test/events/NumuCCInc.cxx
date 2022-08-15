#include "supera/test/TestEvents.h"

namespace supera
{
  namespace test
  {
    TestEvent NumuCCIncEvt()
    {
      supera::test::TestEvent ret;

      supera::EventInput input;

      supera::ParticleInput particleInput;
      particleInput.type = kPrimary;

      supera::EDep particleInput0_edep0;
      particleInput0_edep0.x = 341.8;
      particleInput0_edep0.y = -71.8;
      particleInput0_edep0.z = 641;
      particleInput0_edep0.t = 1;
      particleInput0_edep0.e = 0.351079;
      particleInput0_edep0.dedx = 1.79769e+308;
      particleInput.pcloud.emplace_back(std::move(particleInput0_edep0));
      supera::EDep particleInput0_edep1;
      particleInput0_edep1.x = 341.8;
      particleInput0_edep1.y = -71.8;
      particleInput0_edep1.z = 641.4;
      particleInput0_edep1.t = 1.01013;
      particleInput0_edep1.e = 0.83434;
      particleInput0_edep1.dedx = 1.79769e+308;
      particleInput.pcloud.emplace_back(std::move(particleInput0_edep1));
      supera::EDep particleInput0_edep2;
      particleInput0_edep2.x = 341.8;
      particleInput0_edep2.y = -71.8;
      particleInput0_edep2.z = 641.8;
      particleInput0_edep2.t = 1.02026;
      particleInput0_edep2.e = 0.83434;
      particleInput0_edep2.dedx = 1.79769e+308;
      particleInput.pcloud.emplace_back(std::move(particleInput0_edep2));
      supera::EDep particleInput0_edep3;
      particleInput0_edep3.x = 341.8;
      particleInput0_edep3.y = -71.8;
      particleInput0_edep3.z = 642.2;
      particleInput0_edep3.t = 1.03038;
      particleInput0_edep3.e = 0.830779;
      particleInput0_edep3.dedx = 1.79769e+308;
      particleInput.pcloud.emplace_back(std::move(particleInput0_edep3));
      supera::EDep particleInput0_edep4;
      particleInput0_edep4.x = 341.8;
      particleInput0_edep4.y = -72.2;
      particleInput0_edep4.z = 642.2;
      particleInput0_edep4.t = 1.04051;
      particleInput0_edep4.e = 0.0035606;
      particleInput0_edep4.dedx = 1.79769e+308;
      particleInput.pcloud.emplace_back(std::move(particleInput0_edep4));
      supera::EDep particleInput0_edep5;
      particleInput0_edep5.x = 341.8;
      particleInput0_edep5.y = -72.2;
      particleInput0_edep5.z = 642.6;
      particleInput0_edep5.t = 1.05064;
      particleInput0_edep5.e = 0.83434;
      particleInput0_edep5.dedx = 1.79769e+308;
      particleInput.pcloud.emplace_back(std::move(particleInput0_edep5));
      supera::EDep particleInput0_edep6;
      particleInput0_edep6.x = 341.8;
      particleInput0_edep6.y = -72.2;
      particleInput0_edep6.z = 643;
      particleInput0_edep6.t = 1.06077;
      particleInput0_edep6.e = 0.274378;
      particleInput0_edep6.dedx = 1.79769e+308;
      particleInput.pcloud.emplace_back(std::move(particleInput0_edep6));
      supera::EDep particleInput0_edep7;
      particleInput0_edep7.x = 341.4;
      particleInput0_edep7.y = -72.2;
      particleInput0_edep7.z = 643;
      particleInput0_edep7.t = 1.0709;
      particleInput0_edep7.e = 0.559962;
      particleInput0_edep7.dedx = 1.79769e+308;
      particleInput.pcloud.emplace_back(std::move(particleInput0_edep7));
      supera::EDep particleInput0_edep8;
      particleInput0_edep8.x = 341.4;
      particleInput0_edep8.y = -72.2;
      particleInput0_edep8.z = 643.4;
      particleInput0_edep8.t = 1.08102;
      particleInput0_edep8.e = 0.83434;
      particleInput0_edep8.dedx = 1.79769e+308;
      particleInput.pcloud.emplace_back(std::move(particleInput0_edep8));
      supera::EDep particleInput0_edep9;
      particleInput0_edep9.x = 341.4;
      particleInput0_edep9.y = -72.2;
      particleInput0_edep9.z = 643.8;
      particleInput0_edep9.t = 1.09115;
      particleInput0_edep9.e = 0.83434;
      particleInput0_edep9.dedx = 1.79769e+308;
      particleInput.pcloud.emplace_back(std::move(particleInput0_edep9));
      supera::EDep particleInput0_edep10;
      particleInput0_edep10.x = 341.4;
      particleInput0_edep10.y = -72.2;
      particleInput0_edep10.z = 644.2;
      particleInput0_edep10.t = 1.10128;
      particleInput0_edep10.e = 0.6177;
      particleInput0_edep10.dedx = 1.79769e+308;
      particleInput.pcloud.emplace_back(std::move(particleInput0_edep10));

      particleInput.part.id = 0; 
      particleInput.part.shape = kShapeTrack;     
      particleInput.part.trackid = 0;     
      particleInput.part.pdg = 13;
      std::tie(particleInput.part.px, particleInput.part.py, particleInput.part.pz) = std::make_tuple(-972.213, -258.3, 4677.45);
      particleInput.part.vtx = {341.995, -71.9245, 641.032, 1};
      particleInput.part.end_pt = {85.08, -70.1889, 1778.16, 40.5223};
      particleInput.part.first_step = {341.995, -71.9245, 641.032, 1};
      particleInput.part.last_step = {281.361, -83.1292, 930, 17.379};
      particleInput.part.dist_travel = 1175.63; 
      particleInput.part.energy_init = 4785.57;
      particleInput.part.energy_deposit = 508.921;
      particleInput.part.process = "primary";     

      particleInput.part.parent_trackid = kINVALID_TRACKID; 
      particleInput.part.parent_pdg = kINVALID_PDG;     
      particleInput.part.parent_vtx = {0., 0., 0., -1.};     

      // this particle is at the top so it's its own ancestor
      particleInput.part.ancestor_trackid = particleInput.part.trackid; 
      particleInput.part.ancestor_pdg = particleInput.part.pdg;     
      particleInput.part.ancestor_vtx = particleInput.part.vtx;     
      particleInput.part.ancestor_process = particleInput.part.process; 

      particleInput.part.parent_process = ""; 
      particleInput.part.parent_id = kINVALID_INSTANCEID;      
      particleInput.part.children_id = {}; 
      particleInput.part.group_id = 0;       
      particleInput.part.interaction_id = 0; 

      input.emplace_back(std::move(particleInput));

      ret.input = std::move(input);

      supera::ImageMeta3D meta;
      meta.set(-370, -160, 400, 370, 160, 930, 1850, 800, 1325);
      ret.output_meta = std::move(meta);

      supera::EventOutput evtOutput;
      supera::ParticleLabel evtOutput_part0_label;
      supera::Particle evtOutput_part0_label_part;
      evtOutput_part0_label_part.id = 0;
      evtOutput_part0_label_part.shape = static_cast<supera::SemanticType_t>(1);
      evtOutput_part0_label_part.trackid = 0;
      evtOutput_part0_label_part.pdg = 13;
      evtOutput_part0_label_part.px = -972.213;
      evtOutput_part0_label_part.py = -258.3;
      evtOutput_part0_label_part.pz = 4677.45;
      evtOutput_part0_label_part.vtx = {341.995, -71.9245, 641.032, 1};
      evtOutput_part0_label_part.end_pt = {85.08, -70.1889, 1778.16, 40.5223};
      evtOutput_part0_label_part.first_step = {341.8, -71.8, 641, 1};
      evtOutput_part0_label_part.last_step = {285, -83, 913.4, 10.2888};
      evtOutput_part0_label_part.dist_travel = 1175.63;
      evtOutput_part0_label_part.energy_init = 4785.57;
      evtOutput_part0_label_part.energy_deposit = 594.98;
      evtOutput_part0_label_part.process = "primary";
      evtOutput_part0_label_part.parent_trackid = 4294967295;
      evtOutput_part0_label_part.parent_pdg = 0;
      evtOutput_part0_label_part.parent_vtx = {0, 0, 0, 0};
      evtOutput_part0_label_part.ancestor_trackid = 4294967295;
      evtOutput_part0_label_part.ancestor_pdg = 0;
      evtOutput_part0_label_part.ancestor_vtx = {0, 0, 0, 0};
      evtOutput_part0_label_part.ancestor_process = "";
      evtOutput_part0_label_part.parent_process = "";
      evtOutput_part0_label_part.parent_id = 0;
      evtOutput_part0_label_part.children_id = { 2, 3, 4, 5, 6, 7, 8, 13, 14, 18, 19, 20, 35, 48 };
      evtOutput_part0_label_part.group_id = 0;
      evtOutput_part0_label_part.interaction_id = 0;
      evtOutput_part0_label.part = std::move(evtOutput_part0_label_part);
      evtOutput_part0_label.valid = 0;
      evtOutput_part0_label.add_to_parent = 0;
      evtOutput_part0_label.type = static_cast<supera::ProcessType>(13);
      evtOutput_part0_label.trackid_v = {  };
      supera::VoxelSet evtOutput_part0_label_energyVoxSet;
      evtOutput_part0_label_energyVoxSet.id(18446744073709551615ul);
      evtOutput_part0_label_energyVoxSet.reserve(860);
      supera::Voxel evtOutput_part0_label_energyVoxSet_vox0;
      evtOutput_part0_label_energyVoxSet_vox0.set(static_cast<supera::VoxelID_t>(891368779), 0.351079);
      evtOutput_part0_label_energyVoxSet.emplace(std::move(evtOutput_part0_label_energyVoxSet_vox0), false);
      evtOutput_part0_label.energy = std::move(evtOutput_part0_label_energyVoxSet);
      supera::VoxelSet evtOutput_part0_label_dedxVoxSet;
      evtOutput_part0_label_dedxVoxSet.id(18446744073709551615ul);
      evtOutput_part0_label_dedxVoxSet.reserve(0);
      evtOutput_part0_label.dedx = std::move(evtOutput_part0_label_dedxVoxSet);
      supera::EDep evtOutput_part0_label_firstEdep;
      evtOutput_part0_label_firstEdep.x = 1.79769e+308;
      evtOutput_part0_label_firstEdep.y = 1.79769e+308;
      evtOutput_part0_label_firstEdep.z = 1.79769e+308;
      evtOutput_part0_label_firstEdep.t = 1.79769e+308;
      evtOutput_part0_label_firstEdep.e = 1.79769e+308;
      evtOutput_part0_label_firstEdep.dedx = 1.79769e+308;
      evtOutput_part0_label.first_pt = std::move(evtOutput_part0_label_firstEdep);
      supera::EDep evtOutput_part0_label_lastEdep;
      evtOutput_part0_label_lastEdep.x = 1.79769e+308;
      evtOutput_part0_label_lastEdep.y = 1.79769e+308;
      evtOutput_part0_label_lastEdep.z = 1.79769e+308;
      evtOutput_part0_label_lastEdep.t = 1.79769e+308;
      evtOutput_part0_label_lastEdep.e = 1.79769e+308;
      evtOutput_part0_label_lastEdep.dedx = 1.79769e+308;
      evtOutput_part0_label.last_pt = std::move(evtOutput_part0_label_lastEdep);
      evtOutput.Particles().push_back(std::move(evtOutput_part0_label));


      ret.output_labels = std::move(evtOutput);



      return ret;
    }
  }
}
