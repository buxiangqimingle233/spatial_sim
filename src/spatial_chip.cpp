#include <string>
#include <fstream>
#include "config_utils.hpp"
#include "noc.hpp"
#include "core_array.hpp"
#include "spatial_chip.hpp"
#include "spatial_config.hpp"

namespace spatial {

SpatialChip::SpatialChip(std::string spatial_chip_spec) {

    SpatialSimConfig config;

    // Parse config file
    std::string dummy = "spatialsim";
    char* argv[2] = {const_cast<char *>(dummy.c_str()), const_cast<char *>(spatial_chip_spec.c_str())};
    int argc = 2;
    if ( !ParseArgs( &config, argc, argv ) ) {
        cerr << "Usage: " << argv[0] << " configfile... [param=value...]" << endl;
        exit(0);
    } 
    _config = config;

    // Check log file
    _log_file = std::ofstream(config.GetStr("log_file"), std::ios::out);
    if (!_log_file) {
        throw "Log file doesn't exist !! ";
    }
    
    // Initialize the interface queues between cores and nocs
    int array_size = config.GetInt("array_size");
    
    _send_queues = std::make_shared<std::vector<CNInterface> >();
    _received_queues = std::make_shared<std::vector<CNInterface> >();
    _credit_board = std::make_shared<std::vector<bool> >();

    typedef std::queue<spatial::Packet> T;
    for (int i = 0; i < array_size; ++i) {
        _send_queues->push_back(std::make_shared<T>(T()));
        _received_queues->push_back(std::make_shared<T>(T()));
    }
    _credit_board->resize(array_size, true);

    // Instantiate NoC
    noc = std::make_shared<NoC>(config, _send_queues, _received_queues);

    // Instantiate Core Array
    core_array = std::make_shared<CoreArray>(config, _send_queues, _received_queues, _credit_board);

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
    // (*_send_queues)[0]->push(
    //     Packet(1, 1, 3, 1, 0, nullptr)
    // );
    // (*_send_queues)[0]->push(
    //     Packet(1, 2, 2, 2, 0, nullptr)
    // );

    for (; _clock < 1000; ++_clock) {
        core_array->step(_clock);
        noc->step(_clock);
    }

}

};