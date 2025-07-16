# SpatialSim

## Micro Instruction Formats

Network Interface: 
* Packet Send: NI.send dest_nid tid
> Ni.send 0 1
* Packet Receive: NI.recv tid
> NI.recv 1

AI Core: 
* Calculation: ACC.cal command out_tid in_tid1 in_tid2
    * ACC.cal conv out_tid weight_tid input_tid
    * ACC.cal einsum out_tid in_tid1 in_tid2

RISCV CPU:
* Polling: CPU.poll 
* Sleep: CPU.sleep
* Reshape: CPU.reshape tid dim0 dim1 dim2 ... 

BUS: 
* Transaction: BUS.trans dataid

Buffer: 
* Read: BUFFER.read dataid
* Write: BUFFER.write dataid


## Task Format
A task indicates the codes running on a core. It is composed of two fields: `operator` and `data`. 

The `operator` field is a list of tensor operators, whose order respones real execution order. 
Operators are composed of two categories: 

* `compute` takes several tensors in, computes on them, and generates a new tensor. Each `compute` operator has three sub-fields: compute description, 
input tensor ids and output tensor ids. The compute descriptions are in form of Eistein Expression, see xxx for further descriptions. Input tensor ids describe the tensors that this operator depends on. It should obey the order that the computation description specifies. 

* `manipulate` creates tensors, and send them out. This operator acts as a tensor generator, which regularly produces packets with the fixed interval. It runs without dependencies, and stop after a fixed iteration count.

The `data` field describes output tensors of the task. If an operator has no inputs, e.g. Memory read, to `create` the output tensor, you must provide whole dimensions. 
Otherwise, when the tensor's dimensions can be inferred from other tensors, e.g. the output tensor of a convolution, its dims can be omitted.

Here are the detailed formats for these two operator categories: 
```
operator: 
// config # output tensor ids # input tensor ids
compute # ij, jk -> i # 2 # 0, 1 
compute # ij, jk -> i # 5 # 3, 4 

data: 
// tensor id # destination nids # dims [omitted]
2 # 6 #
5 # 3 # 
```

```
operator: 
// interval # iter_cnt # out tids
manipulate # 5 # 10 # 0, 1, 3

data: 
// tensor id # destination nids # dims
0 # 5 # 2, 4, 8
1 # 6 # 4, 4, 4
3 # 6 # 4, 4, 4

```


## Path Format

Now SpatialSim supports multicast message, i.e. the message with multiple destinations. You need no additional efforts to leverage this feature, SpatialSim automatically recognizes a packet's type, i.e. unicast or multicast, based on the destinations of the tensor it carries. For example: 

This tensor results a multicast message. 
```
data: 
// tensor id # destination nids # dims
0 # 5, 6 # 2, 4, 8
```

This tensor results a unicast one.
```
data: 
// tensor id # destination nids # dims
0 # 5 # 2, 4, 8
```

You can control the routing path of each packet via `routing_board`. As metioned before, we have two packet types: `unicast` and `multicast`, and you're not outght to route for every `unicast` packets, but the path for the `multicast` packet is the must. 

* The path of a `unicast` packet is a list of intermediate nodes, path segments are x-y routes between two adjacent nodes. For example, on a $4 \times 4$ array, we have a packet from node $(0, 0)$ to node $(2, 2)$. If we have no intermediate nodes, the packet passes the minimal x-y routes: 
``
(0,0)->(0,1)->(0,2)->(1,2)->(2,2)
``
. When we have one intermediate node $(3,3)$, the packet detours to $(3,3)$ first, and then goes from this node to $(2,2)$. 

* A `multicast` packet is described as a tree of `unicast` packets. Each node is an `unicast` packet, and it splits to its children
as soon as reaching its destination. Noted that the spliting process is done in situ at the routers with zero latency overheads. 
To help SpatialSim to route `multicast` packets to their destinations, you should manually describe their trees via `routing_board`. 
To do so, you should describe the source, destination, intermediate nodes of each `unicast` packet within the multiacst tree.

Here's how to describe the path for a unicast packet: 
```
0 0 1        // tensor id | source | desination
0 1 2 5      // source | destination | several intermediate nodes
```

For multicast packet, the path likes: 
```
0 0 1 2     // tensor id | source | several desinations
0 1         // unicast packet 1: source | destination | several intermediate nodes [omitted]
0 2 4       // unicast packet 2: source | destination | several intermediate nodes 
```

The tree is inferred internally from the endpoints of packets: A packet `a` is the succeed of a packet `b` if the destination of `a` is the source of packet `b`. The dummy packet, i.e. the packet doesn't target to any real destinations, will be destroyed by SpatialSim once reaching its fake destinating router. 