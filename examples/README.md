# SpatialSim Examples

This directory contains example workloads for the SpatialSim simulator.

## Ring Communication Example

The `ring/` directory contains a ring all-reduce communication pattern example that demonstrates synchronized communication between 4 cores in a ring topology.

### Overview

The ring example implements a lock-step communication pattern where 4 cores (c0-c3) form a ring and pass data around in a synchronized manner. Each core follows the same pattern: send data to the next core, receive data from the previous core, then perform computation.

### Ring Topology

```
Core 0 ──→ Core 1 ──→ Core 2 ──→ Core 3 ──┐
  ↑                                        │
  └────────────────────────────────────────┘
```

### Communication Pattern

Each core follows this lock-step sequence:
1. **Send** tensor to successor core
2. **Receive** tensor from predecessor core  
3. **Compute** (sleep for 50 cycles)
4. **Repeat** for next tensor

### Tensor Flow

- **Core 0**: Sends tensors 100-104 → Core 1, Receives tensors 400-404 ← Core 3
- **Core 1**: Sends tensors 200-204 → Core 2, Receives tensors 100-104 ← Core 0
- **Core 2**: Sends tensors 300-304 → Core 3, Receives tensors 200-204 ← Core 1
- **Core 3**: Sends tensors 400-404 → Core 0, Receives tensors 300-304 ← Core 2

### Tensor ID Mapping

| Core | Sends | Receives |
|------|-------|----------|
| Core 0 | 100-104 → Core 1 | 400-404 ← Core 3 |
| Core 1 | 200-204 → Core 2 | 100-104 ← Core 0 |
| Core 2 | 300-304 → Core 3 | 200-204 ← Core 1 |
| Core 3 | 400-404 → Core 0 | 300-304 ← Core 2 |

### Key Features

- **Deadlock Prevention**: Each core starts with a send operation to ensure data flow
- **Synchronized Execution**: All cores work in lock-step with coordinated communication
- **Computation Interleaved**: Each core performs computation between communication steps
- **Ring All-Reduce**: Mimics distributed training patterns where data flows around a ring

### Running the Example

```bash
# Build the simulator
./rebuild.sh

# Run the ring example
./build/bin/spatialsim ./examples/ring/task_spec
```

### Files

- `c0.inst`: Core 0 instruction trace
- `c1.inst`: Core 1 instruction trace  
- `c2.inst`: Core 2 instruction trace
- `c3.inst`: Core 3 instruction trace
- `task_spec`: Task specification file

This example demonstrates how SpatialSim can model synchronized communication patterns commonly used in distributed computing and neural network training. 