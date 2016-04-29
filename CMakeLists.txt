cmake_minimum_required(VERSION 2.8.11)
project(YANE)

file(GLOB SRC src/*.cpp)
file(GLOB SRC_MAPPERS src/mappers/*.cpp)
file(GLOB SRC_RENDERERS src/renderers/*.cpp)

set(COMPILER_FLAGS
 -std=c++11
 -Wall
 -O3
)

set(LIBS
  pthread
  boost_program_options
  SDL
)

include_directories(${PROJECT_SOURCE_DIR}/src)
add_compile_options(${COMPILER_FLAGS})
set(EXECUTABLE_OUTPUT_PATH bin)
add_executable(yane ${SRC} ${SRC_MAPPERS} ${SRC_RENDERERS})
target_link_libraries(yane ${LIBS})