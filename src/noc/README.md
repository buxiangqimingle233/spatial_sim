# An Uniform Simulator Supporting Multiple Neural Network Architectures



This project is the cycle-accurate spatial architecture simulator. This project is based on Booksim, and modify its simulation kernel to enable users to specify traffic with trace files. 

### Traffic Specs

The trace file specifies traffic in the node-centric way. We use a `node` to denote the injection process of a single router. 
Node is futher composed of a list of `flows` that represent a fixed-interval packet stream. 

To describe a flow, you should provide the following attributes: 
* `fid`: an unique number to mark this flow ( in int32 )
* `interval / computing time`: the time duration to compute after all dependent packets arrive
* `counts`: the number of packets within the stream
* `depend`: the number of dependent packets
* `flit`: the packet size
* `dest`: nid of its destination
* `src`: nid of its source

List these attributes in a line and seperate them with space, for example, 
```pseudocode
413 6 512 2 1 19 3
```

After each flow-specification line, the list of dependent flows should be provided. Each dependency contains two attributes, `fid` and `ratio`. 
* `fid`: marks which packets it takes as the input
* `ratio`: denotes how much packets it consumes at once ( for each packet ), can be a float

Here's the complete description of one flow: 
```pseudocode
413 6 512 2 1 19 3
104    0.0020136
109    0.9900000
```

A node can contain various flows. First, head the node's specification with a line of node's information. You should provide two 
attributes at this line: 
* `nid`: the unique number to mark this node ( different with `fid` )
* `flow_cnt`: the number of flows in this node. 

Here's the complete description of one node: 
```pseudocode
3 1
413 6 512 2 1 19 3
104    0.0020136
109    0.9900000
```

You can also describe the traffic in a cycle-accurate way ( It is not fully supported yet. ).

In this way, the trace file specifies packet issuing time at each node. The details are described as follows. Each entry denotes when to issue a bit. 

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

### Runfiles

* We model the collective communication as set of point-to-point traffic flows between all source nodes and all destinations. 

* Set `traffic = focus` and `injection_process = focus` in the config files, the example configuration file is ***runfiles/focusconfig***

* When the cycle insufficiency error arises, please enlarge the `sample_period` at the runfile. 

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

