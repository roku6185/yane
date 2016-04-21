#include "mappers/mapper7.h"

using namespace std;

Mapper7::Mapper7(iNes *rom) : Cartridge(rom) {}

void Mapper7::reset()
{
  mapPrg32Kb(0);
  mapChr8Kb(0);
}

bool Mapper7::writePrgRom(unsigned short address, unsigned char value)
{
  mapPrg32Kb(value & 0x0F);

  if (value & AOROM_MIRRORING)
  {
    mirroring = Mirroring::SingleScreenUpperBank;
  }
  else
  {
    mirroring = Mirroring::SingleScreenLowerBank;
  }

  return true;
}
