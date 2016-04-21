#include <iostream>
#include <stdlib.h>
#include <sstream>

#include "config.h"


using namespace std;

std::string Config::getVersion()
{
  std::ostringstream stream;
  stream << YANE_VERSION_MAJOR << "." << YANE_VERSION_MINOR << "." << YANE_VERSION_REVISION;
  return stream.str();
}
