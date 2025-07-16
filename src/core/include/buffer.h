#ifndef _BUFFER_H
#define _BUFFER_H

#include "component.h"

namespace spatial {

class BUFFER : public COMPONENT {
public:
    using COMPONENT::COMPONENT;
    BUFFER(const std::map<std::string, float> mi_);
    virtual int simple_sim(const std::string micro_instr, std::map<int, Tensor>& data, int clock_) override;
private:
    
};

}

#endif