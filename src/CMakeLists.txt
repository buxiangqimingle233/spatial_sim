project(spatialsim)

# compiling flags: 
# MULTICAST: enable tree-based multicasts
# DUMP_WAITING_FLITS: dump total number of queueing flits from 10k to 20k cycle
# DUMP_NODE_STATE: dump node states at each cycle

# add_compile_options(-O0 -g -DMULTICAST)
add_compile_options(-O0 -g)

add_subdirectory(core)
add_subdirectory(noc)

# build spatialsim
file(GLOB cpp_files *.cpp)
add_executable(${PROJECT_NAME} ${cpp_files})

# include
target_include_directories(${PROJECT_NAME} PUBLIC ${PROJECT_SOURCE_DIR}/include)

# link noc
target_link_libraries(${PROJECT_NAME} PUBLIC noc)
# link core
target_link_libraries(${PROJECT_NAME} PUBLIC core)
# link parser
file(GLOB parser noc/parser/*.a)
target_link_libraries(${PROJECT_NAME} PUBLIC ${parser})
