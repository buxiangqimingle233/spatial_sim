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
git submodule update --init --recursive --remote
```

Build the project:
```bash
# build
mkdir build
cd build
cmake .. && make -j
```

Run the demo benchmark: 
```
build/bin/spatialsim tasks/4-core-1-gemm/spatial_spec
```

Configurations of this simulator are specified in this file. 

## Notes

* Use `git submodule update --init --recursive --remote` to track submodules with the latest version. 
* WARNING: The above command will NOT pull & merge submodules to their master branches, but create NULL branches instead. You should enter the submodule and create a temporal branch using ``git checkout -b temp`` to hold the commit you just pulled. You can work on this branch, and merge it to the `master` branch.

