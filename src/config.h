#ifndef _CONFIG_H_
#define _CONFIG_H_

#include <string>
#include <boost/assert.hpp>

#define YANE_VERSION_MAJOR   0
#define YANE_VERSION_MINOR   6
#define YANE_VERSION_REVISION  6


class Config
{
public:
  static Config& instance()
  {
    static Config instance;
    return instance;
  }

  std::string getVersion();

  bool showRomInfo;
  bool doInstructionLogging;
  bool isNesTest;
  bool isBlarghTest;
  bool isFullscreen;
  std::string renderer;

private:
  Config() :
    showRomInfo(false),
    doInstructionLogging(false),
    isNesTest(false),
    isBlarghTest(false),
    isFullscreen(false),
    renderer("")
  {}

  ~Config() {}

  Config(Config const & copy);
  Config& operator=(Config const & copy);
};

#endif
