#ifndef _NI_H
#define _NI_H

#include <queue>
#include "component.h"
#include "bridge.hpp"
#include "path.hpp"
#include <memory>

namespace spatial {

class NI : public COMPONENT {
using COMPONENT::COMPONENT;

protected:
    int width;
    int cid;
    int threshold;
    bool _doorbell(const std::set<int>& dests);
    bool _send_package(const Packet&);
    bool _receive_package(int, Tensor&);

public:
    virtual int simple_sim(const std::string micro_instr, std::map<int, Tensor>& data, int clock_) override;
    virtual void DisplayStats(std::ostream & os = cout) override;

    Packet GeneratePacket(const Tensor& tensor, const std::vector<int>& dests, int src);
    std::pair<CNInterface, CNInterface> GetQueuePairs();

    NI(CNInterface sq_, CNInterface rq_, std::shared_ptr<std::vector<bool> > pipe_open_, 
       std::map<std::string, float> mi_, int cid_, const std::string& rb_file, 
       int threshold_, int width_);
    ~NI() { }

protected:
    CNInterface _send_queue, _receive_queue;
    std::shared_ptr<std::vector<bool> > _pipe_open;
};

extern std::shared_ptr<std::map<int, MCTree> > routing_board;

}

#endif