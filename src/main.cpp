#include "spatial_chip.hpp"

int main() {
    // try {
    spatial::SpatialChip test_chip("runfiles/spatial_spec");
    test_chip.run();
    // }
    // catch (const char* msg) {
    //     std::cerr << msg << std::endl;
    //     return -1;
    // }
    return 0;
}