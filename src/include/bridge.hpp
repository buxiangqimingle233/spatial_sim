#ifndef __PACKET_H__
#define __PACKET_H__

#include <memory>
#include <queue>
#include <vector>
#include <iostream>
#include <map>
#include <assert.h>

namespace spatial {

struct Tensor {
    std::vector<int> dims;
    int tid;

    Tensor(std::vector<int> dims_, int tid_): dims(dims_), tid(tid_) { };
    Tensor(): dims(std::vector<int>()), tid(-1) { };
    void explicitCopy(const Tensor& other) {
        tid = other.tid;
        dims = other.dims;
    }
    int size() {
        if (dims.empty()) {
            return -1;
        }
        int size = 1;
        for (int d: dims) {
            size *= d;
        }
        return size;
    }
};

struct Packet {
    int fid = -1;
    int size = -1;
    int dest = -1;
    int source = -1;
    Tensor data;
    std::queue<int> im_nodes;  // intermediate nodes

public:
    Packet(int fid_, int size_, int dest_, int source_, Tensor data_) : 
        fid(fid_), size(size_), dest(dest_), source(source_), data(data_) { };
    Packet() { };

    std::queue<int> getIntermediateNodes() {
        return im_nodes;
    }

    void setIntermediateNodes(const std::vector<int>& list) {
        for (int i: list) {
            im_nodes.push(i);
        }
        im_nodes.push(dest);
    }
};

}; 

typedef std::shared_ptr<std::queue<spatial::Packet>> CNInterface;
typedef std::shared_ptr<std::vector<CNInterface>> PCNInterfaceSet;

#endif