#include "spatial_chip.hpp"
#include "stdlib.h"

int main(int argc, char **argv) {

    // For debugging
    srand(1000);
    try {
        spatial::SpatialChip test_chip(argv[1]);
        test_chip.run();
        std::cout << "compute cycles:" << std::endl;
        for (int c: test_chip.compute_cycles()) {
            std::cout << c << " ";
        }
        std::cout << std::endl;
        std::cout << "communicate cycles: " << std::endl;
        for (int c: test_chip.communicate_cycles()) {
            std::cout << c << " ";
        }
        std::cout << std::endl;
    }
    catch (const std::string msg) {
        std::cerr << msg << std::endl;
        return -1;
    }
    return 0;
}

