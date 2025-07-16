#include "component.h"

namespace spatial {

COMPONENT::COMPONENT(const std::string name_, const std::map<std::string, float> mi_): name(name_), _micro_instr_latency(mi_) { }

int COMPONENT::simple_sim(const std::string micro_instr, std::map<int, Tensor>& data, int clock_) {
    std::map <std::string, float>::iterator iter = _micro_instr_latency.find(micro_instr);
    if (iter == _micro_instr_latency.end()) {
        instruction_mismatch(micro_instr);
    } else {
        return clock_ + iter->second;
    }
}

void COMPONENT::instruction_mismatch(const std::string& micro_instr) {
    std::cerr << "ERROR | Module " << name << " can not execute the instruction " << micro_instr << std::endl;
    exit(0);
}

void COMPONENT::DisplayStats(std::ostream & os) { 
    // TODO:
}

};