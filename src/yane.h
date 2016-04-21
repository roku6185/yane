#ifndef _YANE_H_
#define _YANE_H_

#include <string>
#include <boost/assert.hpp>
#include <SDL/SDL.h>

class Cartridge;
class Renderer;
class iNes;
class cpu;
class ppu;
class Controller;


class Yane
{
public:
  Yane();
  ~Yane();
  void init(std::string filename);
  void run();
  void stop();
  void reset();
  void handleUserInput(SDL_Event event);

private:
  Cartridge *_mapper;
  Renderer *_renderer;
  iNes *_rom;
  cpu *_cpu;
  ppu *_ppu;
  Controller *_controller;
  bool isRunning;
  bool isReset;
};

#endif
