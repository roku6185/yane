cmake_minimum_required(VERSION 2.8.11)
project(YANE)

set(SRC
  cartridge.cpp
  cartridge_factory.cpp
  config.cpp
  controller.cpp
  cpu.cpp
  ines.cpp
  main.cpp
  ppu.cpp
  renderer_factory.cpp
  utils.cpp
  yane.cpp
)

set(MAPPERS
  mappers/mapper0.cpp
  mappers/mapper1.cpp
  mappers/mapper2.cpp
  mappers/mapper3.cpp
  mappers/mapper4.cpp
  mappers/mapper7.cpp
  mappers/mapper9.cpp
  mappers/mapper66.cpp
)


set(RENDERERS
  renderers/sdlrenderer.cpp
)

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

include_directories(${PROJECT_SOURCE_DIR})
add_compile_options(${COMPILER_FLAGS})
add_executable(yane ${SRC} ${MAPPERS} ${RENDERERS})
target_link_libraries(yane ${LIBS})
