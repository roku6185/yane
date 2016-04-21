#ifndef _MAPPER0_H_
#define _MAPPER0_H_

#include "cartridge.h"

class Mapper0 : public Cartridge
{
public:
  Mapper0(iNes *rom);
  ~Mapper0() {}
  virtual void reset();
  enum Mirroring getMirroring();
  std::string getName() { return "NROM"; };
};

#endif
