#ifndef __OPERATOR_H__
#define __OPERATOR_H__

#include <assert.h>
#include <string>
#include <vector>
#include <queue>
#include <map>
#include <sstream>
#include <algorithm>
#include <set>
#include <iterator>
#include "common.h"
#include "bridge.hpp"

class CompOperator;
class ManOperator;

class Operator {

public:
    // Set by primary parsing
    std::vector<int> inputs;
    std::vector<int> outputs;

    // Set by joint analysis
    std::map<int, std::vector<int> > dest_nodes;

public:

    static std::shared_ptr<Operator> getOperator(std::string type) {
        std::shared_ptr<Operator> ptr;
        if (type.find("compute") != type.npos) {
            ptr = std::dynamic_pointer_cast<Operator>(std::make_shared<CompOperator>());
        } else if (type.find("manipulate") != type.npos) {
            ptr = std::dynamic_pointer_cast<Operator>(std::make_shared<ManOperator>());
        } else {
            // ptr = std::make_shared<Operator>();
            throw "Please specify operator type !";
        }      
        return ptr;
    }

    Operator() { };

    std::vector<int> getOutputCopy() {
        return outputs;
    }

    void setDestNodes(std::map<int, std::vector<int>>& destinations) {
        for (int o: outputs) {
            assert(destinations.find(o) != destinations.end());
            dest_nodes[o] = destinations[o];
        }
    }

    virtual void parse(const std::string& line) = 0;

    virtual void lower(const std::map<int, spatial::Tensor>& data, std::queue<std::string>& out) = 0;
};


class CompOperator : public Operator {

public:
    std::map<int, bool> input_stationary;
    std::string config;

    virtual void parse(const std::string& line) override {
        // ij, jk -> i # 1, 2, 5, 7 # 8, 9, 10

        // remove spaces
        std::string remain = line;
        remain.erase(std::remove_if(remain.begin(), remain.end(), ::isspace), remain.end());

        // divide the line
        std::vector<std::string> fields = spatial::split(remain, "#");
        assert(fields.size() == 3);

        // Extract op configs
        config = fields[0];

        // Extract op outputs
        std::string output = fields[1];
        std::replace(output.begin(), output.end(), ',', ' ');
        std::stringstream oss(output);
        int id;
        while (oss >> id) { 
            outputs.push_back(id);
        }

        // Extract op inputs
        std::string input = fields[2];
        std::replace(input.begin(), input.end(), ',', ' ');
        std::stringstream iss(input);
        while (iss >> id) {
            inputs.push_back(id);
        }
    }

    virtual void lower(const std::map<int, spatial::Tensor>& data, std::queue<std::string>& out) override {
        char buf[1000] = "";

        // get input messages
        for (int i = 0; i < inputs.size(); ++i) {
            int tid = inputs[i];
            if (!input_stationary[tid]) {
                // Ni.receive
                sprintf(buf, "NI.recv %d", tid);    // config the tensor
                out.push(std::string(buf));

                // Bus.transaction
                sprintf(buf, "BUS.trans %d", tid);
                out.push(std::string(buf));

                // Buffer.write
                sprintf(buf, "BUFFER.write %d", tid);
                out.push(std::string(buf));
            }
        }

        for (int o = 0; o < outputs.size(); ++o) {
            std::stringstream ss;
            // Read inputs
            for (int dep: inputs) {
                sprintf(buf, "BUFFER.read %d", dep);
                out.push(std::string(buf));
                
                sprintf(buf, "BUS.trans %d", dep);
                out.push(std::string(buf));
                ss << dep << " ";
            }

            int tid = outputs[o];
            // TODO: generate computation micro instructions, check which hd component do we need
            if (config.empty()) {
                sprintf(buf, "CPU.sleep %d", 1);
                out.push(std::string(buf));
            } else {
                sprintf(buf, "ACC.cal %s %d %s", config.c_str(), tid, ss.str().c_str());
                out.push(std::string(buf));
            }

            sprintf(buf, "BUS.trans %d", tid);
            out.push(std::string(buf));

            sprintf(buf, "BUFFER.write %d", tid);
            out.push(std::string(buf));

            // Transmit outputs
            std::vector<int> dests = dest_nodes[tid];

            sprintf(buf, "BUFFER.read %d", tid);
            out.push(std::string(buf));

            sprintf(buf, "BUS.trans %d", tid);
            out.push(std::string(buf));
            
            // Unicast & multicast
            ss.str("");
            ss << "NI.send " << tid;
            for (int dest: dests) {
                ss << " " << dest;
            }
            out.push(ss.str());
        }
    }

    static void DiscoverStationary(std::vector<std::shared_ptr<Operator>>& op_list) {
        // Input stationary
        set<int> vis_tid;
        for (shared_ptr<Operator> op: op_list) {
            for (int dep: op->inputs) {
                if (vis_tid.count(dep) == 0) {
                    dynamic_pointer_cast<CompOperator>(op)->input_stationary.insert(make_pair(dep, false));
                    vis_tid.insert(dep);
                } else {
                    dynamic_pointer_cast<CompOperator>(op)->input_stationary.insert(make_pair(dep, false));
                }
            }
        }

        // TODO: We do not support output stationary at present.

    }

};

class ManOperator : public Operator {

protected:
    int interval = 0;
    int iter_cnt = 0;

    virtual void parse(const std::string& line) override {
        // interval # cnt # out tids
        
        std::string remain = line;
        remain.erase(std::remove_if(remain.begin(), remain.end(), ::isspace), remain.end());
        std::vector<std::string> fields = spatial::split(remain, "#");
        assert(fields.size() == 3);

        interval = stoi(fields[0]);
        iter_cnt = stoi(fields[1]);

        // Extract out tids
        std::string output = fields[2];
        std::replace(output.begin(), output.end(), ',', ' ');
        std::stringstream oss(output);
        int id;
        while (oss >> id) { 
            outputs.push_back(id);
        }
    }

    virtual void lower(const std::map<int, spatial::Tensor>& data, std::queue<std::string>& out) override {
        auto casted = const_cast<std::map<int, spatial::Tensor>&>(data);
        for (int o: outputs) {
            casted.find(o)->second.size();
        }

        for (int cnt = 0; cnt < iter_cnt; ++cnt) {
            for (int tid: outputs) {
                std::stringstream ss;
                std::copy(dest_nodes[tid].begin(), dest_nodes[tid].end(), std::ostream_iterator<int>(ss, " "));
                out.push("NI.send " + std::to_string(tid) + " " + ss.str());
                // std::stringstream ss;
                // ss << "NI.send " << tid;
                // for (int dest: dest_nodes[tid]) {
                //     ss << " " << dest;
                // }
            }

            char buf[1000] = "";
            sprintf(buf, "CPU.sleep %d", interval);
            out.push(std::string(buf));
        }
    }

};

#endif