# Trace-Enabled BookSim Simulator

This project is the cycle-accurate on-chip interconnection simulator, which is based on Booksim ( further described in below ). We modify the simulation kernel of Booksim to enable users to specify the traffic with trace files. 

The trace file describes the traffic as a communication graph, where each node denotes a computation core and its attached router, and edges are traffic flows between cores. A typical representation of a node is: 

```pseudocode
0 2
4608 32 2 17 51 0
4608 32 2 17 143 0
```

The first line is the global id of the node and its attached flow number. If a flow is attached to a node, it's injected by it. The two subsequent lines describe the traffic flows, which are given by *interval, max_iteration_cnt, waiting_flow_cnt, flits_per_message, dest_id, source_id* 

We model the collective communication as set of point-to-point traffic flows between all source nodes and all destinations. 

* The number of simulated cores should not exceed 1500
* Set `traffic = focus` and `injection_process = focus` in the config files, the example configuration file is **src/examples/focusconfig**
* We just support repeated traffic flows now.







==Below is the original README file== 

# BookSim Interconnection Network Simulator

BookSim is a cycle-accurate interconnection network simulator. Originally developed for and introduced with the [Principles and Practices of Interconnection Networks](http://cva.stanford.edu/books/ppin/) book, its functionality has since been continuously extended. The current major release, BookSim 2.0, supports a wide range of topologies such as mesh, torus and flattened butterfly networks, provides diverse routing algorithms and includes numerous options for customizing the network's router microarchitecture.

------

If you use BookSim in your research, we would appreciate the following citation in any publications to which it has contributed:

Nan Jiang, Daniel U. Becker, George Michelogiannakis, James Balfour, Brian Towles, John Kim and William J. Dally. A Detailed and Flexible Cycle-Accurate Network-on-Chip Simulator. In *Proceedings of the 2013 IEEE International Symposium on Performance Analysis of Systems and Software*, 2013.

