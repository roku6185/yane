// Based on MMC3 code from Halfnes
#include "mappers/mapper4.h"
#include "cpu.h"

using namespace std;

Mapper4::Mapper4(
  boost::shared_ptr<iNes> rom,
  boost::shared_ptr<cpu> cpu)
:
  Cartridge(rom),
  _cpu(cpu)
{}

void Mapper4::reset()
{
  irqCounter = 0;
  irqCounterReload = 0;
  irqEnabled = false;
  irqReload = false;
  interrupted = false;

  bankMode = 0;
  prgMode = 0;
  chrMode = 0;
  prgBank = 0;
  memset(chrBanks, 0, sizeof(chrBanks));

  mapPrg16Kb(PRG_FIRST_BANK_ADDR, 0);
  mapPrg16Kb(PRG_SECOND_BANK_ADDR, getLastPrgBank(PRG_BANKS_FOR_16KB));
  mapChr8Kb(0);

  setupPrg();
}

bool Mapper4::writePrgRom(unsigned short address, unsigned char value)
{
  // Odd addresses
  if (address & 0x1)
  {
    if (address >= 0x8000 && address <= 0x9FFF)
    {
      if (bankMode <= 5)
      {
        chrBanks[bankMode] = value;
        setupChr();
      }
      else if (bankMode == 6)
      {
        prgBank = value;
        setupPrg();
      }
      else if (bankMode == 7)
      {
        mapPrg8Kb(0xA000, value);
      }
    }
    else if (address >= 0xA000 && address <= 0xBFFF)
    {
      // TODO: PRG RAM write protection
    }
    else if (address >= 0xC000 && address <= 0xDFFF)
    {
      irqReload = true;
    }
    else if (address >= 0xE000 && address <= 0xFFFF)
    {
      irqEnabled = true;
    }
  }
  // Even addresses
  else
  {
    if (address >= 0x8000 && address <= 0x9FFF)
    {
      bankMode = value & MMC3_SELECT_BANK;
      prgMode = value & MMC3_PRG_MODE;
      chrMode = value & MMC3_CHR_MODE;

      setupChr();
      setupPrg();
    }
    else if (address >= 0xA000 && address <= 0xBFFF)
    {
      if (value & MMC3_MIRRORING)
      {
        mirroring = Mirroring::Horizontal;
      }
      else
      {
        mirroring = Mirroring::Vertical;
      }
    }
    else if (address >= 0xC000 && address <= 0xDFFF)
    {
      irqCounterReload = value;
      irqReload = true;
    }
    else if (address >= 0xE000 && address <= 0xFFFF)
    {
      if (interrupted)
      {
        _cpu->dequeueInterrupt();
      }

      interrupted = false;
      irqEnabled = false;
      irqCounter = irqCounterReload;
    }
  }

  return true;
}

void Mapper4::irqTick()
{
  if (irqReload)
  {
    irqReload = false;
    irqCounter = irqCounterReload;
  }

  if (irqCounter-- <= 0)
  {
    if (irqCounterReload == 0)
    {
      return;
    }

    if (irqEnabled)
    {
      if (!interrupted)
      {
        _cpu->enqueueInterrupt(Interrupt::Irq);
        interrupted = true;
      }
    }

    irqCounter = irqCounterReload;
  }
}

void Mapper4::setupChr()
{
  // 4 1KB banks @ 0000-0fff, 2 2KB banks @ 1000-1fff
  if (chrMode)
  {
    mapChr1Kb(0x0000, chrBanks[2]);
    mapChr1Kb(0x0400, chrBanks[3]);
    mapChr1Kb(0x0800, chrBanks[4]);
    mapChr1Kb(0x0C00, chrBanks[5]);
    mapChr2Kb(0x1000, chrBanks[0] >> 1);
    mapChr2Kb(0x1800, chrBanks[1] >> 1);
  }
  // 2 2KB banks @ 0000-0fff, 4 1KB banks in 1000-1fff
  else
  {
    mapChr1Kb(0x1000, chrBanks[2]);
    mapChr1Kb(0x1400, chrBanks[3]);
    mapChr1Kb(0x1800, chrBanks[4]);
    mapChr1Kb(0x1C00, chrBanks[5]);
    mapChr2Kb(0x0000, chrBanks[0] >> 1);
    mapChr2Kb(0x0800, chrBanks[1] >> 1);
  }
}

void Mapper4::setupPrg()
{
  // Map 8000-9fff to last bank, c000 to dfff to selected bank
  if (prgMode)
  {
    mapPrg8Kb(0x8000, getLastPrgBank(PRG_BANKS_FOR_8KB) - 1);
    mapPrg8Kb(0xC000, prgBank);
  }
  // Map c000-dfff to last bank, 8000-9fff to selected bank
  else
  {
    mapPrg8Kb(0xC000, getLastPrgBank(PRG_BANKS_FOR_8KB) - 1);
    mapPrg8Kb(0x8000, prgBank);
  }
}
