#ifndef _CONTROLLER_H_
#define _CONTROLLER_H_

#include <thread>

class Yane;

class Controller
{
public:
  Controller(Yane *yane);
  ~Controller();
  void start();
  void stop();
  void run();

private:
  std::thread t;
  Yane *_yane;
  SDL_Event event;
  bool isRunning;
};

#endif
