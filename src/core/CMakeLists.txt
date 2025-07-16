project(core)

file(GLOB_RECURSE cpp_files *.cpp)
file(GLOB_RECURSE cc_files *.cc)
add_library(${PROJECT_NAME} ${cpp_files} ${cc_files})

target_compile_options(${PROJECT_NAME} PUBLIC -g)

# include dir
get_filename_component(PARENT_DIR ${PROJECT_SOURCE_DIR} DIRECTORY)
target_include_directories(${PROJECT_NAME} PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_include_directories(${PROJECT_NAME} PUBLIC ${PARENT_DIR}/include)
