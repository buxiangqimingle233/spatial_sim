#ifndef _ACCELERATOR_H
#define _ACCELERATOR_H

#include "component.h"

namespace spatial {

class ACCELERATOR : public COMPONENT {
public:

    using COMPONENT::COMPONENT;
    ACCELERATOR(const std::map<std::string, float> mi_);
    virtual int simple_sim(const std::string micro_instr, std::map<int, Tensor>& data, int clock_) override;

private:
    int pre_execute(const std::string& micro_instr, std::map<int, Tensor>& data);
};

}

#endif