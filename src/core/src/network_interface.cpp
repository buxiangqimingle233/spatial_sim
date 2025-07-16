#include <string>
#include <sstream>
#include <assert.h>
#include <queue>
#include <math.h>
#include "core.h"
#include "network_interface.h"
#include "parser.h"

namespace spatial {

std::shared_ptr<std::map<int, MCTree> > routing_board = nullptr;

NI::NI(
    CNInterface sq_, CNInterface rq_, std::shared_ptr<std::vector<bool> > pipe_open_, 
    std::map<std::string, float> mi_, int cid_, const std::string& rb_file, 
    int threshold_, int width_ = 128
): _send_queue(sq_), _receive_queue(rq_), _pipe_open(pipe_open_), width(width_), cid(cid_),
   threshold(threshold_), COMPONENT("NI", mi_) 
{
    // NIs share a global routing board
    if (routing_board == nullptr) {
        routing_board = std::make_shared<std::map<int, MCTree>>();
        PathParser::parsePathFile(rb_file, *routing_board);
    }
};

int NI::simple_sim(const std::string micro_instr, std::map<int, Tensor>& data, int clock_) 
{
    std::stringstream ss(micro_instr);
    std::string type;
    ss >> type;

    (*_pipe_open)[cid] = _receive_queue->size() <= threshold;

    // e.g. Ni.send dest_nid data_ptr
    if (type == "NI.send") {
        int tid = -1;
        ss >> tid;
        std::vector<int> dests;
        int d;
        while (ss >> d) {
            if (cid != d) {
                dests.push_back(d);
            } else {
                std::cerr << "WARNING: " << "core " << cid << " sends a packet"
                          << " to itself. We ignore this packet, " \
                          << "but it may cause deadlocks." << std::endl;
            }
        }
        if (dests.size() == 0) {
            return clock_ + 1;
        }
        assert(dests.size() && tid != -1);
        assert(data.count(tid) != 0);

        Tensor tensor = data[tid];

#ifdef MULTICAST
        Packet p = GeneratePacket(tensor, dests, cid);
        return _send_package(p);
#else
        // TODO: It's ugly, refactor this
        std::vector<Packet> unicast_packets; 
        for (int dest: dests) {
            Packet p = GeneratePacket(tensor, vector<int>({dest}), cid);
            unicast_packets.push_back(p);
        }
        std::set<int> s(dests.begin(), dests.end());
        if (_doorbell(s)) {
            for (Packet& p: unicast_packets) {
                assert(_send_package(p));
            }
            return clock_ + 1;
        } else {
            return -1;
        }
#endif

    } else if (type == "NI.recv") {
        int fid = -1;
        ss >> fid;
        assert(fid >= 0);
        if (_receive_package(fid, data[fid])) {
            return clock_ + 1;
        } else {
            return -1;
        }
    }
    instruction_mismatch(micro_instr);
}

bool NI::_doorbell(const std::set<int>& dests) {
    bool dests_all_free = true;
    for (int d: dests) {
        assert(d >= 0 && d < array_size);
        dests_all_free &= (*_pipe_open)[d];
    }
    // bool src_channel_available = _send_queue->size() < threshold;
    bool src_channel_available = true;      // TODO: this may cause deadlock
    return dests_all_free && src_channel_available;
}


bool NI::_send_package(const Packet& package) {
    assert(package.size > 0);
    set<int> dests = package.path->getDestNodes();

    if (_doorbell(dests)) {
        std::cout << "CORE | NI enqueues package" << std::endl;
        _send_queue->push(package);
        return true;
    } else {
        std::cout << "CORE | Destinations of this package is unavailable, pend package sending" << std::endl;
        return false;
    }
}


bool NI::_receive_package(int fid, Tensor& buffer) {
    // Find the package we need in the receive queue.
    CNInterface inter_buf = std::make_shared<std::queue<Packet> >();
    bool found = false;
    while (!_receive_queue->empty()) {
        Packet p = _receive_queue->front();
        _receive_queue->pop();
        if (p.fid == fid) {
            buffer.copyFrom(p.data);
            found = true;
            break;
        }
        inter_buf->push(p);
    }

    while (!inter_buf->empty()) {
        _receive_queue->push(inter_buf->front());
        inter_buf->pop();
    }

    return found;
}


Packet NI::GeneratePacket(const Tensor& tensor, const std::vector<int>& dests, int src) {

    Packet p;
    p.fid = tensor.tid;

    assert(dests.size() > 0);
    if (dests.size() == 1) {
        p.type = Packet::TransferType::_UNICAST;
    } else {
        p.type = Packet::TransferType::_MULTICAST;
    }

    assert(src >= 0 && src < array_size);
    auto iter = routing_board->find(p.fid);
    if (iter != routing_board->end()) {
#ifdef MULTICAST
        p.path = std::make_shared<MCTree>(iter->second);
        assert(p.path->getDestNodes() == std::set<int>(dests.begin(), dests.end()));
#else
        std::cout << "WARNING | " << "We ignore the broadcast tree because only unicast packets are permitted. " << std::endl;
        assert(dests.size() == 1);
        p.path = std::make_shared<MCTree>(cid);
        p.path->addSegment(cid, dests.front(), nullptr, true);
#endif 
    } else {
        if (dests.size() > 1) {
            std::cerr << "ERROR | " << "Please specify the multicast tree for tensor " << tensor.tid << std::endl;
            assert(false);
        }
        p.path = std::make_shared<MCTree>(src);
        for (int dest: dests) {
            p.path->addSegment(src, dest, nullptr, true);
        }
    }

    p.data = tensor;
    p.size = std::ceil(p.data.size() / (float)width);

    // TODO: The tensor is empty indicates that it's not correctly calculated by ACC or CPU. 
    // But to keep simulation, we just print a warning and pad the tensor with one flit.
    if (p.data.size() == 0) {
        p.size = 1;
        p.data.dims.push_back(1);
        std::cerr << "WARNING: The network interface want to send the empty tensor " 
                  << p.fid << ". ";
        std::cerr << "We pad it with one flit to keep on simulating." << std::endl;
    }
    
    return p;
}


std::pair<CNInterface, CNInterface> NI::GetQueuePairs() {
    return std::make_pair(_send_queue, _receive_queue);
}

void NI::DisplayStats(std::ostream& os) {
    os << "Network Interface: ";
    os << "send queue size: " << _send_queue->size() << " receive queue size: " << _receive_queue->size();
    os << std::endl;
}

}