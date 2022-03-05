#ifndef _CORE_WRAPPER_H__
#define _CORE_WRAPPER_H__

#include <queue>
#include <memory>
#include <map>
#include <string>

#include "config_utils.hpp"
#include "core.h"

namespace spatial {

class CoreArray {

private:
    std::vector<CORE> _cores;
    Configuration _config;

public:
    CoreArray(Configuration config, CNInterfaceSet send_queues_, CNInterfaceSet receive_queues_);

    static std::shared_ptr<CoreArray> New(std::string spec_file, CNInterfaceSet sqs, CNInterfaceSet rqs);
    void step(int clock);
    void printStats();

};


};


#endif