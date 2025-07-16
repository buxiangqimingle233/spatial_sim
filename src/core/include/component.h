#ifndef _COMPONENT_H
#define _COMPONENT_H

#include <string>
#include "common.h"
#include "bridge.hpp"

namespace spatial {

struct CompInstr;

class COMPONENT {
public:
    COMPONENT(const std::string name_, const std::map<std::string, float> mi_);
    const std::string name;

    void instruction_mismatch(const std::string& micro_instr);
    virtual int simple_sim(const std::string micro_instr, std::map<int, Tensor>& data, int clock_);
    virtual void DisplayStats(ostream & os = cout );

protected:
    std::map<std::string, float> _micro_instr_latency;
};

};

#endif