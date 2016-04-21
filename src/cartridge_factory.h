#ifndef _CARTRIDGE_FACTORY_H_
#define _CARTRIDGE_FACTORY_H_

#include "ines.h"
#include "cartridge.h"

class cpu;

class CartridgeFactory
{
public:
  static Cartridge* create(iNes *rom, cpu *cpu);

private:
  CartridgeFactory();
};

#endif
