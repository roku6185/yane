#include "mappers/mapper9.h"
#include <string.h>

using namespace std;

Mapper9::Mapper9(iNes *rom) : Cartridge(rom) {}

void Mapper9::reset()
{
  latch = MMC2_LATCH_INIT;
  memset(latchData, 0, sizeof(latchData));

  mapChr4Kb(CHR_FIRST_BANK_ADDR, 0);
  mapChr4Kb(CHR_SECOND_BANK_ADDR, 0);

  unsigned char lastBank = getLastPrgBank(PRG_BANKS_FOR_8KB);
  mapPrg8Kb(PRG_FIRST_BANK_ADDR, 0);
  mapPrg8Kb(0xA000, lastBank - 2);
  mapPrg8Kb(0xC000, lastBank - 1);
  mapPrg8Kb(0xE000, lastBank);
}

bool Mapper9::writePrgRom(unsigned short address, unsigned char value)
{
  unsigned short addr = (address >> 12) & 0x07;

  switch (addr)
  {
  // 0xA000-0xAFFF
  case 2:
    mapPrg8Kb(PRG_FIRST_BANK_ADDR, value & 0x0F);
    return true;

  // 0xB000-0xCFFF
  case 3:
  case 4:
    mapChr4Kb(CHR_FIRST_BANK_ADDR, value);
    return true;

  // 0xD000-0xEFFF
  case 5: // 0xD000-0xDFFF
  case 6: // 0xE000-0xEFFF
    latchData[addr - 5] = value;

    if (latch == 0xFD)
    {
      mapChr4Kb(CHR_SECOND_BANK_ADDR, latchData[0]);
    }
    else if (latch == 0xFE)
    {
      mapChr4Kb(CHR_SECOND_BANK_ADDR, latchData[1]);
    }
    return true;

  // 0xF000-0xFFFF
  case 7:
    if (value & MMC2_MIRRORING)
    {
      mirroring = Mirroring::Horizontal;
    }
    else
    {
      mirroring = Mirroring::Vertical;
    }
    return true;
  }

  return false;
}

bool Mapper9::readChrRom(unsigned short address, unsigned char &value)
{
  unsigned char bank = address >> 12;

  if ((address >= 0x0FD0 && address <= 0x0FDF) ||
    (address >= 0x1FD0 && address <= 0x1FDF))
  {
    latch = 0xFD;
    mapChr4Kb(bank ? CHR_SECOND_BANK_ADDR : CHR_FIRST_BANK_ADDR, latchData[0]);
  }
  else if ((address >= 0x0FE0 && address <= 0x0FEF) ||
    (address >= 0x1FE0 && address <= 0x1FEF))
  {
    latch = 0xFE;
    mapChr4Kb(bank ? CHR_SECOND_BANK_ADDR : CHR_FIRST_BANK_ADDR, latchData[1]);
  }

  return Cartridge::readChrRom(address, value);
}
