project(spatialsim)

# compiling flags: 
# MULTICAST: enable tree-based multicasts
# DUMP_WAITING_FLITS: dump total number of queueing flits from 10k to 20k cycle
# DUMP_NODE_STATE: dump node states at each cycle

# add_compile_options(-O0 -g -DMULTICAST)
# add_compile_options(-O3 -g -fPIC)
add_compile_options(-O3 -g -fPIC -DMULTICAST -DRT_DETAIL_LOG)

add_subdirectory(core)
add_subdirectory(noc)

# build lib
file(GLOB lib_files spatial_chip.cpp core_array.cpp noc.cpp)
add_library(${PROJECT_NAME}_lib ${lib_files})
# include
target_include_directories(${PROJECT_NAME}_lib PUBLIC 
    ${PARENT_DIRECTORY}
    ${PROJECT_SOURCE_DIR}/include
)
# link sub-projects
target_link_libraries(${PROJECT_NAME}_lib PUBLIC
    noc core ${parser}
)

# executable
file(GLOB cpp_files main.cpp)
add_executable(${PROJECT_NAME} ${cpp_files})
target_link_libraries(${PROJECT_NAME} PUBLIC ${PROJECT_NAME}_lib)

# wrapper
pybind11_add_module(simulator wrapper.cpp)
target_link_libraries(simulator PUBLIC noc core ${PROJECT_NAME}_lib)
