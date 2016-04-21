#include <iostream>
#include <stdlib.h>
#include <sstream>

#include "yane.h"
#include "cartridge.h"
#include "cartridge_factory.h"
#include "renderer_factory.h"
#include "cpu.h"
#include "ppu.h"
#include "controller.h"
#include "yane_exception.h"
#include "config.h"
#include "ines.h"
#include "utils.h"


Yane::Yane() : isRunning(false), isReset(false)
{
  _cpu = new cpu();
  _ppu = new ppu();
  _controller = new Controller(this);
}

Yane::~Yane()
{
  delete _cpu;
  delete _ppu;
  delete _rom;
  delete _controller;
}

void Yane::init(std::string filename)
{
  // Initialize rom
  _rom = new iNes(filename);
  _rom->init();

  if (!_rom->isValid())
  {
    throw InvalidRomException(filename);
  }

  // Initialize cartridge, cpu and ppu
  try
  {
    _mapper = CartridgeFactory::create(_rom, _cpu);
    _renderer = RendererFactory::create(Config::instance().renderer);

    // Display rom headers and exit
    if (Config::instance().showRomInfo)
    {
      std::cout << _mapper->toString();
      exit(0);
    }

    _cpu->init(_mapper, _ppu);
    _ppu->init(_mapper, _renderer, _cpu);
  }
  catch (YaneException e)
  {
    std::cerr << "Error: " << e.what() << std::endl;
    exit(1);
  }
}

void Yane::stop()
{
  isRunning = false;
}

void Yane::reset()
{
  isReset = true;
}

void Yane::handleUserInput(SDL_Event event)
{
  _cpu->updateControllerKeyStatus(event);
}

void Yane::run()
{
  isRunning = true;

  // Start emulate components
  _ppu->start();
  _cpu->start();
  _controller->start();

  while (isRunning)
  {
    // Check status of a blargh test?
    if (Config::instance().isBlarghTest && _cpu->checkTestStatus())
    {
      isRunning = false;
      break;
    }
    else if (isReset)
    {
      _ppu->reset();
      _cpu->reset();
      isReset = false;
    }

    // Execute instruction
    try
    {
      unsigned short cycles = _cpu->executeOpcode();
      _ppu->execute(cycles);
    }
    catch (InvalidOpcodeException e)
    {
      std::cerr << "Error: " << e.what() << std::endl;
      isRunning = false;
    }
  }

  // Shut down
  std::cout << "Yane shutting down..." << std::endl;
  _controller->stop();
  _cpu->stop();
  _ppu->stop();
}
