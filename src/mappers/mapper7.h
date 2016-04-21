#ifndef _MAPPER7_H_
#define _MAPPER7_H_

#include "mapper0.h"

#define AOROM_MIRRORING 0x10

class Mapper7 : public Cartridge
{
public:
  Mapper7(iNes *rom);
  void reset();
  std::string getName() { return "AOROM"; };
  bool writePrgRom(unsigned short address, unsigned char value);
  enum Mirroring getMirroring() { return mirroring; };

private:
  enum Mirroring mirroring;
};

#endif
