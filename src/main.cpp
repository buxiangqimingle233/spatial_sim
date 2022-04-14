#include "spatial_chip.hpp"
#include "stdlib.h"

int main(int argc, char **argv) {

    // For debugging
    srand(1000);
    try {
        spatial::SpatialChip test_chip(argv[1]);
        test_chip.run();
    }
    catch (const std::string msg) {
        std::cerr << msg << std::endl;
        return -1;
    }
    return 0;
}