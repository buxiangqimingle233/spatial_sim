#include <string>
#include <fstream>
#include "buffer.h"

namespace spatial {

BUFFER::BUFFER(const std::map<std::string, float> mi_): COMPONENT("BUFFER", mi_) {

}

int BUFFER::simple_sim(const std::string micro_instr, std::map<int, Tensor>& data, int clock_) {
    std::vector<std::string> inst_fields = split(micro_instr, " ");
    std::string inst_type = inst_fields[0];
    std::map <std::string, float>::iterator iter = _micro_instr_latency.find(inst_type);
    if (iter == _micro_instr_latency.end()) {
        instruction_mismatch(micro_instr);
    } else {
        int size = data[std::stoi(inst_fields[1])].size();
        return clock_ + size / iter->second;
    }
}

}