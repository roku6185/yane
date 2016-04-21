#include "mappers/mapper3.h"

using namespace std;

Mapper3::Mapper3(iNes *rom) : Mapper0(rom) {}

void Mapper3::reset()
{
  Mapper0::reset();
  mapChr8Kb(0);
}

bool Mapper3::writePrgRom(unsigned short address, unsigned char value)
{
  mapChr8Kb(value & 0x0F);
  return true;
}
