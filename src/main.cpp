#include "spatial_chip.hpp"

int main(int argc, char **argv) {
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