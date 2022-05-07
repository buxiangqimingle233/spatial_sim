#ifndef __PACKET_H__
#define __PACKET_H__

#include <memory>
#include <queue>
#include <vector>
#include <iostream>
#include <map>
#include <assert.h>

#include "path.hpp"

namespace spatial {

struct Tensor {
    std::vector<int> dims;
    int tid;

    Tensor(std::vector<int> dims_, int tid_): dims(dims_), tid(tid_) { };
    Tensor(): dims(std::vector<int>()), tid(-1) { };
    void copyFrom(const Tensor& other) {
        tid = other.tid;
        dims = other.dims;
    }
    int size() {
        if (dims.empty()) {
            return 0;
        }
        int size = 1;
        for (int d: dims) {
            size *= d;
        }
        return size;
    }
};

struct Packet {
    enum TransferType { _UNICAST, _MULTICAST, _REDUCE } type;
    int fid;
    int size;
    shared_ptr<MCTree> path;
    Tensor data;

public:

    // Packet(int fid_, TransferType type_, int size_, Tensor data_, shared_ptr<MCTree> path_)
    //     : fid(fid_), type(type_), size(size_), data(data_), path(path_) { };
    Packet(): type(TransferType::_UNICAST), fid(-1), size(-1), path(nullptr), data() { };
    void dumpStat(std::ostream & os) {
        os << "pkt " << fid << ": size-" << size << " tensor-" << data.tid << " to-";
        for (auto i = path->_leafs.begin(); i != path->_leafs.end(); ++i) {
            os << *i << ',';
        }
    }
};

}; 

typedef std::shared_ptr<std::queue<spatial::Packet>> CNInterface;
typedef std::shared_ptr<std::vector<CNInterface>> PCNInterfaceSet;

#endif