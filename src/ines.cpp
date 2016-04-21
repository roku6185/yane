#include <iostream>
#include <string.h>
#include <sstream>

#include "ines.h"

using namespace std;

iNes::iNes(const string _filename)
{
  filename = _filename;
  prgPageCount = 0;
  chrPageCount = 0;
}

bool iNes::init()
{
  ifstream file(filename.c_str(), ios::in | ios::binary);

  if (file.is_open())
  {
    // Read header
    file.seekg(0, ios::beg);
    file.read((char*)&header, sizeof(header));
    prgPageCount = header.prgRomPageCount << 1;  // Convert 16 Kb PRG-ROM banks to 8 Kb
    chrPageCount = header.chrRomPageCount << 3;  // Convert 8 Kb CHR-ROM banks to 1 Kb

    // Read trainer
    if (hasTrainer())
    {
      unsigned char trainer[TRAINER_SIZE];
      file.read((char*)&trainer, sizeof(trainer));
    }

    // Read PRG-ROM pages
    prgPages = new prgRomPage[prgPageCount];

    for (int i = 0; i < prgPageCount; i++)
    {
      file.read((char*)&prgPages[i], sizeof(prgRomPage));
    }

    // If only CHR-RAM is available, fake a CHR-ROM bank
    if (chrPageCount == 0)
    {
      chrPageCount = CHR_BANKS;
      chrPages = new chrRomPage[CHR_BANKS];
    }
    // Read CHR-ROM pages
    else
    {
      chrPages = new chrRomPage[chrPageCount];

      for (int i = 0; i < chrPageCount; i++)
      {
        file.read((char*)&chrPages[i], sizeof(chrRomPage));
      }
    }

    file.close();

    _isValid = true;
  }
  else
  {
    _isValid = false;
  }

  return _isValid;
}

bool iNes::isValid()
{
  bool ret = _isValid;

  ret &= (getPreamble() == INES_PREAMBLE);
  ret &= (header.delimiter == INES_DELIMITER);

  return ret;
}

const string iNes::getPreamble()
{
  char tmp[4];
  memset(tmp, 0, sizeof(tmp));
  memcpy(tmp, header.nes, sizeof(header.nes));
  return string(tmp);
}

int iNes::getMapperId()
{
  return (header.controlByte2 & 0xF0) | (header.controlByte1 >> 4);
}

bool iNes::hasHorizontalMirroring()
{
  return !(header.controlByte1 & 0x1);
}

bool iNes::hasVerticalMirroring()
{
  return header.controlByte1 & 0x1;
}

bool iNes::hasSRAM()
{
  return header.controlByte1 & 0x2;
}

bool iNes::hasTrainer()
{
  return header.controlByte1 & 0x4;
}

bool iNes::hasFourScreenMirroring()
{
  return header.controlByte1 & 0x8;
}

prgRomPage* iNes::getPrgRomPage(int page)
{
  return &prgPages[page];
}

chrRomPage* iNes::getChrRomPage(int page)
{
  return &chrPages[page];
}
