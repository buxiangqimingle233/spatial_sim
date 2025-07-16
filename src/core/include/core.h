#ifndef _CORE_H
#define _CORE_H

#include <fstream>
#include <map>
#include "common.h"
#include "network_interface.h"
#include "bus.h"
#include "riscv_cpu.h"
#include "accelerator.h"
#include "buffer.h"
#include "bridge.hpp"
#include "parser.h"

namespace spatial {

extern int array_size;

class CORE {
public:

    // initialization
    CORE(const std::string&, const std::string&, const std::string&, int, CNInterface, CNInterface , std::shared_ptr<std::vector<bool> >, int, int);
    void SetupSim();
    
    // execute
    void ClockTick(int clock);

    shared_ptr<COMPONENT> getExecuteComponent(std::string instr);
    int selectActiveTask(int clock);
    bool isBusy(int clock);
    void DisplayStats(ostream & os = std::cout ) const;

    // statistics functions
    int getBusyCycles() const;
    int getIdleCycles() const;

protected:
    std::vector<shared_ptr<COMPONENT>> _modules;
    std::vector<std::queue<std::string>> _tasks;
    std::vector<int> _cycle_to_issue;
    std::map<int, Tensor> data;
    int cid;

    // statistics tracking
    int _busy_cycles;
    int _idle_cycles;
    int _last_clock;

public:
    // control
    inline bool equalTo(CORE& other) const {
        return _cycle_to_issue == other._cycle_to_issue && _tasks == other._tasks;
    }

    inline bool finishAllTasks(int clock) const {
        auto t = _tasks.begin();
        auto c = _cycle_to_issue.begin();
        for (; t != _tasks.end() && c != _cycle_to_issue.end(); ++t, ++c) {
            if (!t->empty() || *c > clock) {
                return false;
            }
        }
        return true;
    }
};

}
#endif