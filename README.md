# A Simulator for Spatial Architectures

## Project Organization

* spatial_simulator: This repository

   * src/noc: On-chip network simulator based on booksim.
      > git@github.com:mass-spatialarch/noc.git

   * src/core: Tenstorrent-like computing cores.
      > git@github.com:mass-spatialarch/core.git


## Prerequisites

* cmake: version >= 3.5
* bison & flex (optional)

## How to Use ? 

Setup submodules: 

```bash
git submodule update --init --recursive
```

```bash
# build
mkdir build
cd build
cmake .. && make -j
```

The executable file is build/bin/spatialsim. Run it without any paramters, we encode the runfile path with codes now. 


## Notes

* Use `git submodule update --init --recursive --remote` to track submodules with the latest version. 
* WARNING: The above command will NOT pull & merge submodules to their master branches, but create NULL branches instead. You should enter the submodule and create a temporal branch using ``git checkout -b temp`` to hold the commit you just pulled. You can work on that branch, and push `temp` to the `master` branch in remote.
