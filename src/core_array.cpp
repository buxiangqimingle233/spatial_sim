#include "core_array.hpp"
#include "bridge.hpp"
#include "core.h"
#include "custom_configs.hpp"

namespace spatial {


CoreArray::CoreArray(Configuration config, PCNInterfaceSet send_queues_, PCNInterfaceSet receive_queues_, \
    std::shared_ptr<std::vector<bool>> open_pipes): _config(config), _pipe_open(open_pipes)
{
    int size = config.GetInt("array_size");
    std::vector<std::string> inst_file_names = config.GetStrArray("inst_file_names");
    std::vector<int> assigned_cores = config.GetIntArray("assigned_cores");
    std::string inst_dir = config.GetStr("inst_dir");
    if (assigned_cores.size() < size) {
        throw "Please specify an instruction file to every core !";
    }

    for (int i = 0; i < size; ++i) {
        int core = assigned_cores[i];
        if (core >= inst_file_names.size()) {
            throw "The instruction file of node " + std::to_string(i) + " is not specified !!";
        }
        std::string inst_file = inst_dir + "/" + inst_file_names[core];
        std::shared_ptr<CNInterface> sq(&(*send_queues_)[i]), rq(&(*receive_queues_)[i]);
        _cores.push_back(CORE(inst_file, sq, rq, open_pipes));
    }
}



std::shared_ptr<CoreArray> CoreArray::New(std::string spec_file, PCNInterfaceSet sqs, PCNInterfaceSet rqs, \
    std::shared_ptr<std::vector<bool>> open_pipe) 
{
    CoreArrayConfig config;
    // TODO: Replace this with ParseArgs
    config.ParseFile(spec_file);
    return std::make_shared<CoreArray>(config, sqs, rqs, open_pipe);
}

void CoreArray::step(int clock) {
    for (CORE& core : _cores) {
        core.ClockTick();
    }

    int threshold = _config.GetInt("threshold");
    int size = _cores.size();
    for (int i = 0; i < size; ++i) {
        (*_pipe_open)[i] = (_cores[i].rq)->size() > threshold;
    }
}

}