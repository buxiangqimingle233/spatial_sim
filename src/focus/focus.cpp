#include "focus.hpp"
#include <iostream>
#include <algorithm>
#include <math.h>

namespace focus 
{

std::shared_ptr<FocusInjectionKernel> FocusInjectionKernel::_kernel = NULL;

int Flow::forward(bool grant_arbit, bool grant_alloc) {
    FlowState next_state = FlowState::Closed;
    int issue = false;

    switch (_state) {
        case FlowState::Init: {
            next_state = FlowState::Waiting;
            issue = false;
            break;
        }
        case FlowState::Waiting: {
            // FIXME: > or >=
            if (_iter >= _max_iter) {
                next_state = FlowState::Closed;
                issue = false;
            } else {
                auto it = _received_packet.begin();
                for (; it != _received_packet.end(); ++it) {
                    // Exists an dependent flow unsatisfied
                    if (it->second < _depend_flows[it->first]) {
                        break;
                    }
                }
                if (it != _received_packet.end()) {
                    next_state = FlowState::Waiting;
                    issue = false;
                } else {
                    next_state = FlowState::Computing;
                    issue = false;
                    // Comsume the received packets
                    it = _received_packet.begin();
                    for (; it != _received_packet.end(); ++it) {
                        it->second -= _depend_flows[it->first];
                    }
                }
            }
            break;
        }
        // We're doing computing now, the flow is sleeping ...
        case FlowState::Computing: {
            // We should issue the flow at the first iteration without waiting
            if (_iter == 0) {
                _slept_time = 0;
                next_state = FlowState::Arbitrating;
                issue = false;
            } else if (_slept_time < _interval) {
                _slept_time++;
                next_state = FlowState::Computing;
                issue = false;
            } else {
                _slept_time = 0;
                next_state = FlowState::Arbitrating;
                issue = false;
            }
            break;
        }
        // Computation is done, waiting for arbitration flag
        case FlowState::Arbitrating: {
            if (grant_arbit && grant_alloc) {
                next_state = FlowState::Commiting;
                issue = true;
            } else if (grant_arbit && !grant_alloc) {
                next_state = FlowState::Arbitrating;
                issue = false;
            } else if (!grant_arbit) {
                next_state = FlowState::Arbitrating;
                issue = false;
            }
            break;
        }
        case FlowState::Commiting: {
            if (grant_alloc) {
                ++_iter;
                next_state = FlowState::Waiting;
                issue = false;
            } else {
                next_state = FlowState::Commiting;
                issue = false;
            }
            break;
        }
        // Dead here
        default: {
            next_state = FlowState::Closed;
            issue = false;
            break;
        }
    }

    _state = next_state;
    return issue;
}

void Flow::receiveDependentPacket(int flow_id) {
    if (_received_packet.find(flow_id) == _received_packet.end()) {
        throw "Receive the an independent packet " + flow_id;
    }
    _received_packet[flow_id] += 1;
}

bool Node::step(bool buffer_empty) {
    if (_out_flows.size() == 0) {
        return false;
    }

    int size_ = _out_flows.size();
    std::vector<int> ready_flows;
    for (int i = 0; i < size_; ++i) {
        if (_out_flows[i].canIssue()) {
            ready_flows.push_back(i);
        }
    }
    std::random_shuffle(ready_flows.begin(), ready_flows.end());

    if (!buffer_empty) {
        _flow_to_inject = _flow_to_inject;
    } else if (buffer_empty && !ready_flows.empty()) {
        _flow_to_inject = ready_flows[0];
    } else {
        _flow_to_inject = INVALID;
    }

    bool issue = false;
    for (int i = 0; i < size_; ++i) {
        issue |= _out_flows[i].forward(i == _flow_to_inject, buffer_empty);
    }

    return issue;
}

int Node::getDestination() {
    if (_flow_to_inject != INVALID) {
        return _out_flows[_flow_to_inject].getDestination();
    } else {
        return INVALID;
    }
}

int Node::getFlowSize() {
    if (_flow_to_inject != INVALID) {
        return _out_flows[_flow_to_inject].getFlowSize();
    } else {
        return INVALID;
    }
}

int Node::getFlowID() {
    if (_flow_to_inject != INVALID) {
        return _out_flows[_flow_to_inject].getFlowID();
    } else {
        return INVALID;
    }
}

int Node::getComputingTime(int flow_id) {
    if (_flow_to_inject != INVALID) {
        return _out_flows[flow_id].getComputingTime();
    } else {
        return INVALID;
    }
}

void Node::receivePacket(int flow_id) {
    // When the node receives a flow, it broadcast it to every operators
    for (auto& f : _out_flows) {
        try {
            f.receiveDependentPacket(flow_id);
        } catch (const char* msg) {
            // do nothing ... 
        }
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
    _buffer_empty.resize(nodes, true);
    
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

        int flow_size;
        ifs >> flow_size;
        std::vector<Flow> flows;
        for (int _ = 0; _ < flow_size; ++_) {
            int flow_id, ct, mi, dep, size, src, dst;
            ifs >> flow_id >> ct >> mi >> dep >> size >> dst >> src;

            std::map<int, double> depend_flows;

            for (int df = 0; df < dep; ++df) {
                int df_id;
                double df_cnt;
                ifs >> df_id >> df_cnt;
                depend_flows[df_id] = df_cnt;
            }

            flows.push_back(Flow(flow_id, ct, mi, depend_flows, size, src, dst));
        }

        Node* node = new Node(flows, node_id);
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

// Producer methods 
void FocusInjectionKernel::step() {
    int _size = _nodes.size();
    for (int i = 0; i < _size; ++i) {
        if (_nodes[i]->step(_buffer_empty[i])) {
            _buffer_empty[i] = false;
        }
    }
}

// Consumer methods
bool FocusInjectionKernel::test(int source) {
    checkNodes(source);
    if (!_buffer_empty[source]) {
        _buffer_empty[source] = true;
        return true;
    } else {
        return false;
    }
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

void FocusInjectionKernel::retirePacket(int source, int dest, int flow_id) {
    checkNodes(source);
    checkNodes(dest);
    _nodes[dest]->receivePacket(flow_id);
}

bool FocusInjectionKernel::allNodesClosed(int cycle) {
    // FIXME: Instead of waiting for all nodes to close, we ternimate the simulation 
    // when 95% nodes finish their tasks. In this way, 
    // we mitigate long tail executing nodes and greatly accelerate the simulation. 
    float close_ratio = closeRatio(cycle);
    return close_ratio >= 1;
}

void FocusInjectionKernel::dump(int cycle, std::ofstream& out) {
    out << "cycle " << cycle << std::endl;
    
    for (Node* node: _nodes) {
        node->dump(out);
    }
    out << std::endl;
    out.flush();
}

float FocusInjectionKernel::closeRatio(int cycle) {
    int cnt = 0;
    for (Node* node: _nodes) {
        if (node->isClosed()) {
            ++cnt;
        }
    }
    return (float) cnt / _nodes.size();
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