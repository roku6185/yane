#include "mappers/mapper2.h"

using namespace std;

Mapper2::Mapper2(iNes *rom) : Mapper0(rom) {}

void Mapper2::reset()
{
  mapPrg16Kb(PRG_FIRST_BANK_ADDR, 0);
  mapPrg16Kb(PRG_SECOND_BANK_ADDR, getLastPrgBank(PRG_BANKS_FOR_16KB));
  mapChr8Kb(0);
}

bool Mapper2::writePrgRom(unsigned short address, unsigned char value)
{
  mapPrg16Kb(PRG_FIRST_BANK_ADDR, value & 0x0F);
  return true;
}
