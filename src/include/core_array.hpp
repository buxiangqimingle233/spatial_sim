#ifndef _CORE_WRAPPER_H__
#define _CORE_WRAPPER_H__

#include "spatial_chip.hpp"
#include "core.h"
#include <queue>

namespace spatial {

class CoreArray {

public:
    void init();
    void step(CNInterface& sq, CNInterface& rq, clock_t& clock);
    void sweep();
    void printStats();

};


};


#endif