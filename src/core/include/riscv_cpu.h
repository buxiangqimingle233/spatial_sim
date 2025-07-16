#ifndef _RISCV_CPU_H
#define _RISCV_CPU_H

#include "component.h"

namespace spatial {

class RISCV_CPU : public COMPONENT {
protected:
    void _reshape(Tensor& tensor, const vector<int>& dims);

public:
    using COMPONENT::COMPONENT;
    RISCV_CPU(const std::map<std::string, float> mi_);
    virtual int simple_sim(const std::string micro_instr, std::map<int, Tensor>& data, int clock_) override;
    
};

}

#endif