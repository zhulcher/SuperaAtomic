# SuperaAtomic

C++ software to generate true labels for `lartpc_mlreco3d` ([repository](https://github.com/DeepLearnPhysics/lartpc_mlreco3d)), a full data reconstruction chain of modular machine learning (ML) algorithms for 3D particle images. The repository provides Python binding via [`pybind11`](https://pybind11.readthedocs.io/en/stable/), which can be optionally turned off. The output file format is [`larcv3`](https://github.com/DeepLearnPhysics/larcv3).

More documentation to come.
This repository is an attempt to decouple input file format dependency of the original toolkit `Supera` ([repository](https://github.com/DeepLearnPhysics/Supera)) and currently under development.

Milestones
[x] Working cmake build with python binding
[x] Migrate [SuperaMCParticleClusterData](https://github.com/DeepLearnPhysics/Supera/blob/icarus/SuperaMCParticleClusterData.h)
[ ] Migrate [SuperaMCParticleCluster](https://github.com/DeepLearnPhysics/Supera/blob/icarus/SuperaMCParticleCluster.h)