#ifndef __PACKET_H__
#define __PACKET_H__

#include <memory>
#include <queue>
#include <vector>

namespace spatial {

struct Packet {
    int type = -1;
    int fid = -1;
    int size = -1;
    int dest = -1;
    int source = -1;
    std::shared_ptr<char[]> data = nullptr;

public:
    Packet(int type_, int fid_, int size_, int dest_, int source_, std::shared_ptr<char[]> data_) : 
        type(type_), fid(fid_), size(size_), dest(dest_), source(source_), data(data_) { };
    Packet() { };
};

};

typedef std::queue<spatial::Packet> CNInterface;
typedef std::shared_ptr<std::vector<CNInterface>> CNInterfaceSet;

#endif