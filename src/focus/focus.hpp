#ifndef __FOCUS_H__
#define __FOCUS_H__

#include <fstream>
#include <iostream>
#include <vector>
#include <map>
#include <memory>
#include <algorithm>

namespace focus
{

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
    std::map<int, int> _dependent_flows;

public:
    // Flow() { };
    Flow(int computing_time, long long max_iter, int wait_flows, int size, int dest, int src):
        _computing_time(computing_time), _max_iter(max_iter), _wait_flows(wait_flows),
        _size(size), _dest(dest), _src(src), _slept_time(0), _iter(0) { 
            _state = FlowState::Init;
            _iter = _slept_time = 0;
            _interval = _size > _computing_time ? _size + 20 : _computing_time;
            _dependent_flows = std::map<int, int>();
        };

    // For the nodes with multiple flows to issue to check which
    bool canIssue() { return _state == FlowState::Blocked; };
    bool isClosed() { return _state == FlowState::Closed; };
    int getDestination() { return _dest; };
    int getFlowSize() { return _size; };
    int getComputingTime() { return _computing_time; }
    int forward(bool token); // return whether this flow is to be activate
    void arriveDependentFlow(int source);
};

// vertex-centric programming model
class Node {

protected:
    int _node_id;

protected:
    int _flow_with_token; // if a flow is to inject, it dictates which one
    std::vector<Flow> _out_flows;

public:
    // Node() {}
    Node(std::ifstream& ifs);
    Node(std::ifstream& ifs, int node_id);

    int test(); // whether to inject a packet
    // TODO: update with arguments: flow_id
    int getDestination(); // the destination of active flow with token
    int getFlowSize();
    int getFlowID();
    int getComputingTime(int flow_id);

    void receiveFlow(int source);
    bool isClosed();
};


class FocusInjectionKernel {

protected:
    std::vector<Node> _nodes;
    static std::shared_ptr<FocusInjectionKernel> _kernel;
    void checkNodes(int nid);

public:
    FocusInjectionKernel();
    FocusInjectionKernel(int nodes);
    
    static std::shared_ptr<FocusInjectionKernel> getKernel();
    static void renewKernel();    

// For statistics
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

// For iteration with the booksim kernels
public:
    void updateFocusKernel(int source, int dest);
    bool allNodeClosed();

    // For debugging
    void debug() {
        for (int i = 0; i < 100; ++i) {
            for (auto& n : _nodes) {
                std::cout << n.test() << std::endl;
            }
        }
    }
};

}

#endif