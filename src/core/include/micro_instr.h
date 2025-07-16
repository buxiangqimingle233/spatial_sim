#ifndef __MICRO_INSTR_H__
#define __MICRO_INSTR_H__

#include <vector>
#include "component.h"

namespace spatial {
    struct CompInstr {
        std::shared_ptr<COMPONENT> executer;    // which item to execute
        std::shared_ptr<int> command;           // point to a CompInstr within the micro instruction board
        std::vector<int> paras;                 // parameter
    };

}


#endif