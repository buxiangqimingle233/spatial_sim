#include <string>
#include <fstream>
#include "config_utils.hpp"
#include "noc.hpp"
#include "core_array.hpp"
#include "spatial_chip.hpp"
#include "spatial_sim_config.hpp"

namespace spatial {

SpatialChip::SpatialChip(std::string spatial_chip_spec) {
    SpatialSimConfig config;

    // TODO: modify this to ArgParse() with outputs;
    config.ParseFile(spatial_chip_spec);

    _config = config;
    _log_file = std::ofstream(config.GetStr("log_file"), std::ios::out);
    if (!_log_file) {
        throw "Log file doesn't exist !! ";
    }
    
    int core_array_size = config.GetInt("array_size"), network_size = config.GetInt("network_size");
    int queue_size = core_array_size > network_size ? core_array_size : network_size;

    // FIXME: test the initialization
    _send_queues = std::make_shared<std::vector<CNInterface>>();
    _received_queues = std::make_shared<std::vector<CNInterface>>();
    _send_queues->resize(queue_size);
    _received_queues->resize(queue_size);

    // Instantiate Core Array
    std::string core_array_spec = config.GetStr("core_array_spec");
    // TODO: do sth
    // core = 

    // Instantiate NoC
    std::string noc_spec = config.GetStr("noc_spec");
    std::string dummy = "spatialsim";
    char* argv[2] = {const_cast<char *>(dummy.c_str()), const_cast<char *>(noc_spec.c_str())};
    int argc = 2;
    noc = NoC::New(argc, argv, _send_queues, _received_queues);

    // setup clock
    _clock = 0;
}


void SpatialChip::reset() {
    // reset clock
    // reset queue pairs
    // reset noc & core
}


void SpatialChip::run() {

    reset();
    (*_send_queues)[0].push(
        Packet(1, 1, 3, 1, 0, nullptr)
    );
    (*_send_queues)[0].push(
        Packet(1, 2, 2, 2, 0, nullptr)
    );

    for (; _clock < 10000; ++_clock) {
        noc->step(_clock);
    }

}

};