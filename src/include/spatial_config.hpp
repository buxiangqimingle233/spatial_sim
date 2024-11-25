#ifndef __CUSTOM_CONFIGS_H__
#define __CUSTOM_CONFIGS_H__

#include <assert.h>
#include "config_utils.hpp"
#include "booksim_config.hpp"


class SpatialSimConfig : public BookSimConfig {
public:
    SpatialSimConfig() {

        AddStrField("log_file", "log.txt");

        // tasks
        AddStrField("tasks", "");
        AddStrField("working_directory", "tasks");

        // micro-instruction latencies
        AddStrField("micro_instr_latency", "runfiles/micro_instr_latency");

        // routing-boards
        AddStrField("routing_board", "routing_board");

        _int_map["threshold"] = 2;      // When to reject accepting packets
        _int_map["array_size"] = 16;    // The size of core array, FIXME: Deprecated now
        _int_map["deadlock_check_freq"] = 1000;     // How much cycles do we check deadlocks

    }

    void checkConsistency() {
        assert(_int_map["k"] ^ _int_map["n"] == _int_map["array_size"]);
    }
};

#endif