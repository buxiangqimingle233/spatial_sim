#include <assert.h>
#include <sstream>
#include <iostream>
#include <fstream>
#include <string>
#include <algorithm>
#include <map>
#include <time.h>

#include "common.h"
#include "bridge.hpp"
#include "parser.h"


void spatial::TaskParser::compileTaskFile(const std::string& file, \
    std::vector<std::queue<std::string>>& mi_to, std::map<int, Tensor>& data_to) 
{
    using namespace std;
    TaskParser parser;

    parser.file_name = file;
    std::ifstream task_file(file, std::ios::in);
    if (!task_file.is_open()) {
        throw "Task speficication file " + file + " not founded !";
    }

    // parse the file to operators
    std::string line;
    enum FIELD {op_, data_, comment_} flag = FIELD::comment_;
    while (getline(task_file, line)) {
        // Set file segment
        if (line.find("operators") != line.npos) {
            flag = FIELD::op_;
        } else if (line.find("data") != line.npos) {
            flag = FIELD::data_;
        }
        else if (line.find("//") != line.npos) { 
            flag = flag;
        }
        // Parse the line
        else if (!line.empty()) {
            if (flag == FIELD::op_) {
                if (line.find("{") != line.npos) {
                    parser.tasks_.push_back(std::vector<std::shared_ptr<Operator>>());
                } else if (line.find("}") != line.npos) {
                } else {
                    // assert(parser._tasks.size());
                    if (parser.tasks_.empty()) {
                        parser.tasks_.push_back(std::vector<std::shared_ptr<Operator>>());
                    }
                    parser._parseOperator(line, parser.tasks_.back());
                }
            } else if (flag == FIELD::data_) {
                parser._parseTensor(line);
            }
        }
    }

    /** Lower the task specifications to micro instructions
     * pkt receive, bus trans, buffer write,
     * buffer read, bus trans, ai-core / riscv comp, bus trans, buffer write, 
     * buffer read, pkt send
     */
    parser._allocateMissingTensors();
    // std::cout << "Pour out tensor finish" << std::endl;
    parser._linkOpWithTensor();
    // std::cout << "setup ops finish" << std::endl;
    parser._generate(mi_to, data_to);
    // std::cout << "gen micro instr finish" << std::endl;
}


void spatial::TaskParser::_parseOperator(const std::string& line, std::vector<std::shared_ptr<Operator>>& to) {
    int pos = line.find("#");
    std::string type = line.substr(0, pos);
    std::shared_ptr<Operator> ptr = Operator::getOperator(type);
    ptr->parse(line.substr(pos + 1));
    to.push_back(ptr);
}


void spatial::TaskParser::_parseTensor(const std::string& line) {
    // 1 # 2, 0, 4 # 7, 8, 9

    Tensor tensor = Tensor();

    std::string remain = line;
    remain.erase(std::remove_if(remain.begin(), remain.end(), ::isspace), remain.end());

    std::vector<std::string> fields = split(remain, "#");
    assert(fields.size() >= 2);
    
    if (fields.size() == 2) {
        fields.push_back("");
    }

    // Extract tensor id
    int tid = atoi(fields[0].c_str());
    tensor.tid = tid;

    // Extract destination nodes
    destinations_[tid] = std::vector<int>();
    std::string dests = fields[1];
    std::replace(dests.begin(), dests.end(), ',', ' ');
    std::stringstream dss(dests);
    int id;
    while (dss >> id) {
        destinations_[tid].push_back(id);
    }

    // Extract dims
    std::string dims = fields[2];
    std::replace(dims.begin(), dims.end(), ',', ' ');
    std::stringstream ss(dims);
    int dim = -1;
    while (ss >> dim) {
        tensor.dims.push_back(dim);
    }

    tensors_.insert(std::make_pair(tensor.tid, tensor));
}


void spatial::TaskParser::_allocateMissingTensors() {
    std::map<int, Tensor> data = tensors_;

    auto check_insert = [&data] (int tid, bool& exist) {
        if (data.find(tid) == data.end()) {
            Tensor t = Tensor();
            t.tid = tid;
            data.insert(std::make_pair(tid, t));
            exist = false;
        }
        exist = true;
    };

    bool exist = false;
    for (auto track: tasks_) {
        for (std::shared_ptr<Operator> op: track) {
            for (int tid: op->inputs) {
                check_insert(tid, exist);
            }
            for (int tid: op->outputs) {
                check_insert(tid, exist);
                assert(exist);
            }
        }
    }
}


void spatial::TaskParser::_linkOpWithTensor()
{   
    // check each output tensor is specified with a destination node
    for (auto track: tasks_) {
        for (std::shared_ptr<Operator> op: track) {
            std::vector<int> outputs = op->getOutputCopy();
            for (int o: outputs) {
                if (destinations_.find(o) == destinations_.end()) {
                    throw "operator & data mismatch at tensor " + std::to_string(o) + " in file " + file_name;
                }
            }
        }
    }
    // Set dest nodes for each operation
    for (auto track: tasks_) {
        for (std::shared_ptr<Operator> op: track) {
            op->setDestNodes(destinations_);
        }
    }
    // We just discover stationary within a task for computation operators
    for (auto serial_task: tasks_) {
        std::vector<std::shared_ptr<Operator>> compute_ops;
        std::copy_if(serial_task.begin(), serial_task.end(), std::back_inserter(compute_ops), 
            [](shared_ptr<Operator> op) { return typeid(*op) == typeid(CompOperator); });
        CompOperator::DiscoverStationary(compute_ops);
    }
}


void spatial::TaskParser::_generate(std::vector<std::queue<std::string>>& instrs, std::map<int, spatial::Tensor>& data) {

    data.insert(tensors_.begin(), tensors_.end());

    // Generate micro instructions
    instrs.resize(tasks_.size(), std::queue<std::string>());
    auto serial_task = tasks_.begin();
    auto micro_instr = instrs.begin();
    for (; serial_task != tasks_.end() && micro_instr != instrs.end(); ++serial_task, ++micro_instr) {
        for (std::shared_ptr<Operator> op: *serial_task) {
            op->lower(data, *micro_instr);
        }
    }

}


void spatial::MIParser::parseLatencyFile(const std::string& file,  \
    std::map<std::string, std::shared_ptr<std::map<std::string, float>>>& latency)
{
    std::ifstream mi_latency;
    mi_latency.open(file, std::ios::in);
    if (!mi_latency.is_open()) {
        throw "Latency Spec Open Failed";
    }

    std::string line = "";
    while (getline(mi_latency, line)) {
        int pos = line.find(" ");
        std::string subinstr = line.substr(0, pos);
        float cycles = stof(line.substr(pos + 1));
        std::string comp = subinstr.substr(0, subinstr.find("."));
        assert(latency.count(comp) == 1);
        latency[comp]->insert(make_pair(subinstr, cycles));
    }
}

void spatial::PathParser::parsePathFile(const std::string& file, std::map<int, MCTree>& path) {

    std::ifstream stream;
    stream.open(file, std::ios::in);
    if (!stream.is_open()) {
        std::cerr << "WARNING: The routing board file " << file \
                  << " does not exist, the routing board is set empty. " \
                  << std::endl;
        return;
    }

    std::string line = "";
    PathParser parser;
    MCTree pres_tree = MCTree(INVALID);
    int pres_tid = INVALID;

    while (getline(stream, line)) {
        if (parser._state == STATE::INIT && !line.empty()) {
            parser._parseAttribute(line, pres_tid, pres_tree);            
            parser._state = STATE::ATTR;
        } else if (parser._state == STATE::ATTR && !line.empty()) {
            parser._parseSegment(line, pres_tree);
            parser._state = STATE::SEG;
        } else if (parser._state == STATE::SEG && !line.empty()) {
            parser._parseSegment(line, pres_tree);
        } else if (parser._state == STATE::SEG && line.empty()) {
            path.insert(make_pair(pres_tid, pres_tree));
            pres_tree = MCTree(INVALID);         // reset
            pres_tid = INVALID;
            parser._state = STATE::INIT;
        } else {
            parser._state = parser._state;  // do nothing
        }
    }

    // If there hasn't the last blank line
    if (pres_tid != INVALID) {
        path.insert(make_pair(pres_tid, pres_tree));
    }
}

void spatial::PathParser::_parseAttribute(const std::string& line, int& tid, MCTree& tree) {
    assert(tree.root() < 0);
    stringstream ss(line);
    int src;
    std::set<int> dsts;
    ss >> tid >> src;
    int d;
    while (ss >> d) {
        dsts.insert(d);
    }
    tree = MCTree(src);
    tree.setDestNodes(dsts);
}

void spatial::PathParser::_parseSegment(const std::string& line, MCTree& tree) {
     assert(tree.root() >= 0);
    stringstream ss(line);
    int seg_start, seg_end;
    std::shared_ptr<std::queue<int>> im_nodes;
    ss >> seg_start >> seg_end;
    int in;
    while (ss >> in) {
        im_nodes->push(in);
    }
    tree.addSegment(seg_start, seg_end, im_nodes, tree.isDestNode(seg_end));
}
