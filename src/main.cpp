#include <iostream>
#include <stdlib.h>
#include <signal.h>
#include <SDL/SDL.h>
#include <boost/program_options.hpp>
#include <boost/algorithm/string.hpp>

#include "config.h"
#include "yane.h"
#include "yane_exception.h"


using namespace std;

int main(int argc, char **argv)
{
  atexit(SDL_Quit);

  boost::program_options::variables_map vm;
  boost::program_options::options_description desc("Usage: yane [options] --rom <ROM_FILE>");
  desc.add_options()
    ("version", "Show version")
    ("help", "Show this help message")
    ("rom", boost::program_options::value<string>(), "What iNES rom file to use")
    ("nes-test", "Treat rom as nestest (adjusted reset vector)")
    ("blargh-test", "Treat rom as blargh test (polls 0x6000 for results)")
    ("log", "Enable logging of instructions (nestest format)")
    ("rom-info", "Display rom headers")
    ("fullscreen,f", "Use fullscreen mode")
    ("renderer,r", boost::program_options::value<string>()->default_value("sdl"), "Use another render engine (default: SDL)")
  ;

  try
  {
    boost::program_options::store(boost::program_options::parse_command_line(argc, argv, desc), vm);
    boost::program_options::notify(vm);

    // Check options that have early exit
    if (vm.count("version"))
    {
      cout << "Yane " << Config::instance().getVersion() << endl;
      cout << "Yet Another Nes Emulator" << endl;
      exit(0);
    }
    else if (vm.count("help"))
    {
      cout << desc << endl;
      exit(0);
    }
    else if (!vm.count("rom"))
    {
      cout << desc << endl;
      cerr << "Error: No rom file was specified." << endl;
      exit(1);
    }

    // Pass program options to Config singleton
    Config::instance().showRomInfo = vm.count("rom-info");
    Config::instance().doInstructionLogging = vm.count("log");
    Config::instance().isNesTest = vm.count("nes-test");
    Config::instance().isBlarghTest = vm.count("blargh-test");
    Config::instance().isFullscreen = vm.count("fullscreen");

    std::string renderer = vm["renderer"].as<string>();
    boost::algorithm::to_lower(renderer);
    Config::instance().renderer = renderer;
  }
  catch (exception& e)
  {
    cout << desc << endl;
    cerr << "Error: " << e.what() << endl;
    exit(1);
  }

  // Start emulator
  Yane yane;

  try
  {
    yane.init(vm["rom"].as<string>());
  }
  catch (YaneException e)
  {
    std::cerr << "Error: " << e.what() << std::endl;
    exit(1);
  }

  yane.run();

  return 0;
}
