# A Simulator for Spatial Architectures

## Project Organization

* spatial_simulator: This repository

   * src/noc: On-chip network simulator based on booksim.
      > git@github.com:buxiangqimingle233/spatialsim_noc.git

   * src/core: Tenstorrent-like computing cores.
      > git@github.com:zjc990112/SpatialSim.git


## Prerequisites

* cmake: version >= 3.5
* bison & flex (optional)

## How to Use ? 

```bash
# build
mkdir build
cd build
cmake .. && make -j
```

Executable files locate at the build/bin folder. Either `core` or `noc` will be found, depending on the compilied parts that /CMakeLists.txt specificies. 

## Notes

We haven't integrated `noc` and `core` to a single simulator yet, it still needs some efforts ... 