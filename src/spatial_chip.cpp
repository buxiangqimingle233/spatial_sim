#include <string>
#include <fstream>
#include "math.h"
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
    config.checkConsistency();
    _config = config;

    // Initialize the interface queues between cores and nocs
    int k = config.GetInt("k");
    int n = config.GetInt("n");
    array_size = (int)std::pow(k, n);

    _send_queues = std::make_shared<std::vector<CNInterface> >();
    _received_queues = std::make_shared<std::vector<CNInterface> >();
    _credit_board = std::make_shared<std::vector<bool> >();

    typedef std::queue<spatial::Packet> T;
    for (int i = 0; i < array_size; ++i) {
        _send_queues->push_back(std::make_shared<T>(T()));
        _received_queues->push_back(std::make_shared<T>(T()));
    }
    _credit_board->resize(array_size, true);

    std::streambuf *backup = std::cout.rdbuf();
    // redirect the standard output stream into log file
    if (config.GetStr("log_file") != "-") {
        // Check log file
        _log_file = std::ofstream(config.GetStr("log_file"), std::ios::out);
        if (!_log_file) {
            throw "Log file doesn't exist !! ";
        }
        std::cout.rdbuf(_log_file.rdbuf());
    }

    try {
        // Instantiate NoC
        noc = std::make_shared<NoC>(config, _send_queues, _received_queues);

        // Instantiate Core Array
        core_array = std::make_shared<CoreArray>(config, _send_queues, _received_queues, _credit_board);
    } catch (char const* msg) {
        std::cerr << msg << std::endl;
        exit(-1);
    }

    // restore cout's original streambuf
    std::cout.rdbuf(backup);

    // setup clock
    _clock = 0;
}


void SpatialChip::reset() {
    // reset clock
    _clock = 0;
    // reset queue pairs
    // reset noc & core
}


void SpatialChip::run() {
    reset();
    int check_frequency = _config.GetInt("deadlock_check_freq");

    while (!task_finished()) {
        core_array->step(_clock);
        noc->step(_clock);
        _clock++;

        if (_clock % check_frequency  == 0) {
            std::cout << "Simulate " << _clock << " cycles" << std::endl;
            if (check_deadlock()) {
                std::cerr << "Deadlock detected: the chip state keeps unchanged over " << check_frequency << " cycles" << std::endl;
                display_stats();
                exit(1); 
            }
#ifdef DUMP_NODE_STATE
            std::ofstream out("dump.log", std::ios::out|std::ios::app);
            out.close();
#endif
        }

    }
    std::cout << _clock << " | " << "Task Is Finished " << std::endl;
}

bool SpatialChip::task_finished() {
    return noc->traffic_drained() && core_array->allCoreClosed();
}

bool SpatialChip::check_deadlock() {

    static std::pair<PCNInterfaceSet, PCNInterfaceSet> queue_pair_backup = std::make_pair(nullptr, nullptr);
    bool deadlock = true;

    if (queue_pair_backup.first == nullptr) {
        deadlock = false;
    } else {
        deadlock &= !core_array->stateChanged();
        deadlock &= (*(queue_pair_backup.first) == *_send_queues) && (*(queue_pair_backup.second) == *_received_queues);
    }

    std::vector<CNInterface> sq_bu = *_send_queues, rq_bu = *_received_queues;
    queue_pair_backup.first = std::make_shared<vector<CNInterface> >(sq_bu);
    queue_pair_backup.second = std::make_shared<vector<CNInterface> >(rq_bu);

    return deadlock;
}


void SpatialChip::display_stats(std::ostream & os) {

    os << std::endl << " ================== Dumped Stats ================== " << std::endl;
    core_array->DisplayStats(os);
    noc->DisplayStats(os);

    os << std::endl << " ================================================= " << std::endl;
}

};