#include <iostream>
#include <fstream>
#include <string>
#include <algorithm>
#include "core.h"
#include "bridge.hpp"
#include "parser.h"

namespace spatial {
using namespace std;

CORE::CORE(
    const string& instruction_file, const string& latency_file, const string& rb_file, 
    int cid_, CNInterface sq_, CNInterface rq_, shared_ptr<vector<bool> > pipe_open_, 
    int threshold_, int width_ = 128
): cid(cid_), _busy_cycles(0), _idle_cycles(0), _last_clock(0)
{
    clock_t start, end;
    start = clock();

    // Setup hardare modules
    map<string, shared_ptr<map<string, float>> > latency = {
        {"BUS", make_shared<map<string, float>>()}, 
        {"CPU", make_shared<map<string, float>>()}, 
        {"ACC", make_shared<map<string, float>>()}, 
        {"BUFFER", make_shared<map<string, float>>()}, 
        {"NI", make_shared<map<string, float>>()}, 
    };

    MIParser::parseLatencyFile(latency_file, latency);

    _modules.push_back(make_shared<BUS>(*(latency["BUS"])));
    _modules.push_back(make_shared<RISCV_CPU>(*(latency["CPU"])));
    _modules.push_back(make_shared<ACCELERATOR>(*(latency["ACC"])));
    _modules.push_back(make_shared<BUFFER>(*(latency["BUFFER"])));
    _modules.push_back(make_shared<NI>(sq_, rq_, pipe_open_, *(latency["NI"]), cid_, rb_file, threshold_, width_));

    TaskParser::compileTaskFile(instruction_file, _tasks, data);
    _cycle_to_issue.resize(_tasks.size(), -1);

    end = clock();

    cout << "Initialize | " << "Initialization for core " << cid \
         << " completes, which generates " << _tasks.size() << " paralleled tasks: ";
    for (auto serial_task: _tasks) {
        cout << serial_task.size() << "-";
    }
    cout << "instructions, and takes " << (double)(end-start) / CLOCKS_PER_SEC << " seconds." << endl;
}


shared_ptr<COMPONENT> CORE::getExecuteComponent(std::string micro_instr) {
    micro_instr.erase(std::remove_if(micro_instr.begin(), micro_instr.end(), ::isspace), micro_instr.end());
    std::string executer_name = split(micro_instr, ".")[0];
    for (shared_ptr<COMPONENT> c: _modules) {
        if (c->name == executer_name) {
            return c;
        }
    }
    return nullptr;
}


void CORE::ClockTick(int clock) {

    // Update statistics based on previous clock cycle
    if (clock > _last_clock) {
        if (_last_clock > 0) {
            if (isBusy(_last_clock)) {
                _busy_cycles++;
            } else {
                _idle_cycles++;
            }
        }
        _last_clock = clock;
    }

    // if no instructions lie on the instruction list or 
    if (finishAllTasks(clock)) {
        return;
    }
    // if some tasks are taking up the core
    if (isBusy(clock)) {
        return;
    }

    int idx = selectActiveTask(clock);
    // no task is activate
    if (idx < 0) {
        return; 
    }

    assert(_tasks[idx].size());
    assert(idx < _cycle_to_issue.size() && idx >= 0 && \
            (_cycle_to_issue[idx] <= clock || _cycle_to_issue[idx] == -1));

    queue<string>& task_to_issue = _tasks[idx];
    int& issue_cycle = _cycle_to_issue[idx];

    // fire the next instruction
    if (issue_cycle != -1) {
        string finished_instr = task_to_issue.front();
        task_to_issue.pop();
        cout << clock << " | CORE" << cid << " | Finish Micro-Inst: | " << finished_instr << endl;
    } 
    if (task_to_issue.size()) {
        string current_instr = task_to_issue.front();
        shared_ptr<COMPONENT> executer = getExecuteComponent(current_instr);
        assert(executer != nullptr);
        issue_cycle = executer->simple_sim(current_instr, data, clock);
    }
}


int CORE::selectActiveTask(int clock) {
    int task_num = _tasks.size();
    std::vector<int> candidates;
    for (int idx = 0; idx < task_num; ++idx) {
        if (!_tasks[idx].empty() && (_cycle_to_issue[idx] == -1 || _cycle_to_issue[idx] <= clock)) {
            candidates.push_back(idx);
        }
    }
    if (candidates.empty()) {
        return -1;
    }
    std::random_shuffle(candidates.begin(), candidates.end());
    // The forwarding operators have high priority, while redo operators have low priority
    for (int idx: candidates) {
        if (_cycle_to_issue[idx] != -1) {
            return idx;
        }
    }
    return candidates.front();
}


bool CORE::isBusy(int clock) {
    for (int c: _cycle_to_issue) {
        if (c != -1 && c > clock) {
            return true;
        }
    }
    return false;
}


int CORE::getBusyCycles() const {
    return _busy_cycles;
}

int CORE::getIdleCycles() const {
    return _idle_cycles;
}

void CORE::DisplayStats(std::ostream & os) const {
    os << "CORE " << cid << " Stats: " << std::endl;
    if (finishAllTasks(0)) {
        os << "All instructions are finished, this core is closed" << std::endl;
    } else {
        os << "Some instructions have not been executed, the front instructions are: ";
        for (auto t: _tasks) {
            os << t.front() << "-";
        }
        os << std::endl; 
    }

    os << "Modules Stats: " << std::endl;
    for (shared_ptr<COMPONENT> c : _modules) {
        c->DisplayStats(os);
    }
    os << std::endl;
}

}