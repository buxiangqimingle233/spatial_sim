#ifndef __OPERATOR_H__
#define __OPERATOR_H__

#include <assert.h>
#include <string>
#include <vector>
#include <queue>
#include <map>
#include <sstream>
#include <algorithm>
#include "common.h"
#include "bridge.hpp"

class CompOperator;
class ManOperator;

class Operator {

public:
    // Set by primary parsing
    std::string config;
    std::vector<int> inputs;
    std::vector<int> outputs;

    // Set by joint analysis
    std::map<int, std::vector<int> > dest_nodes;
    std::vector<bool> input_stationary;
    std::vector<bool> output_stationary;

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

    void setStationary(const std::vector<bool>& is, const std::vector<bool>& os) {
        input_stationary = is;
        output_stationary = os;
    }

    virtual void parse(const std::string& line) = 0;

    virtual std::queue<std::string> lower(const std::map<int, spatial::Tensor>& data) = 0;
};


class CompOperator : public Operator {

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

    virtual std::queue<std::string> lower(const std::map<int, spatial::Tensor>& data) override {
        std::queue<std::string> mi;
        char buf[1000] = "";

        // get input messages
        for (int i = 0; i < inputs.size(); ++i) {
            int tid = inputs[i];
            if (!input_stationary[i]) {
                // Ni.receive

                // FIXME: recover this
                sprintf(buf, "NI.recv %d", tid);    // config the tensor
                mi.push(std::string(buf));

                // Bus.transaction
                sprintf(buf, "BUS.trans %d", tid);
                mi.push(std::string(buf));

                // Buffer.write
                sprintf(buf, "BUFFER.write %d", tid);
                mi.push(std::string(buf));
            }
        }

        for (int o = 0; o < outputs.size(); ++o) {
            std::stringstream ss;
            // Read inputs
            for (int dep: inputs) {
                sprintf(buf, "BUFFER.read %d", dep);
                mi.push(std::string(buf));
                
                sprintf(buf, "BUS.trans %d", dep);
                mi.push(std::string(buf));
                ss << dep << " ";
            }

            int tid = outputs[o];
            // TODO: generate computation micro instructions, check which hd component do we need
            if (config.empty()) {
                sprintf(buf, "CPU.sleep %d", 1);
                mi.push(std::string(buf));
            } else {
                sprintf(buf, "ACC.cal %s %d %s", config.c_str(), tid, ss.str().c_str());
                mi.push(std::string(buf));
            }

            sprintf(buf, "BUS.trans %d", tid);
            mi.push(std::string(buf));

            sprintf(buf, "BUFFER.write %d", tid);
            mi.push(std::string(buf));

            // Transmit outputs
            if (!output_stationary[o]) {
                std::vector<int> dests = dest_nodes[tid];
                // TODO: We just support unicast now
                for (int dest: dests) {
                    sprintf(buf, "BUFFER.read %d", tid);
                    mi.push(std::string(buf));
                    
                    sprintf(buf, "BUS.trans %d", tid);
                    mi.push(std::string(buf));
                    
                    sprintf(buf, "NI.send %d %d", dest, tid);
                    mi.push(std::string(buf));
                }
            }
        }
        return mi;
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

    virtual std::queue<std::string> lower(const std::map<int, spatial::Tensor>& data) override {
        auto casted = const_cast<std::map<int, spatial::Tensor>&>(data);
        for (int o: outputs) {
            casted.find(o)->second.size();
        }

        std::queue<std::string> mi;
        char buf[1000] = "";
        for (int cnt = 0; cnt < iter_cnt; ++cnt) {
            for (int tid: outputs) {
                for (int dest: dest_nodes[tid]) {
                    sprintf(buf, "NI.send %d %d", dest, tid);
                    mi.push(std::string(buf));
                }
            }

            sprintf(buf, "CPU.sleep %d", interval);
            mi.push(std::string(buf));
        }
        return mi;
    }

};

#endif