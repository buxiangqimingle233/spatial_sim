# An Uniform Simulator Supporting Multiple Neural Network Architectures



This project is the cycle-accurate spatial architecture simulator. This project is based on Booksim, and modify its simulation kernel to enable users to specify traffic with trace files. 

### Traffic Specs

The trace file describes a communication graph, where each node denotes a computation core alongwith its attached router, and edges are traffic flows between cores. We support two methods to specifiy a communication. 

The first way to describe a communication graph is to build multiple Finite State Machines (FSM) for nodes. The FSM specifies a bunch of discrete packets with equally space. The details are described as follows: 

```pseudocode
0 2
4608 32 2 17 51 0
4608 32 2 17 143 0
```

The first line is the global id of the node along with its attached flow number. If a flow is attached to a node, it's injected by the node. The two subsequent lines describe the traffic flows attached to node $0$, which are given by **interval, max_iteration_cnt, waiting_flow_cnt, flits_per_message, dest_id, source_id** 



The second way to describe traffic is to maintain a look up table of packet issuing time for each node. The details are described as follows. Each entry denotes when to issue a bit. 

```pseudocode
1 7
99 6
99 11
99 2
473 6
473 11
473 2
1023 6
```

* We model the collective communication as set of point-to-point traffic flows between all source nodes and all destinations. 

* Set `traffic = focus` and `injection_process = focus` in the config files, the example configuration file is ***runfiles/focusconfig***

* We just support repeated traffic flows now.

### Prerequisites

FREE OF DEPENDENCE~

### How to build

This project supports to be built in two modes to parse the two graph specification ways as mentioned above. 

Run `python run.py build --target ann` to build the project to support the first traffic specification way, and run `python run.py build --target snn` for the second way. 

### How to use

First, you should store the benchmark in the "benchmark" folder, then you can run this simulator in three modes: 

1. Testing single benchmark in the first mode: `python run.py single-a --bm your_benchmark_name`
   Tesing single benchmark in the second mode: `python run.py single-a --bm your_benchmark_name`
2. Comparing the performance of two benchmarks: `python run.py comp --bm benchmark1 benchmark2`
3. Testing all benchmarks under the 'benchmark' folder: `python run.py all`

