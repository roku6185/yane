#ifndef _YANE_H_
#define _YANE_H_

#include <string>
#include <boost/make_shared.hpp>
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
  boost::shared_ptr<Cartridge> _mapper;
  boost::shared_ptr<Renderer> _renderer;
  boost::shared_ptr<iNes> _rom;
  boost::shared_ptr<cpu> _cpu;
  boost::shared_ptr<ppu> _ppu;
  boost::shared_ptr<Controller> _controller;
  bool isRunning;
  bool isReset;
};

#endif
