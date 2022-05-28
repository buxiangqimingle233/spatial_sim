#include "core_array.hpp"
#include "bridge.hpp"
#include "math.h"
#include "core.h"
#include "spatial_config.hpp"

namespace spatial {

int array_size;

CoreArray::CoreArray(Configuration config, PCNInterfaceSet send_queues_, PCNInterfaceSet receive_queues_, \
    std::shared_ptr<std::vector<bool>> open_pipes): _config(config), _pipe_open(open_pipes)
{
    // int size = config.GetInt("array_size");
    int k = config.GetInt("k");
    int n = config.GetInt("n");
    array_size = (int)std::pow(k, n);

    std::vector<std::string> inst_file_names = config.GetStrArray("tasks");
    std::string working_dir = config.GetStr("working_directory");
    std::string latency_file = config.GetStr("micro_instr_latency");

    int width = config.GetInt("channel_width");
    int threshold = _config.GetInt("threshold");

    for (int i = 0; i < array_size; ++i) {
        int core = i;
        if (core >= inst_file_names.size()) {
            throw "The instruction file of node " + std::to_string(i) + " is not specified !!";
        }
        std::string inst_file = working_dir + "/" + inst_file_names[core];
        std::string routing_board_file = config.GetStr("routing_board");
        // routing_board_file = working_dir + "/" + routing_board_file;
        _cores.push_back(CORE(inst_file, latency_file, routing_board_file, core, (*send_queues_)[i], (*receive_queues_)[i], open_pipes, threshold, width));
    }
}


void CoreArray::step(int clock) {
    for (CORE& core : _cores) {
        core.ClockTick(clock);
    }
}

bool CoreArray::stateChanged() {
    
    static std::vector<CORE> cores_backup = std::vector<CORE>();
    bool change = false;
    if (cores_backup.size() != _cores.size()) {
        change = true;
    } else {
        int size = _cores.size();
        for (int i = 0; i < size; ++i) {
            if (!_cores[i].EqualTo(cores_backup[i])) {
                change = true;
                break;
            }
        }
    }

    cores_backup = _cores;
    return change;
}

bool CoreArray::allCoreClosed(int _clock) {
    bool core_closed = true;
    for (auto& c: _cores) {
        core_closed &= c.AllInstFinished(_clock);
    }
    return core_closed;
}

void CoreArray::DisplayStats(std::ostream& os) {
    for (const CORE& c : _cores) {
        c.DisplayStats(os);
    }
}

}