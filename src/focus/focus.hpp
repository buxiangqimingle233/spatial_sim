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

class Node;

class Flow {

// Unchangable features
protected:
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
protected:
    enum FlowState { Waiting, Computing, Arbitrating, Commiting, Closed, Init };
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

    // For the nodes with multiple flows to issue to check which
    void setState(FlowState state) { _state = state; }
    bool canIssue() { return _state == FlowState::Arbitrating; };
    bool isClosed() { return _state == FlowState::Closed; };
    int getDestination() { 
        return _dst; 
    };
    int getFlowSize() { return _size; };
    int getComputingTime() { return _computing_time; };
    int getState() { return _state; };
    int getFlowID() { return _flow_id; };
    int getSleptTime() { return _slept_time; };
    int getIter() { return _iter; };
    int getMaxIter() { return _max_iter; };
    std::map<int, double> getReceivedPacket() { return _received_packet; }

    void receiveDependentPacket(int flow_id);
    int forward(bool arbitration, bool allocation);    // return whether this flow is active

};

// The computing core or the processing elements which directly attatch to the NoC.
class Node {

protected:
    int _node_id;
    int _flow_to_inject; // if a flow is to inject, it dictates which one
    std::vector<Flow> _out_flows;

public:
    Node(): _node_id(-1), _flow_to_inject(0), _out_flows(std::vector<Flow>()) { };
    Node(std::vector<Flow> flows, int node_id): _out_flows(flows), _node_id(node_id), 
                                                _flow_to_inject(0) { }; 

    // Producer
    virtual bool step(bool buffer_empty);    // TODO: forward flows, maintain which flow should be issued, return whether to reverse the flag.
                                    // if the buffer full, do not give the flag to any flow;
                                    // on the contray, if we give the flag to a flow, the buffer must be empty

    // General API
    virtual int getDestination(); // the destination of active flow with token
    virtual int getFlowSize();
    virtual int getFlowID();
    virtual bool isClosed();

    // Optional for Async
    int getComputingTime(int flow_id);
    void receivePacket(int flow_id);

public:
    // for debugging
    void dump(std::ofstream& ofs) {
        if (_out_flows.empty()) {
            return;
        }
        ofs << "node " << _node_id << ": " << "flow_to_inject: " << _flow_to_inject << std::endl;
        for (auto f : _out_flows) { 
            ofs << "fid: " << f.getFlowID() << ", state: " << f.getState() << ", comp-slept: " \
                << f.getComputingTime() << "-" << f.getSleptTime() << ", iter-max_iter: " \
                << f.getIter() << "-" << f.getMaxIter();
            ofs << ", received packets: ";
            for (auto p: f.getReceivedPacket()) {
                ofs << p.first << "-" << p.second << " ";
            }
            ofs << std::endl;
        }
    }
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
    std::vector<bool> _buffer_empty;
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
    // API for TrafficPattern
    int dest(int source);       // invoke the Node for flows_to_inject
    // API for getting flow size
    int flowSize(int source);   // Bare
    // API for generating flows
    int flowID(int source);
    // API for getting interval
    int interval(int source, int flow_id);

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