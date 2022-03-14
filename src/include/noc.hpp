#ifndef __NOC_WRAPPER_H__
#define __NOC_WRAPPER_H__

#include "spatial_chip.hpp"
#include "trafficmanager.hpp"
#include "booksim_config.hpp"
#include <queue>
#include <memory>

namespace spatial {

class NoC {

private:
    TrafficManager* _traffic_manager = NULL;
    BookSimConfig _config;

public:
    void step(clock_t clock);
    void printStats();

public:
    NoC(BookSimConfig config, PCNInterfaceSet send_queues_, PCNInterfaceSet receive_queues_);
    ~NoC() { delete _traffic_manager; };
};


};

#endif  