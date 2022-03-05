
#include <iostream>
#include <fstream>
#include <sstream>
#include <memory>
#include <string>
#include <string.h>
#include "noc.hpp"

// Booksim:main.cpp dependency
#include "booksim.hpp"
#include "routefunc.hpp"
#include "traffic.hpp"
#include "booksim_config.hpp"
#include "trafficmanager.hpp"
#include "random_utils.hpp"
#include "network.hpp"
#include "injection.hpp"
#include "power_module.hpp"
#include "globals.hpp"

/* printing activity factor*/
bool gPrintActivity;
int gK; //radix
int gN; //dimension
int gC; //concentration
int gNodes;
//generate nocviewer trace
bool gTrace;
ostream * gWatchOut;

// Well ... some old troublesomes of booksim2
TrafficManager* trafficManager = NULL;

std::shared_ptr<spatial::NoC> spatial::NoC::New(
    int argc, char** argv, CNInterfaceSet sqs, 
    CNInterfaceSet rqs
) {

    BookSimConfig config;
    std::srand(1234);

    if ( !ParseArgs( &config, argc, argv ) ) {
        cerr << "Usage: " << argv[0] << " configfile... [param=value...]" << endl;
        return nullptr;
    } 

    /*initialize routing, traffic, injection functions
   */
    InitializeRoutingMap(config);

    gPrintActivity = (config.GetInt("print_activity") > 0);
    gTrace = (config.GetInt("viewer_trace") > 0);
    
    string watch_out_file = config.GetStr( "watch_out" );
    if(watch_out_file == "") {
        gWatchOut = NULL;
    } else if(watch_out_file == "-") {
        gWatchOut = &cout;
    } else {
        gWatchOut = new ofstream(watch_out_file.c_str());
    }

    // Setup Nets
    vector<Network *> net;
    int subnets = config.GetInt("subnets");
    if (subnets != 1) {
        throw "The number of subnets is not 1 !!";
    }

    /*To include a new network, must register the network here
    *add an else if statement with the name of the network
    */
    net.resize(subnets);
    for (int i = 0; i < subnets; ++i) {
        ostringstream name;
        name << "network_" << i;
        net[i] = Network::New( config, name.str() );
    }

    TrafficManager* _traffic_manager = TrafficManager::New(config, net);
    _traffic_manager->SetupSim(sqs, rqs);

    trafficManager = _traffic_manager;

    return std::make_shared<NoC>(_traffic_manager, config);

}


void spatial::NoC::step(clock_t clock) {
    _traffic_manager->_Step();

    if (clock % 1000  == 0) {
        std::cout << "Simulate " << clock << " cycles" << std::endl;
#ifdef DUMP_NODE_STATE
        std::ofstream out("dump.log", std::ios::out|std::ios::app);
        out.close();
#endif
    }
}