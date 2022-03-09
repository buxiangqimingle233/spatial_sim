#ifndef __PACKET_H__
#define __PACKET_H__

#include <memory>
#include <queue>
#include <vector>
#include "flit.hpp"

namespace spatial {

struct Packet {
    int type = -1;
    int fid = -1;
    int size = -1;
    int dest = -1;
    int source = -1;
    void* data = NULL;

public:
    Packet(int type_, int fid_, int size_, int dest_, int source_,void* data_) : 
        type(type_), fid(fid_), size(size_), dest(dest_), source(source_), data(data_) { };
    Packet(const Flit* flit) { 
        // How to recover the packet from its tail flit.
        std::cerr << "Please finish this construction method" << std::endl;
    };
    Packet() { };
};

};

typedef std::queue<spatial::Packet> CNInterface;
typedef std::shared_ptr<std::vector<CNInterface>> PCNInterfaceSet;

#endif