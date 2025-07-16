#include <functional>
#include <string>
#include <numeric>
#include <vector>
#include "riscv_cpu.h"

namespace spatial {

RISCV_CPU::RISCV_CPU(const std::map<std::string, float> mi_): COMPONENT("CPU", mi_) {

}

int RISCV_CPU::simple_sim(const std::string micro_instr, std::map<int, Tensor>& data, int clock_) {
    std::vector<std::string> inst_fields = split(micro_instr, " ");
    std::string inst_type = inst_fields[0];
    std::map <std::string, float>::iterator iter = _micro_instr_latency.find(inst_type);
    if (iter == _micro_instr_latency.end()) {
        instruction_mismatch(micro_instr);
    } else if (inst_type == "CPU.reshape") {
        int out_tid = stoi(inst_fields[1]);
        assert(data.find(out_tid) != data.end());
        vector<int> dims;
        for (int i = 2; i < inst_fields.size(); ++i) {
            dims.push_back(stoi(inst_fields[i]));
        }
        _reshape(data[out_tid], dims);
        return clock_ + iter->second;
    } else if (inst_type == "CPU.poll") {
        return clock_ + iter->second;
    } else if (inst_type == "CPU.sleep") {
        int cycles = std::stoi(inst_fields[1]);
        return clock_ + cycles;
    }

}


void RISCV_CPU::_reshape(Tensor& tensor, const vector<int>& dims) {
    int size = accumulate(dims.begin(), dims.end(), 1, multiplies<int>());
    assert(size == tensor.size());

    tensor.dims.clear();
    tensor.dims = dims;
}

}