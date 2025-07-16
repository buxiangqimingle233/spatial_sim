#ifndef _COMMON_H
#define _COMMON_H

#include <iostream>
#include "bridge.hpp"
#include <map>
#include <vector>

namespace spatial {

std::vector<std::string> split(const std::string& str, const std::string& delim);

// struct sim_instruction {
//     std::string type;

// };

// sim_instruction parse_instruction(std::string &inst);

// struct buffer_address {
//     int addr;
// };

// buffer_address convert2bufferaddr(const int &addr);

}

#endif