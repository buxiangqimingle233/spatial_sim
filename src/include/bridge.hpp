#ifndef __PACKET_H__
#define __PACKET_H__

#include <memory>
#include <queue>
#include <vector>
#include <iostream>

namespace spatial {

struct Tensor {
    std::vector<int> dims;
    std::string type;
    int tid;

    Tensor(std::vector<int> dims_, std::string type_, int tid_): dims(dims_), type(type_), tid(tid_) { };
    Tensor(): dims(std::vector<int>()), type(""), tid(-1) { };
};

struct Packet {
    int type = -1;
    int fid = -1;
    int size = -1;
    int dest = -1;
    int source = -1;
    Tensor data;

public:
    Packet(int type_, int fid_, int size_, int dest_, int source_, Tensor data_) : 
        type(type_), fid(fid_), size(size_), dest(dest_), source(source_), data(data_) { };
    Packet() { };
};

}; 

typedef std::shared_ptr<std::queue<spatial::Packet>> CNInterface;
typedef std::shared_ptr<std::vector<CNInterface>> PCNInterfaceSet;

#endif