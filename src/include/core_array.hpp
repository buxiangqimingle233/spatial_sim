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
    std::shared_ptr<std::vector<bool>> _pipe_open;
    Configuration _config;

public:
    CoreArray(Configuration config, PCNInterfaceSet send_queues_, PCNInterfaceSet receive_queues_, \
        std::shared_ptr<std::vector<bool>> open_pipe);
    ~CoreArray() {}

    void step(int clock);
    void printStats();
    
    bool allCoreClosed() {
        bool core_closed = true;
        for (auto& c: _cores) {
            core_closed &= c.closed();
        }
        return core_closed;
    }

    bool stateChanged();

};


};


#endif