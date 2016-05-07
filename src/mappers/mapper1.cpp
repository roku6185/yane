// Based on MMC1 code from fakenes-0.5.9-beta3
#include "mappers/mapper1.h"

using namespace std;

Mapper1::Mapper1(boost::shared_ptr<iNes> rom)
:
  Cartridge(rom)
{}

void Mapper1::reset()
{
  mapPrg16Kb(PRG_FIRST_BANK_ADDR, 0);
  mapPrg16Kb(PRG_SECOND_BANK_ADDR, getLastPrgBank(PRG_BANKS_FOR_16KB));
  mapChr8Kb(0);

  shiftRegister = 0;
  shiftCounter = 0;
  reg[0] = reg[1] = reg[2] = reg[3] = 0;
}

bool Mapper1::writePrgRom(unsigned short address, unsigned char value)
{
  if (address < PRG_FIRST_BANK_ADDR)
  {
    return false;
  }

  unsigned short registerNumber = (address & 0x6000) >> 13;

  if (value & 0x80)
  {
    shiftRegister = 0;
    shiftCounter = 0;
    handleRegister(0, reg[0] | 0x0C);
  }
  else
  {
    shiftRegister |= (value & 0x01) << shiftCounter;

    if (++shiftCounter == MMC1_MAX_COUNTER)
    {
      handleRegister(registerNumber, shiftRegister);
      shiftRegister = 0;
      shiftCounter = 0;
    }
  }

  return true;
}

void Mapper1::handleRegister(unsigned char registerNumber, unsigned char value)
{
  reg[registerNumber] = value;

  switch (registerNumber)
  {
  // 0x8000-0x9FFF
  case 0:
    funcControl();
    break;

  // 0xA000-0xBFFF
  case 1:
    funcChrBank0();
    break;

  // 0xC000-0xDFFF
  case 2:
    funcChrBank1();
    break;

  // 0xE000-0xFFFF
  case 3:
    funcPrgBank();
    break;
  }
}

void Mapper1::funcControl()
{
   switch (reg[0] & MMC1_REG0_MIRRORING)
   {
   case 0:
    mirroring = Mirroring::SingleScreenLowerBank;
    break;

   case 1:
    mirroring = Mirroring::SingleScreenUpperBank;
    break;

   case 2:
    mirroring = Mirroring::Vertical;
    break;

   case 3:
    mirroring = Mirroring::Horizontal;
    break;
   }

   funcPrgBank();
   funcChrBank0();
   funcChrBank1();
}

void Mapper1::funcChrBank0()
{
  if (reg[0] & MMC1_REG0_CHRROM_BANK_MODE)
  {
    // NOT WORKING?!
    // return;
    mapChr4Kb(CHR_FIRST_BANK_ADDR, reg[1] & 0x1F);
  }
  else
  {
    mapChr8Kb(reg[1] & 0x1E);
  }
}

void Mapper1::funcChrBank1()
{
  if (reg[0] & MMC1_REG0_CHRROM_BANK_MODE)
  {
    mapChr4Kb(CHR_SECOND_BANK_ADDR, reg[2] & 0x1F);
  }
}

void Mapper1::funcPrgBank()
{
  switch ((reg[0] & MMC1_REG0_PRGROM_BANK_MODE) >> 2)
  {
  case 0:
  case 1:
    mapPrg32Kb(reg[3] & 0x0E);
    break;

  case 2:
    mapPrg16Kb(PRG_FIRST_BANK_ADDR, 0);
    mapPrg16Kb(PRG_SECOND_BANK_ADDR, reg[3] & 0x0F);
    break;

  case 3:
    mapPrg16Kb(PRG_FIRST_BANK_ADDR, reg[3] & 0x0F);
    mapPrg16Kb(PRG_SECOND_BANK_ADDR, getLastPrgBank(PRG_BANKS_FOR_16KB));
    break;
  }
}
