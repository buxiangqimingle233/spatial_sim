# SpatialSim: A Simulator for Spatial Architectures

## Overview

SpatialSim is a cycle-accurate simulator for spatial computing architectures, designed to model systems with multiple computing cores connected via an on-chip network. The simulator provides detailed performance analysis and timing simulation for spatial computing workloads.

## Architecture

The simulator consists of two main components:

* **Computing Cores**: Tenstorrent-like spatial computing cores that execute tensor operations
* **Network-on-Chip (NoC)**: On-chip network simulator based on BookSim for inter-core communication

## Core Structure

Each computing core contains multiple **operators** that can execute in parallel. Each operator consists of a sequence of **assembles** that trigger different modules within the core.

### Core Modules

The core includes several functional modules:

* **CPU**: RISC-V processor for control flow and data manipulation
  * `CPU.sleep <cycles>`: Sleep for specified number of cycles
  * `CPU.poll`: Polling operation
  * `CPU.reshape <tid> <dim0> <dim1> <dim2> ...`: Reshape tensor dimensions

* **Network Interface (NI)**: Handles packet communication
  * `NI.send <tid> <dest_nid>`: Send tensor to destination node
  * `NI.recv <tid>`: Receive tensor

* **Accelerator (ACC)**: Specialized compute units
  * `ACC.cal <command> <out_tid> <in_tid1> <in_tid2>`: Perform computation
  * `ACC.cal conv <out_tid> <weight_tid> <input_tid>`: Convolution operation
  * `ACC.cal einsum <out_tid> <in_tid1> <in_tid2>`: Einstein summation

* **Bus**: Inter-module communication
  * `BUS.trans <dataid>`: Bus transaction

* **Buffer**: Local memory management
  * `BUFFER.read <dataid>`: Read from buffer
  * `BUFFER.write <dataid>`: Write to buffer

## Instruction Format

### Task Structure

A task defines the code running on a core and consists of two main sections:

1. **operators**: List of operators with their execution sequence
2. **data**: Tensor metadata and routing information

### Operator Format

Each operator is enclosed in braces `{}` and contains a sequence of assembles:

```
{
assemble # CPU.sleep 128
assemble # NI.send 32 8
assemble # NI.send 33 16
assemble # CPU.sleep 128
...
}
```

### Data Format

The data section describes tensor information with the format:
```
<tid> # <dest_nid> # <size_in_flits>
```

Where:
* `tid`: Tensor ID (unique identifier)
* `dest_nid`: Destination node ID (core to send to)
* `size_in_flits`: Size of the tensor in network flits

Example:
```
32 # 8 # 4.0
33 # 16 # 4.0
2195 # 40 # 99.0
```

## Project Organization

* **spatial_simulator**: Main simulator repository
  * `src/noc`: On-chip network simulator (BookSim-based)
  * `src/core`: Tenstorrent-like computing cores

## Prerequisites

* cmake: version >= 3.5
* bison & flex (optional)

## Building and Running

### Setup Submodules
```bash
git submodule update --init --recursive --remote
```

### Build the Project
```bash
./rebuild.sh
```

### Run a Benchmark
```bash
build/bin/spatialsim tasks/4-core-1-gemm/spatial_spec
```

## Notes

* Use `git submodule update --init --recursive --remote` to track submodules with the latest version
* WARNING: The above command will NOT pull & merge submodules to their master branches, but create NULL branches instead. You should enter the submodule and create a temporal branch using `git checkout -b temp` to hold the commit you just pulled
* Using wrapper takes about 6% more time than directly executing the binary