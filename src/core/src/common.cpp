#include "common.h"

namespace spatial{

std::vector<std::string> split(const std::string& str, const std::string& delim) {
    std::vector<std::string> res;
    if ("" == str) {
        return res;
    }

    std::string strs = str + delim;
    int pos;
    int size = strs.size();

    for (int i = 0; i < size; i++) {
        pos = strs.find(delim, i);
        if (pos < size) {
            std::string s = strs.substr(i, pos - i);
            if (s != "") {
                res.push_back(s);
            }
            i = pos + delim.size() - 1;
        }
    }
    return res;
}

// sim_instruction parse_instruction(std::string &inst) {
//     sim_instruction s_inst;
//     s_inst.type = inst;
//     return s_inst;
// }

// buffer_address convert2bufferaddr(const int &addr) {
//     buffer_address b_addr;
//     b_addr.addr = addr;
//     return b_addr;
// }

}
