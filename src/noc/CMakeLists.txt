project(noc)

# parser
file(GLOB parser parser/*.a)
# sub-modules
file(GLOB sub_modules allocators arbiters focus monitor networks power routers)
# cpp files
file(GLOB_RECURSE cpp_files *.cpp)

add_library(${PROJECT_NAME} ${cpp_files})

# SPATIAL: enables the interface, SpatialTrafficManager, to interact with Spatial Cores
target_compile_options(${PROJECT_NAME} PUBLIC -g -DSPATIAL)

# includes
get_filename_component(PARENT_DIR ${PROJECT_SOURCE_DIR} DIRECTORY)
target_include_directories(${PROJECT_NAME} PUBLIC ${PARENT_DIR}/include)
target_include_directories(${PROJECT_NAME} PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_include_directories(${PROJECT_NAME} PUBLIC ${sub_modules})

target_link_libraries(${PROJECT_NAME} ${parser})