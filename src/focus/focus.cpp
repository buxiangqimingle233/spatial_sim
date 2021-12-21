#include "focus.hpp"
#include <iostream>

namespace focus {

std::shared_ptr<FocusInjectionKernel> FocusInjectionKernel::_kernel = NULL;

int Flow::forward(bool token) {
    FlowState next_state = FlowState::Closed;
    int should_issue = false;

    switch (_state) {
        case FlowState::Init: {
            next_state = FlowState::Waiting;
            should_issue = false;
            break;
        }
        case FlowState::Waiting: {
            std::vector<std::map<int, int>::iterator> available_flows;
            for (auto it = _dependent_flows.begin(); it != _dependent_flows.end(); ++it) {
                if (it->second > 0) {
                    available_flows.push_back(it);
                }
            }
            // FIXME: we just maintain the number of dependent flows
            if (static_cast<int>(available_flows.size()) >= _wait_flows) {
                for (auto it: available_flows) {
                    it->second--;
                }
                next_state = FlowState::Computing;
                should_issue = false;
            } else {
                next_state = FlowState::Waiting;
                should_issue = false;
            }
            break;
        }
        // Is doing computing now, the flow is sleeping ...
        case FlowState::Computing: {
            if (_slept_time < _interval) {
                _slept_time++;
                next_state = FlowState::Computing;
                should_issue = false;
            } else {
                _slept_time = 0;
                next_state = FlowState::Blocked;
                should_issue = false;
            }
            break;
        }
        // Computation is done, 
        // TODO: waiting the destination node to be available [ not implemented yet ]
        // Update: Blocked state now dictates the state waiting for token. 
        case FlowState::Blocked: {
            if (token) {
                if (_iter > _max_iter) {
                    next_state = FlowState::Closed;
                    should_issue = false;
                } else {
                    _iter++;
                    next_state = FlowState::Waiting;
                    should_issue = true;
                }
            } else {
                next_state = FlowState::Blocked;
                should_issue = false;
            }
            break;
        }
        // Dead here
        default: {
            next_state = FlowState::Closed;
            should_issue = false;
            break;
        }
    }

    _state = next_state;
    return should_issue;
}

void Flow::arriveDependentFlow(int source) {
    if (_dependent_flows.find(source) == _dependent_flows.end()) {
        _dependent_flows.insert(std::make_pair(source, 1));
    } else {
        _dependent_flows[source]++;
    }
}

Node::Node(std::ifstream& ifs): _node_id(-1), _flow_with_token(0) {
    int flow_size;
    ifs >> _node_id >> flow_size;

    for (int _ = 0; _ < flow_size; ++_) {
        int ct, mi, d, s, size, wf;
        ifs >> ct >> mi >> d >> s >> size >> wf;
        Flow flow(ct, mi, d, s, size, wf);
        _out_flows.push_back(flow);
    }
}

Node::Node(std::ifstream& ifs, int node_id): _node_id(node_id), _flow_with_token(0) {
    int flow_size;
    ifs >> flow_size;

    for (int _ = 0; _ < flow_size; ++_) {
        int ct, mi, d, s, size, wf;
        ifs >> ct >> mi >> d >> s >> size >> wf;
        Flow flow(ct, mi, d, s, size, wf);
        _out_flows.push_back(flow);
    }
}

int Node::test() {

    if (_out_flows.size() == 0) {
        return 0;
    }
    
    // select a flow to issue, using Round-Robin scheduling
    int sel_flow = _flow_with_token;
    int size_ = _out_flows.size();
    for (int bias = 1; bias <= size_; ++bias) {
        int idx = (bias + _flow_with_token) %size_;
        if (_out_flows[idx].canIssue()) {
            sel_flow = idx;
            break;
        }
    }
    _flow_with_token = sel_flow;

    bool should_issue = false;
    for (int i = 0; i < size_; ++i) {
        should_issue |= _out_flows[i].forward(i == sel_flow);
    }

    return should_issue;
}

int Node::getDestination() {
    return _out_flows[_flow_with_token].getDestination();
}

int Node::getFlowSize() {
    // std::cout << "Size: " << _out_flows[_flow_with_token].getFlowSize() << std::endl;
    return _out_flows[_flow_with_token].getFlowSize();
}

int Node::getFlowID() {
    return _flow_with_token;
}

int Node::getComputingTime(int flow_id) {
    return _out_flows[flow_id].getComputingTime();
}

void Node::receiveFlow(int source) {
    // FIXME: It's wrong when a PE is mapped with multiple flows
    for (auto& f : _out_flows) {
        f.arriveDependentFlow(source);
    }
}

bool Node::isClosed() {
    for (Flow& it: _out_flows) {
        if (!it.isClosed()) {
            return false;
        }
    }
    return true;
}

FocusInjectionKernel::FocusInjectionKernel() {
    std::ifstream ifs;
    ifs.open("trace.txt", std::ios::in);

    if (!ifs.is_open()) {
        std::cout << "Error: Trace file not found" << std::endl;
        exit(-1);
    }

    int node_id;
    while ((ifs >> node_id) && !ifs.eof()) {
        Node node(ifs, node_id);
        _nodes.push_back(node);
    }

}

FocusInjectionKernel::FocusInjectionKernel(int nodes) {

    // FocusInjectionKernel();
    if (static_cast<int>(_nodes.size()) != nodes) {
        std::cerr << "ERROR: Nodes in trace file does not match specs for Booksim !" << std::endl;
        exit(-1);
    }
}

std::shared_ptr<FocusInjectionKernel> FocusInjectionKernel::getKernel() {
    if (_kernel) {
        return _kernel;
    } else {
        _kernel = std::make_shared<FocusInjectionKernel>();
        return _kernel;
    }
}

void FocusInjectionKernel::renewKernel() {
    std::cerr << "INFO: injection kernel is renewed" << std::endl;
    if (_kernel) {
        _kernel.reset();
        _kernel = std::make_shared<FocusInjectionKernel>();
    }
}

void FocusInjectionKernel::checkNodes(int nid) {

    if (nid >= static_cast<int>(_nodes.size()) || nid < 0) {
        std::cerr << "ERROR focus: node " << nid << " is not recorded in the trace file." << std::endl;
        exit(-1);
    }
}
 
bool FocusInjectionKernel::test(int source) {
    checkNodes(source);
    return _nodes[source].test();
}

int FocusInjectionKernel::dest(int source) {
    checkNodes(source);
    return _nodes[source].getDestination();
}

int FocusInjectionKernel::flowSize(int source) {
    checkNodes(source);
    return _nodes[source].getFlowSize();
}

int FocusInjectionKernel::flowID(int source) {
    return _nodes[source].getFlowID();
}

int FocusInjectionKernel::interval(int source, int flow_id) {
    return _nodes[source].getComputingTime(flow_id);
}

void FocusInjectionKernel::updateFocusKernel(int source, int dest) {
    checkNodes(source);
    checkNodes(dest);
    _nodes[dest].receiveFlow(source);
}

bool FocusInjectionKernel::allNodeClosed() {
    for (Node& node: _nodes) {
        if (!node.isClosed()) {
            return false;
        }
    }
    return true;
}

}