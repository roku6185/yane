#include "mappers/mapper66.h"

using namespace std;

Mapper66::Mapper66(iNes *rom) : Mapper0(rom) {}

void Mapper66::reset()
{
  Mapper0::reset();
  mapPrg32Kb(0);
  mapChr8Kb(0);
}

bool Mapper66::writePrgRom(unsigned short address, unsigned char value)
{
  mapPrg32Kb((value >> 4) & 0x03);
  mapChr8Kb(value & 0x03);
  return true;
}
