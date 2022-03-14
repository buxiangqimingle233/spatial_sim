#include <string>
#include <memory>
#include "instruction.h"

namespace spatial {

INSTRUCTION::INSTRUCTION(std::string inst) {
    // init state
    std::vector<std::string> tmp_inst = split(inst, " ");
    std::vector<std::string>::iterator iter =  tmp_inst.begin();

    type = tmp_inst[0];
    config = tmp_inst[1];
    
    tmp_inst.erase(iter, iter + 1);
    operand = tmp_inst;
}

std::shared_ptr<INSTRUCTION> INSTRUCTION::parse_instruction(std::string inst){
    std::shared_ptr<INSTRUCTION> ptr = nullptr;
    if (split(inst, " ").front() == "poll") {
        ptr = std::make_shared<POLL>(inst);
    } else if (split(inst, " ").front() == "einsum") {
        ptr = std::make_shared<EINSUM>(inst);
    }
    
    return ptr;
}


void POLL::translate(std::queue <std::string> &micro_instrucion, std::vector<Tensor> &data) {
    std::cout << "Polling Core Components" << std::endl;
    micro_instrucion.push("CPU.cal");
    micro_instrucion.push("Bus.trans");

    data.push_back(Tensor());
    Tensor* ptr = data.data() + data.size() - 1;
    micro_instrucion.push("Ni.send 0 1 2 1 0 " + std::to_string((int)ptr));
}

void EINSUM::translate(std::queue <std::string> &micro_instrucion, std::vector<Tensor> &data) {
    std::cout << "Calculating EINSUM" << std::endl;
    // auto op = split(config, "->")[1];
    // auto subscript = split(op, ",");

    // int op_num =operand.size();
    // for (int i = 0; i < op_num; i++) {
    //     data.push_back(Tensor());
    // }
    Tensor* ptr = data.data();
    if (config == "ik,k->i") {
        micro_instrucion.push("Buffer.read" + std::to_string(ptr->tid));
        micro_instrucion.push("Buffer.read" + std::to_string((ptr + 1)->tid));
        micro_instrucion.push("ACC.gemm" + std::to_string(ptr->tid) + std::to_string((ptr + 1)->tid) + std::to_string((ptr + 2)->tid));
        micro_instrucion.push("Buffer.write" + std::to_string((ptr + 2)->tid));
    }
    
}

};