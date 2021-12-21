#ifndef __MONITOR_H__
#define __MONITOR_H__

#include <fstream>
#include <iostream>
#include <vector>
#include <map>
#include <memory>

#include "focus.hpp"
#include "trafficmanager.hpp"
#include "credit.hpp"

namespace focus
{

class Monitor {

protected:
    int _monitor_start_time;     // The time when the last monitor enabling phase starts. 
    int _processing_time;       // How many cycles do the application executes.
    bool _enable;               // Whether the monitor is enabled. 
    static std::shared_ptr<Monitor> _monitor;

public:
    Monitor() { reset(); }

    static std::shared_ptr<Monitor> getMonitor() {
        if (_monitor) {
            return _monitor;
        } else {
            _monitor = std::make_shared<Monitor>();
            return _monitor;
        }
    }

    void start_monitor(int time) { 
        _monitor_start_time = time;    
        _enable = true; 
    }

    void finish_monitor(int time) { 
        if (_enable) {
            _processing_time = time - _monitor_start_time;
        }
        _enable = false; 
    }

    void reset() { 
        _monitor_start_time = 0;
        _processing_time = 0;
        _enable = false;
    }

    bool update(std::shared_ptr<FocusInjectionKernel> injection_kernel, 
                const std::vector<std::map<int, Flit *>>& _total_in_flight_flits,
                int time)
    {
        // computation of node is finished && no flits are in flight

        bool compute_finished = injection_kernel->allNodeClosed();
        bool traffic_drained = true;
        for (auto& c: _total_in_flight_flits) {
            traffic_drained &= c.empty();
        }
        bool credit_drained = !Credit::OutStanding();

        if (traffic_drained && credit_drained && compute_finished) {
            finish_monitor(time);
            return false;
        }

        return true;
    }

};

}

#endif