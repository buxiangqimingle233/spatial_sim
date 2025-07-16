#ifndef _BUS_H
#define _BUS_H

#include "component.h"

namespace spatial {

class BUS : public COMPONENT {
public:
    using COMPONENT::COMPONENT;
    BUS(const std::map<std::string, float> mi_);
    int simple_sim(const std::string micro_instr, std::map<int, Tensor>& data, int clock_) override;

};

}

#endif