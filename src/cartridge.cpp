#include <iostream>
#include <sstream>
#include <boost/assert.hpp>
#include "cartridge.h"
#include "ines.h"

using namespace std;

Cartridge::Cartridge(boost::shared_ptr<iNes> rom)
:
  _rom(rom)
{
  bzero(prgMap, sizeof(prgMap));
  bzero(chrMap, sizeof(chrMap));
}

bool Cartridge::readPrgRom(unsigned short address, unsigned char &value)
{
  size_t bankIndex = (address >> 13) & 0x03;
  BOOST_ASSERT_MSG(bankIndex < sizeof(prgMap), "Invalid PRG MAP address");
  value = _rom->getPrgRomPage(prgMap[bankIndex])->data[address & 0x1FFF];
  return true;
}

bool Cartridge::writePrgRom(unsigned short address, unsigned char value)
{
  size_t bankIndex = (address >> 13) & 0x03;
  BOOST_ASSERT_MSG(bankIndex < sizeof(prgMap), "Invalid PRG MAP address");
  _rom->getPrgRomPage(prgMap[bankIndex])->data[address & 0x1FFF] = value;
  return true;
}

bool Cartridge::readChrRom(unsigned short address, unsigned char &value)
{
  size_t bankIndex = (address >> 10) & 0x0F;
  BOOST_ASSERT_MSG(bankIndex < sizeof(chrMap), "Invalid CHR MAP address");
  value = _rom->getChrRomPage(chrMap[bankIndex])->data[address & 0x03FF];
  return true;
}

bool Cartridge::writeChrRom(unsigned short address, unsigned char value)
{
  size_t bankIndex = (address >> 10) & 0x0F;
  BOOST_ASSERT_MSG(bankIndex < sizeof(chrMap), "Invalid CHR MAP address");
  _rom->getChrRomPage(chrMap[bankIndex])->data[address & 0x03FF] = value;
  return true;
}

void Cartridge::mapPrg(unsigned short address, unsigned char targetBankIndex, unsigned char banksToMap)
{
  size_t bankIndex = (address >> 13) & 0x03;

  for (int i = 0; i < banksToMap; i++)
  {
    BOOST_ASSERT_MSG(bankIndex < sizeof(prgMap), "Invalid PRG MAP address");
    prgMap[bankIndex + i] = targetBankIndex + i;
  }
}

void Cartridge::mapPrg32Kb(unsigned char targetBankIndex)
{
  mapPrg(PRG_FIRST_BANK_ADDR, targetBankIndex * PRG_BANKS_FOR_32KB, PRG_BANKS_FOR_32KB);
}

void Cartridge::mapPrg16Kb(unsigned short address, unsigned char targetBankIndex)
{
  mapPrg(address, targetBankIndex * PRG_BANKS_FOR_16KB, PRG_BANKS_FOR_16KB);
}

void Cartridge::mapPrg8Kb(unsigned short address, unsigned char targetBankIndex)
{
  mapPrg(address, targetBankIndex, PRG_BANKS_FOR_8KB);
}

void Cartridge::mapChr(unsigned short address, unsigned char targetBankIndex, unsigned char banksToMap)
{
  size_t bankIndex = (address >> 10) & 0x0F;

  for (int i = 0; i < banksToMap; i++)
  {
    BOOST_ASSERT_MSG(bankIndex < sizeof(chrMap), "Invalid CHR MAP address");
    chrMap[bankIndex + i] = targetBankIndex + i;
  }
}

void Cartridge::mapChr8Kb(unsigned char targetBankIndex)
{
  mapChr(CHR_FIRST_BANK_ADDR, targetBankIndex * CHR_BANKS_FOR_8KB, CHR_BANKS_FOR_8KB);
}

void Cartridge::mapChr4Kb(unsigned short address, unsigned char targetBankIndex)
{
  mapChr(address, targetBankIndex * CHR_BANKS_FOR_4KB, CHR_BANKS_FOR_4KB);
}

void Cartridge::mapChr2Kb(unsigned short address, unsigned char targetBankIndex)
{
  mapChr(address, targetBankIndex * CHR_BANKS_FOR_2KB, CHR_BANKS_FOR_2KB);
}

void Cartridge::mapChr1Kb(unsigned short address, unsigned char targetBankIndex)
{
  mapChr(address, targetBankIndex, CHR_BANKS_FOR_1KB);
}

const string Cartridge::toString()
{
  stringstream ss;
  ss << "Mapper: " << getName() << " (#" << _rom->getMapperId() << ")" << endl;
  ss << "PRG-ROM: " << _rom->getPrgRomCount()*(PRG_BANK_SIZE/1024) << " KB" << endl;
  ss << "CHR-ROM: " << _rom->getChrRomCount()*(CHR_BANK_SIZE/1024) << " KB" << endl;
  ss << "Horizontal mirroring: " << (_rom->hasHorizontalMirroring() ? "Yes": "No") << endl;
  ss << "Vertical mirroring: " << (_rom->hasVerticalMirroring() ? "Yes" : "No") << endl;
  ss << "Four screen mirroring: " << (_rom->hasFourScreenMirroring() ? "Yes" : "No") << endl;
  ss << "SRAM: " << (_rom->hasSRAM() ? "Yes" : "No") << endl;
  ss << "Trainer: " << (_rom->hasTrainer() ? "Yes" : "No") << endl;
  return ss.str();
}
