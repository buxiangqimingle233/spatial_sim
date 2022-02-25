#ifndef __FOCUS_H__
#define __FOCUS_H__

#include <fstream>
#include <iostream>
#include <vector>
#include <map>
#include <memory>
#include <algorithm>
#include <queue>
#include <string.h>


namespace focus
{

extern int cl0_nodes;
extern int cl0_routers;
extern int channel_width;
extern std::string trace;

#define INVALID -1
typedef int FlowID;


struct PktHeader {
    int flow_id;
    int size;
    int dst;
    int src;

    PktHeader(int flow_id_, int size_, int dst_, int src_): flow_id(flow_id_), size(size_), dst(dst_), src(src_) { };
};

class Flow {

// Unchangable features
public:
    int _flow_id;        // An unique flow id
    int _computing_time; // computing time
    int _interval;
    long long _max_iter; // we model the traffic flow as a regurally happened issue
    std::map<int, double> _depend_flows; // The map indicates which flows and how much packets this 
                                    // flow depends on
    int _size; // flits per packet
    int _dst;
    int _src;

// DFA attributes
public:
    enum FlowState { Waiting, Computing, Arbitrating, Closed, Init };
    int _slept_time;
    long long _iter;
    FlowState _state;
    std::map<int, double> _received_packet;    // Received pakcets, a map of flowid-number;

public:
    Flow(int id, int computing_time, long long max_iter, std::map<int, double> depend_flows, int size, int src, int dst):
        _flow_id(id), _computing_time(computing_time), _max_iter(max_iter), _depend_flows(depend_flows),
        _size(size), _src(src), _dst(dst), _slept_time(0), _iter(0), _state(FlowState::Init) { 
            _interval = computing_time;
            _received_packet = std::map<int, double>();
            for (auto it : _depend_flows) {
                _received_packet[it.first] = 0.;
            }
        };

    PktHeader genPacketHeader() { return PktHeader(_flow_id, _size, _dst, _src); };

    // For the nodes with multiple flows to issue to check which
    bool canIssue() { return _state == FlowState::Arbitrating; };
    bool isClosed() { return _state == FlowState::Closed; };

    // Just for debugging
    int getComputingTime() { return _computing_time; };
    int getState() { return _state; };
    int getSleptTime() { return _slept_time; };
    int getIter() { return _iter; };
    int getMaxIter() { return _max_iter; };
    std::map<int, double> getReceivedPacket() { return _received_packet; }

    // Drive the DFM
    void forward(bool arbitration);
    void receiveDependentPacket(int flow_id);

};


// The computing core or the processing elements which directly attatch to the NoC.
class Node {

protected:
    int _node_id;
    std::vector<Flow> _out_flows;

public:
    Node(): _node_id(-1), _out_flows(std::vector<Flow>()) { };
    Node(std::vector<Flow> flows, int node_id): _out_flows(flows), _node_id(node_id) { }; 

    // Producer
    virtual void step(std::queue<PktHeader>& _send_queue);

    // virtual int getDestination(); // the destination of active flow with token
    // virtual int getFlowSize();
    // virtual int getFlowID();

    // General API
    virtual bool isClosed();

    // Optional for Async
    void receivePacket(int flow_id);

public:
    // for debugging
    void dump(std::ofstream& ofs);
};

class SyncNode: public Node {

protected:
    std::queue<std::pair<int, int> > _lut;
    int _flits_to_issue, _dst_to_issue;

public:
    SyncNode(): Node(), _flits_to_issue(-1), _dst_to_issue(-1) { }
    SyncNode(std::ifstream& ifs, int node_id);

    int test(int time);
    virtual bool isClosed();
    virtual int getDestination(); 
    virtual int getFlowSize();
};


class FocusInjectionKernel {

protected:
    std::vector<Node*> _nodes;
    std::vector<std::queue<PktHeader> > _send_queues;

    static std::shared_ptr<FocusInjectionKernel> _kernel;
    void checkNodes(int nid);

public:
    FocusInjectionKernel(int nodes);
    
    static std::shared_ptr<FocusInjectionKernel> getKernel();
    static void renewKernel();    

// Producer
public:
    void step();  // TODO: maintain a flag for each node [ producer-consumer buffer ]

// Consumer APIs for async simulation
public:
    // API for InjectionProcess
    bool test(int source);      // Check the flag, revert it if true
    void dequeue(int source);

    // API for TrafficPattern
    int dest(int source);       // invoke the Node for flows_to_inject
    // API for getting flow size
    int flowSize(int source);   // Bare
    // API for generating flows
    int flowID(int source);

// APIs for sync simulation
public:
    bool sync_test(int source, int time);
    int sync_dest(int source, int time);
    int sync_flowSize(int source, int time);

// For iteration with the booksim kernels
public:
    void retirePacket(int source, int dest, int flow_id);
    bool allNodesClosed(int cycle);
    float closeRatio(int cycle);

    void dump(int cycle, std::ofstream& ofs);
};

};

#endif