#ifndef _MAPPER2_H_
#define _MAPPER2_H_

#include "mapper0.h"

class Mapper2 : public Mapper0
{
public:
  Mapper2(iNes *rom);
  ~Mapper2() {}
  virtual void reset();
  std::string getName() { return "UNROM"; };
  bool writePrgRom(unsigned short address, unsigned char value);
};

#endif
