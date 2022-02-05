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

typedef int FlowID;

class Node;

class Flow {

// Unchangable features
protected:
    int _computing_time; // computing time
    int _interval;
    long long _max_iter; // we model the traffic flow as a regurally happened issue
    int _wait_flows; // number of depending flows
    int _size; // flits per packet
    int _dest;
    int _src;

// DFA attributes
protected:
    enum FlowState { Waiting, Computing, Blocked, Closed, Init };
    int _slept_time;
    long long _iter;
    FlowState _state;
    // std::map<int, int> _dependent_flows;
    std::queue<int> _dependent_flows;

public:
    // Flow() { };
    Flow(int computing_time, long long max_iter, int wait_flows, int size, int dest, int src):
        _computing_time(computing_time), _max_iter(max_iter), _wait_flows(wait_flows),
        _size(size), _dest(dest), _src(src), _slept_time(0), _iter(0) { 
            _state = FlowState::Init;
            _iter = _slept_time = 0;
            _interval = _size > _computing_time ? _size + 20 : _computing_time;
            _dependent_flows = std::queue<int>();
        };

    // For the nodes with multiple flows to issue to check which
    void setState(FlowState state) { _state = state; }
    bool canIssue() { return _state == FlowState::Blocked; };
    bool isClosed() { return _state == FlowState::Closed; };
    int getDestination() { return _dest; };
    int getFlowSize() { return _size; };
    int getComputingTime() { return _computing_time; }
    int forward(bool token); // return whether this flow is to be activate
    void arriveDependentFlow(int source);

    int getState() { return _state; }
};

// vertex-centric programming model
class Node {

protected:
    int _node_id;
    int _flow_with_token; // if a flow is to inject, it dictates which one
    std::vector<Flow> _out_flows;

public:
    Node();
    Node(std::ifstream& ifs);
    Node(std::ifstream& ifs, int node_id);

    int test(); // whether to inject a packet

    // General API
    // TODO: update with arguments: flow_id
    virtual int getDestination(); // the destination of active flow with token
    virtual int getFlowSize();
    virtual bool isClosed();

    // Optional for Async
    int getFlowID();
    int getComputingTime(int flow_id);
    void receiveFlow(int source);
    

    // for debug
    int getState() {
        return _out_flows[0].getState();
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
    static std::shared_ptr<FocusInjectionKernel> _kernel;
    void checkNodes(int nid);

public:
    FocusInjectionKernel(int nodes);
    
    static std::shared_ptr<FocusInjectionKernel> getKernel();
    static void renewKernel();    

// APIs for async simulation
public:
    // API for InjectionProcess
    bool test(int source);
    // API for TrafficPattern
    int dest(int source);
    // API for getting flow size
    int flowSize(int source);
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
    void updateFocusKernel(int source, int dest);
    bool allNodesClosed();
    float closeRatio();

    // For debugging
    void debug() {
        for (int i = 0; i < 100; ++i) {
            for (auto n : _nodes) {
                std::cout << n->test() << std::endl;
            }
        }
    }

    void checkState(int id) {
        printf("STATE %d\n", _nodes[id]->getState());
    }
};

};

#endif