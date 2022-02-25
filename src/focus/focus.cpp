#include "focus.hpp"
#include <iostream>
#include <algorithm>
#include <math.h>

namespace focus 
{

std::shared_ptr<FocusInjectionKernel> FocusInjectionKernel::_kernel = NULL;

void Flow::forward(bool grant_arbit) {
    FlowState next_state = FlowState::Closed;

    switch (_state) {

        case FlowState::Init: {
            next_state = FlowState::Waiting;
            break;
        }

        case FlowState::Waiting: {
            if (_iter >= _max_iter) {
                next_state = FlowState::Closed;
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
                } else {
                    next_state = FlowState::Computing;
                    // Comsume the received packets
                    it = _received_packet.begin();
                    for (; it != _received_packet.end(); ++it) {
                        it->second -= _depend_flows[it->first];
                    }
                }
            }
            break;
        }

        // It's computing now, the flow is sleeping ...
        case FlowState::Computing: {
            // We should issue the flow at the first iteration without waiting
            if (_iter == 0) {
                _slept_time = 0;
                next_state = FlowState::Arbitrating;
            } else if (_slept_time < _interval) {
                _slept_time++;
                next_state = FlowState::Computing;
            } else {
                _slept_time = 0;
                next_state = FlowState::Arbitrating;
            }
            break;
        }

        // Computation is done, waiting for arbitration flag
        case FlowState::Arbitrating: {
            next_state = grant_arbit ? FlowState::Waiting : FlowState::Arbitrating;
            _iter += grant_arbit ? 1 : 0;
            break;
        }

        // Dead here
        default: {
            next_state = FlowState::Closed;
            break;
        }
    }

    _state = next_state;
}

void Flow::receiveDependentPacket(int flow_id) {
    if (_received_packet.find(flow_id) == _received_packet.end()) {
        throw "Receive the an independent packet " + flow_id;
    }
    _received_packet[flow_id] += 1;
}

void Node::step(std::queue<PktHeader>& send_queue) {
    if (_out_flows.size() == 0) {
        return;
    }

    int size_ = _out_flows.size();
    std::vector<int> ready_flows;
    for (int i = 0; i < size_; ++i) {
        if (_out_flows[i].canIssue()) {
            ready_flows.push_back(i);
        }
    }

    int flow_to_inject = INVALID;
    if (send_queue.empty() && !ready_flows.empty()) {
        std::random_shuffle(ready_flows.begin(), ready_flows.end());
        flow_to_inject = ready_flows[0];
        send_queue.push(_out_flows[flow_to_inject].genPacketHeader());
    }

    for (int i = 0; i < size_; ++i) {
        _out_flows[i].forward(i == flow_to_inject);
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

void Node::dump(std::ofstream& ofs) {
    if (_out_flows.empty()) {
            return;
    }
    ofs << "node " << _node_id << ": " << std::endl;
    for (auto f : _out_flows) { 
        ofs << "fid: " << f._flow_id << ", state: " << f._state << ", comp-slept: " \
            << f._computing_time << "-" << f._slept_time << ", iter-max_iter: " \
            << f._iter << "-" << f._max_iter;
        ofs << ", received packets: ";
        for (auto p: f._received_packet) {
            ofs << p.first << "-" << p.second << " ";
        }
        ofs << std::endl;
    }
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
    _send_queues.resize(nodes, std::queue<PktHeader>());
    
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
    int node_cnt = _nodes.size();
    for (int i = 0; i < node_cnt; ++i) {
        _nodes[i]->step(_send_queues[i]);
    }
}

// Consumer methods
bool FocusInjectionKernel::test(int source) {
    checkNodes(source);
    return !_send_queues[source].empty();
}

int FocusInjectionKernel::dest(int source) {
    checkNodes(source);
    return _send_queues[source].front().dst;
}

int FocusInjectionKernel::flowSize(int source) {
    checkNodes(source);
    return _send_queues[source].front().size;
}

int FocusInjectionKernel::flowID(int source) {
    checkNodes(source);
    return _send_queues[source].front().flow_id;
}

void FocusInjectionKernel::dequeue(int source) {
    checkNodes(source);
    _send_queues[source].pop();
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