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
    static std::shared_ptr<NoC> New(int argc, char** argv, \
        std::shared_ptr<std::vector<CNInterface>> sq, std::shared_ptr<std::vector<CNInterface>> rq);

    void step(clock_t clock);
    void printStats();

public:
    NoC(TrafficManager* traffic_manager_, BookSimConfig config_): _traffic_manager(traffic_manager_), _config(config_) { };
    ~NoC() { delete _traffic_manager; };
};


};

#endif