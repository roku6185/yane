#ifndef _CARTRIDGE_H_
#define _CARTRIDGE_H_

#include <boost/shared_ptr.hpp>
#include <strings.h>
#include "ines.h"

using namespace std;

enum Mirroring { Horizontal, Vertical, SingleScreen, SingleScreenLowerBank, SingleScreenUpperBank, FourScreen };

class Cartridge
{
public:
  Cartridge(boost::shared_ptr<iNes> rom);
  virtual ~Cartridge() {};
  virtual bool readPrgRom(unsigned short address, unsigned char &value);
  virtual bool writePrgRom(unsigned short address, unsigned char value);
  virtual bool readChrRom(unsigned short address, unsigned char &value);
  virtual bool writeChrRom(unsigned short address, unsigned char value);
  virtual void irqTick() {};
  virtual void reset() = 0;
  virtual enum Mirroring getMirroring() = 0;
  virtual string getName() = 0;
  const string toString();

protected:
  boost::shared_ptr<iNes> _rom;
  void mapPrg32Kb(unsigned char targetBankIndex);
  void mapPrg16Kb(unsigned short address, unsigned char targetBankIndex);
  void mapPrg8Kb(unsigned short address, unsigned char targetBankIndex);
  void mapChr8Kb(unsigned char targetBankIndex);
  void mapChr4Kb(unsigned short address, unsigned char targetBankIndex);
  void mapChr2Kb(unsigned short address, unsigned char targetBankIndex);
  void mapChr1Kb(unsigned short address, unsigned char targetBankIndex);
  unsigned char getLastPrgBank(unsigned char banks) { return _rom->getPrgRomCount() / banks - 1; };

private:
  void mapPrg(unsigned short address, unsigned char targetBankIndex, unsigned char banksToMap);
  void mapChr(unsigned short address, unsigned char targetBankIndex, unsigned char banksToMap);
  unsigned char prgMap[PRG_BANKS];  // # 8 Kb pages => 4*8 Kb = 32 Kb PRG-ROM
  unsigned char chrMap[CHR_BANKS];  // # 1 Kb pages => 1*8 Kb = 8 Kb CHR-ROM
};

#endif
