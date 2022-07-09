#ifndef __SPATIAL_CHIP_H__
#define __SPATIAL_CHIP_H__

#include <queue>
#include <vector>
#include <fstream>
#include <string>
#include <memory>
#include "config_utils.hpp"
#include "bridge.hpp"
#include "noc.hpp"
#include "core_array.hpp"


namespace spatial {

class NoC;
class CoreArray;

// The top module of our simulator
class SpatialChip {

private:
    unsigned int _clock;
    Configuration _config;

    PCNInterfaceSet _send_queues;      // the interface between core and noc: send packets
    PCNInterfaceSet _received_queues;  // the interface between core and noc: receive packets
    std::shared_ptr<std::vector<bool> > _credit_board;

    std::ostream* _log_file;

    // Two hardware components, interacting with each other only via the queeue pair
    std::shared_ptr<NoC> noc;
    std::shared_ptr<CoreArray> core_array;

public:
    void reset();
    unsigned int run();
    bool task_finished(int _clock);
    bool check_deadlock();
    void display_stats(std::ostream& os = std::cout);
    std::vector<int> compute_cycles();
    std::vector<int> communicate_cycles();
    std::vector<double> router_conflict_factors();

    SpatialChip(std::string spatial_chip_spec);
    ~SpatialChip() { }
};

};



#endif