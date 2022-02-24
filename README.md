# SuperaAtomic

C++ software to generate true labels for `lartpc_mlreco3d` ([repository](https://github.com/DeepLearnPhysics/lartpc_mlreco3d)), a full data reconstruction chain of modular machine learning (ML) algorithms for 3D particle images. The repository provides Python binding via [`pybind11`](https://pybind11.readthedocs.io/en/stable/), which can be optionally turned off. This software does not implement a process executable to run Supera. An implementation to run this software with [EDepSim](https://github.com/ClarkMcGrew/edep-sim) framework and to produce [larcv](https://github.com/DeepLearnPhysics/larcv3), please see [edep2supera](https://github.com/DeepLearnPhysics/edep2supera).

## How to build
1. Use [this docker image](https://hub.docker.com/layers/deeplearnphysics/larcv2/ub20.04-cuda11.3-cudnn8-pytorch1.10.0-larndsim/images/sha256-b9a67dfabf5190dbd67745cf739f9aeb6a357a6f4580df4702210bdfafa0221b?context=explore) (or any other container image derived from it) to get the _most_ of required softwares. Only additional item you would need is [scikit-build](https://scikit-build.readthedocs.io/en/latest/skbuild.html) which you can install with `pip`. 
  - If you know how to use `cmake` for building a software, you can actually skip `scikit-build` and run `cmake` from `src` directory.
2. Clone this repository (below) or fork-and-clone.
```
> git clone https://github.com/DeepLearnPhysics/SuperaAtomic
```
3. Build 
```
> python3 setup.py build
```
4. Install (the example below installs under your `$HOME/.local` path)
```
> python3 setup.py install --user
```

## How to contribute
1. Fork this repository to your personal github account.
2. Clone the repository to your local machine. Follow the build/install instruction above and make sure you can set up.
3. Create your branch to contain your own development.  Code code code.
4. When it's ready to be shared, make sure a unit test passes. Then request to merge by sending a pull request.

## Status

More documentation to come.
This repository is an attempt to decouple input file format dependency of the original toolkit `Supera` ([repository](https://github.com/DeepLearnPhysics/Supera)) and currently under development.

Milestones

- [x] Working cmake build with python binding

- [x] Migrate [SuperaMCParticleClusterData](https://github.com/DeepLearnPhysics/Supera/blob/icarus/SuperaMCParticleClusterData.h)

- [x] Migrate [SuperaBBoxInteraction](https://github.com/DeepLearnPhysics/Supera/blob/icarus/SuperaBBoxInteraction.h)

- [ ] Migrate [SuperaMCParticleCluster](https://github.com/DeepLearnPhysics/Supera/blob/icarus/SuperaMCParticleCluster.h)
