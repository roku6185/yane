#ifndef _MAPPER9_H_
#define _MAPPER9_H_

#include "cartridge.h"

#define MMC2_MIRRORING 0x01
#define MMC2_LATCH_INIT 0xFE
#define MMC2_LATCHES  2

class Mapper9 : public Cartridge
{
public:
  Mapper9(boost::shared_ptr<iNes> rom);
  void reset();
  std::string getName() { return "MMC2"; };
  bool writePrgRom(unsigned short address, unsigned char value);
  bool readChrRom(unsigned short address, unsigned char &value);
  enum Mirroring getMirroring() { return mirroring; };

private:
  enum Mirroring mirroring;
  unsigned char latch;
  unsigned char latchData[MMC2_LATCHES];
};

#endif
