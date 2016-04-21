#include <iostream>
#include <thread>
#include <SDL/SDL.h>

#include "controller.h"
#include "yane.h"


Controller::Controller(Yane *yane) : _yane(yane), isRunning(false) {}

Controller::~Controller() {}

void Controller::stop()
{
  t.join();
}

void Controller::start()
{
  isRunning = true;
  t = std::thread(&Controller::run, this);
}

void Controller::run()
{
  while (isRunning)
  {
    while (SDL_PollEvent(&event))
    {
      _yane->handleUserInput(event);

      if (event.type == SDL_QUIT ||
        (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_q)
      )
      {
        _yane->stop();
        isRunning = false;
        break;

      }
      else if ((event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_r))
      {
        _yane->reset();
      }
    }
  }
}
