#ifndef _MAPPER1_H_
#define _MAPPER1_H_

#include "cartridge.h"

#define MMC1_MAX_COUNTER     5
#define MMC1_INTERNAL_REGISTERS  4

#define MMC1_REG0_MIRRORING     0x03
#define MMC1_REG0_PRGROM_BANK_MODE 0x0C
#define MMC1_REG0_CHRROM_BANK_MODE 0x10

class Mapper1 : public Cartridge
{
public:
  Mapper1(boost::shared_ptr<iNes> rom);
  ~Mapper1() {}
  void reset();
  bool writePrgRom(unsigned short address, unsigned char value);
  enum Mirroring getMirroring() { return mirroring; };
  std::string getName() { return "MMC1"; };

private:
  enum Mirroring mirroring;
  unsigned char reg[MMC1_INTERNAL_REGISTERS];
  unsigned char shiftRegister;
  unsigned char shiftCounter;

  bool prg_switch_8000;
  bool prg_fix_8000_switch_c000;
  bool prg_fix_c000_switch_8000;
  bool chr_switch_two_4kb;

  void handleRegister(unsigned char registerNumber, unsigned char value);
  void funcControl();
  void funcChrBank0();
  void funcChrBank1();
  void funcPrgBank();
};

#endif
