#ifndef __TASK_H__
#define __TASK_H__

#include <string>
#include <vector>
#include <queue>
#include <map>

#include "bridge.hpp"
#include "operator.hpp"
#include "path.hpp"

namespace spatial {


class TaskParser {

private:
    std::vector<std::vector<std::shared_ptr<Operator>>> tasks_;     
    std::map<int, Tensor> tensors_;
    std::map<int, std::vector<int> > destinations_;        // tid -> dest
    std::string file_name;

    void _parseTensor(const std::string& line);
    void _parseOperator(const std::string& line, std::vector<std::shared_ptr<Operator>>& to);

    void _allocateMissingTensors();
    void _linkOpWithTensor();
    void _generate(std::vector<std::queue<std::string>>&, std::map<int, Tensor>&);

public:
    static void compileTaskFile(const std::string& file, std::vector<std::queue<std::string>>& _instrs, std::map<int, Tensor>& _data);
};


class MIParser {

public:
    static void parseLatencyFile(const std::string& file, std::map<std::string, std::shared_ptr<std::map<std::string, float>>>& latency);
};

class PathParser {

private:
    enum STATE {INIT, ATTR, SEG} _state = STATE::INIT;

    void _parseAttribute(const std::string& line, int& tid, MCTree& tree);
    void _parseSegment(const std::string& line, MCTree& tree);

public:
    static void parsePathFile(const std::string& file, std::map<int, MCTree>& path);
};

};

#endif