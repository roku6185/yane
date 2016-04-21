#ifndef _MAPPER3_H_
#define _MAPPER3_H_

#include "mapper0.h"

class Mapper3 : public Mapper0
{
public:
  Mapper3(iNes *rom);
  void reset();
  std::string getName() { return "CNROM"; };
  bool writePrgRom(unsigned short address, unsigned char value);
};

#endif
