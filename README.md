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

```bash
# build
mkdir build
cd build
cmake .. && make -j
```

The executable file is build/bin/spatialsim. Run it without any paramters, we encode the runfile now. 


## Notes

We haven't integrated `core` yet, it still needs some efforts ... 