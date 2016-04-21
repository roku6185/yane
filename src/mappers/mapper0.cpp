#include "mappers/mapper0.h"

using namespace std;

Mapper0::Mapper0(iNes *rom) : Cartridge(rom) {}

void Mapper0::reset()
{
  switch (_rom->getPrgRomCount())
  {
    case 2:
      mapPrg16Kb(PRG_FIRST_BANK_ADDR, 0);
      mapPrg16Kb(PRG_SECOND_BANK_ADDR, 0);
      break;

    case 4:
      mapPrg32Kb(0);
      break;
  }

  switch (_rom->getChrRomCount())
  {
    case 0:
    case 8:
      mapChr8Kb(0);
      break;
  }
}

enum Mirroring Mapper0::getMirroring()
{
  if (_rom->hasHorizontalMirroring())
  {
    return Mirroring::Horizontal;
  }
  else if (_rom->hasVerticalMirroring())
  {
    return Mirroring::Vertical;
  }
  else if (_rom->hasFourScreenMirroring())
  {
    return Mirroring::FourScreen;
  }

  return Mirroring::SingleScreen;
}
