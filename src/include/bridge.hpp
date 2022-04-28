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
    enum TransferType { UNICAST, MULTICAST, REDUCE } type;
    int fid;
    int size;
    shared_ptr<MCTree> path;
    Tensor data;

public:

    // Packet(int fid_, TransferType type_, int size_, Tensor data_, shared_ptr<MCTree> path_)
    //     : fid(fid_), type(type_), size(size_), data(data_), path(path_) { };
    Packet(): type(TransferType::UNICAST), fid(-1), size(-1), path(nullptr), data() { };
};

}; 

typedef std::shared_ptr<std::queue<spatial::Packet>> CNInterface;
typedef std::shared_ptr<std::vector<CNInterface>> PCNInterfaceSet;

#endif