#include <math.h>
#include "accelerator.h"

namespace spatial {

ACCELERATOR::ACCELERATOR(const std::map<std::string, float> mi_): COMPONENT("ACCELERATOR", mi_) { 
    
}

int ACCELERATOR::pre_execute(const std::string& micro_instr, std::map<int, Tensor>& data) 
{
    std::vector<std::string> inst = split(micro_instr, " ");
    std::string inst_config = inst[1];
    int mac = 1;

    if (inst_config == "conv") {
        assert(inst_config.size() == 4);
        Tensor& output = data[std::stoi(inst[2])];
        // FIXME: well ... it's ugly ...
        Tensor& input = data[std::stoi(inst[4])];
        Tensor& weight = data[std::stoi(inst[3])];
        assert(input.dims.size() == 3);
        assert(weight.dims.size() == 4);

        if (output.dims.empty()) {
            output.dims.push_back(weight.dims[0]);
            output.dims.push_back(input.dims[1]);
            output.dims.push_back(input.dims[2]);
        }

        for (int d: output.dims) {
            mac *= d;
        }
        for (int d: weight.dims) {
            mac *= d;
        }
        mac /= weight.dims[0];
    } else {
        Tensor& output = data[std::stoi(inst[2])];

        std::vector<std::string>::iterator iter = inst.begin();
        inst.erase(iter, iter + 3);
        std::vector<std::string> input = inst;
        int input_num = input.size();

        std::string input_dims = split(inst_config, "->")[0];
        std::vector<std::string> input_dim = split(input_dims, ",");
        std::string output_dim = split(inst_config, "->")[1];

        if (input_num == 1) {
            std::string t1 = input_dim[0];
            for (int i = 0; i < t1.size(); i++) {
                int position = output_dim.find(t1[i]);
                if (position == output_dim.npos) {
                    Tensor& input_i = data[std::stoi(input[0])];
                    mac *= input_i.dims[i];
                }
            }
        } else {
            for (int i = 0; i < input_num - 1; i++) {
                std::string t1 = input_dim[i];
                std::string t2 = input_dim[i + 1];
                for (int j = 0; j < t2.size(); j++) {
                    int position = t1.find(t2[j]);
                    int o_position = output_dim.find(t2[j]);
                    if (position != t1.npos && o_position == output_dim.npos) {
                        Tensor& input_i = data[std::stoi(input[i])];
                        Tensor& input_i_nxt = data[std::stoi(input[i + 1])];
                        if (input_i.dims[position] != input_i_nxt.dims[j]) {
                          throw "Tensor Size Mismatch between Input Tensor " + input[i] + " and Tensor " + input[i + 1];
                        }
                        mac *= input_i.dims[position];
                    }
                }
            }
        }

        if (output_dim.size() != 0) {
            for (int i = 0; i < output_dim.size(); i++) {
                for (int j = 0; j < input_num; j++) {
                    int position = input_dim[j].find(output_dim[i]);
                    if (position != input_dim[j].npos) {
                        Tensor& input_i = data[std::stoi(input[i])];
                        mac *= input_i.dims[position];
                        output.dims.push_back(input_i.dims[position]);
                        break;
                    }
                }
            }
        } else {
            output.dims.push_back(1);
        }
    }
    return mac;
}

int ACCELERATOR::simple_sim(const std::string micro_instr, std::map<int, Tensor>& data, int clock_) {

    // FIXME: add concat support
    std::vector<std::string> inst = split(micro_instr, " ");
    std::string inst_type = inst[0];
    std::map <std::string, float> ::iterator latency_iter = _micro_instr_latency.find(inst_type);
    if (latency_iter == _micro_instr_latency.end()) {
        instruction_mismatch(micro_instr);
    } else {
        int mac = pre_execute(micro_instr, data);
        return clock_ + mac / latency_iter->second;
    }
    // if (mac != 0) {
    //     return clk >= std::ceil((float)mac / (latency_iter->second));
    // }
}

}