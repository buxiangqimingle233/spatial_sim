#ifndef _INSTRUCTION_H
#define _INSTRUCTION_H

#include "common.h"

namespace spatial {

class INSTRUCTION {
public:
    INSTRUCTION(std::string inst);
    virtual ~INSTRUCTION() {
        std::cout << "instruction " << type << " free" << std::endl;
    };
    static std::shared_ptr<INSTRUCTION> parse_instruction(std::string inst);
    virtual void translate(std::queue<std::string> &micro_instrucion, std::vector<Tensor> &data) = 0;

protected:
    std::string type;
    std::string config;
    std::vector<std::string> operand;
};


class POLL : public INSTRUCTION {
public:
    using INSTRUCTION::INSTRUCTION;
    POLL(std::string inst): INSTRUCTION(inst) { }; 

    void translate(std::queue<std::string> &micro_instrucion, std::vector<Tensor> &data);
};

class EINSUM : public INSTRUCTION {
public:
    using INSTRUCTION::INSTRUCTION;
    EINSUM(std::string inst): INSTRUCTION(inst) { };

    void translate(std::queue<std::string> &micro_instrucion, std::vector<Tensor> &data);

};

}
#endif
