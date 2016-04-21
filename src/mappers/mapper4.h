#ifndef _MAPPER4_H_
#define _MAPPER4_H_

#include "cartridge.h"

class cpu;

#define MMC3_SELECT_BANK  0x07
#define MMC3_PRG_MODE    0x40
#define MMC3_CHR_MODE    0x80
#define MMC3_MIRRORING   0x01

#define MMC3_IRQ_RELOAD_INIT  0xFF

class Mapper4 : public Cartridge
{
public:
  Mapper4(iNes *rom, cpu *cpu);
  void reset();
  std::string getName() { return "MMC3"; };
  bool writePrgRom(unsigned short address, unsigned char value);
  enum Mirroring getMirroring() { return mirroring; };
  void irqTick();

private:
  cpu *_cpu;

  void setupChr();
  void setupPrg();

  enum Mirroring mirroring;
  unsigned char bankMode;
  unsigned char prgMode;
  unsigned char chrMode;
  unsigned char chrBanks[6];
  unsigned char prgBank;

  unsigned char irqCounter;
  unsigned char irqCounterReload;
  bool irqEnabled;
  bool irqReload;
  bool interrupted;
};

#endif
