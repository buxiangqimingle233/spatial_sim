#include "focus.hpp"
#include <iostream>
#include <math.h>

namespace focus 
{

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
            // FIXME: we just check the number of arrival packets. It'll be wrong when 
            // multiple packets belong to one traffic flow. 

            // std::vector<std::map<int, int>::iterator> available_flows;
            // for (auto it = _dependent_flows.begin(); it != _dependent_flows.end(); ++it) {
            //     if (it->second > 0) {
            //         available_flows.push_back(it);
            //     }
            // }
            // if (static_cast<int>(available_flows.size()) >= _wait_flows) {
            //     for (auto it: available_flows) {
            //         it->second--;
            //     }
            if (_wait_flows != 0) {
                // std::cout << "dependent flows: " << _dependent_flows.size() << std::endl;
            }
            if (_dependent_flows.size() >= static_cast<unsigned>(_wait_flows)) {
                for (int _; _ < _wait_flows; ++_) {
                    _dependent_flows.pop();
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
    _dependent_flows.push(source);
    // if (_dependent_flows.find(source) == _dependent_flows.end()) {
    //     _dependent_flows.insert(std::make_pair(source, 1));
    // } else {
    //     _dependent_flows[source]++;
    // }
}

Node::Node():_node_id(-1), _flow_with_token(0) { }

Node::Node(std::ifstream& ifs): _node_id(-1), _flow_with_token(0) {
    int flow_size;
    ifs >> _node_id >> flow_size;

    for (int _ = 0; _ < flow_size; ++_) {
        // interval, max_iteration_cnt, waiting_flow_cnt, flits_per_message, dest_id, source_id
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
        int ct, mi, d, s, size, dst;
        ifs >> ct >> mi >> d >> size >> s >> dst;
        Flow flow(ct, mi, d, size, s, dst);
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

SyncNode::SyncNode(std::ifstream& ifs, int node_id): Node(), _flits_to_issue(-1), _dst_to_issue(-1) {
    _node_id = node_id;

    int pkt_num;
    ifs >> pkt_num;

    for (int _ = 0; _ < pkt_num; ++_) {
        int offset, dest_id;
        ifs >> offset >> dest_id;
        _lut.push(std::make_pair(offset, dest_id));
    }
}

bool SyncNode::isClosed() {
    return _lut.empty();
}

int SyncNode::test(int time) {

    if (isClosed()) {
        return 0;
    }

    int bits_to_issue = 0;
    int dest = _lut.front().second;

    while (!_lut.empty() && _lut.front().first <= time && _lut.front().second == dest) {
        bits_to_issue++;
        _lut.pop();
    }

    if (bits_to_issue > 0) {
        _flits_to_issue = ceil(bits_to_issue / (double) channel_width);
        _dst_to_issue = dest;
        return 1;
    } else {
        return 0;
    }
}

int SyncNode::getDestination() {
    return _dst_to_issue;
}

int SyncNode::getFlowSize() {
    return _flits_to_issue;
}

FocusInjectionKernel::FocusInjectionKernel(int nodes) {
    _nodes.resize(nodes, NULL);
    
    std::ifstream ifs;
    ifs.open(trace, std::ios::in);

    if (!ifs.is_open()) {
        std::cout << "Error: Trace file not found" << std::endl;
        exit(-1);
    }

    int node_id;
    while ((ifs >> node_id) && !ifs.eof()) {
        if (node_id > nodes) {
            std::cerr << "Node " << node_id << " specified in trace file exceeds node range" << std::endl;
        }
        // FIXME: ??
#ifdef SYNC_SIM
        SyncNode* node = new SyncNode(ifs, node_id);
#else
        Node* node = new Node(ifs, node_id);
#endif
        _nodes[node_id] = node;
    }

    for (Node*& node : _nodes) {
        if (node == NULL) {
#ifdef SYNC_SIM
            node = new SyncNode();
#else
            node = new Node();
#endif  
        }
    }

    // if (static_cast<int>(_nodes.size()) != nodes) {
    //     std::cerr << "ERROR: Nodes in trace file does not match specs for Booksim !" << std::endl;
    //     exit(-1);
    // }
}

std::shared_ptr<FocusInjectionKernel> FocusInjectionKernel::getKernel() {
    if (_kernel) {
        return _kernel;
    } else {
        _kernel = std::make_shared<FocusInjectionKernel>(cl0_nodes);
        return _kernel;
    }
}

void FocusInjectionKernel::renewKernel() {
    std::cout << "INFO: injection kernel is renewed" << std::endl;
    if (_kernel) {
        _kernel.reset();
        _kernel = std::make_shared<FocusInjectionKernel>(cl0_nodes);
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
    return _nodes[source]->test();
}

int FocusInjectionKernel::dest(int source) {
    checkNodes(source);
    return _nodes[source]->getDestination();
}

int FocusInjectionKernel::flowSize(int source) {
    checkNodes(source);
    return _nodes[source]->getFlowSize();
}

int FocusInjectionKernel::flowID(int source) {
    checkNodes(source);
    return _nodes[source]->getFlowID();
}

int FocusInjectionKernel::interval(int source, int flow_id) {
    checkNodes(source);
    return _nodes[source]->getComputingTime(flow_id);
}

void FocusInjectionKernel::updateFocusKernel(int source, int dest) {
    checkNodes(source);
    checkNodes(dest);
    _nodes[dest]->receiveFlow(source);
}

bool FocusInjectionKernel::allNodeClosed() {
    for (Node* node: _nodes) {
        if (!node->isClosed()) {
            return false;
        }
    }
    return true;
}

bool FocusInjectionKernel::sync_test(int source, int time) {
    checkNodes(source);
    return dynamic_cast<SyncNode*>(_nodes[source])->test(time);
}

int FocusInjectionKernel::sync_dest(int source, int time) {
    checkNodes(source);
    return dynamic_cast<SyncNode*>(_nodes[source])->getDestination();
}

int FocusInjectionKernel::sync_flowSize(int source, int time) {
    checkNodes(source);
    return dynamic_cast<SyncNode*>(_nodes[source])->getFlowSize();
}


}