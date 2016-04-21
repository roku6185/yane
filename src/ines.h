#ifndef _INES_H_
#define _INES_H_

#include <string>
#include <fstream>

#define PRG_ROM_SIZE      32768
#define PRG_BANK_SIZE      8192
#define PRG_BANKS        4
#define PRG_BANKS_FOR_8KB    1
#define PRG_BANKS_FOR_16KB   2
#define PRG_BANKS_FOR_32KB   4
#define PRG_FIRST_BANK_ADDR   0x8000
#define PRG_SECOND_BANK_ADDR  0xC000

#define CHR_ROM_SIZE      8192
#define CHR_BANK_SIZE      1024
#define CHR_BANKS        8
#define CHR_BANKS_FOR_1KB    1
#define CHR_BANKS_FOR_2KB    2
#define CHR_BANKS_FOR_4KB    4
#define CHR_BANKS_FOR_8KB    8
#define CHR_FIRST_BANK_ADDR   0x0000
#define CHR_SECOND_BANK_ADDR  0x1000

#define TRAINER_SIZE      512
#define INES_PREAMBLE      "NES"
#define INES_DELIMITER      0x1A


typedef struct
{
  unsigned char nes[3];
  unsigned char delimiter;
  unsigned char prgRomPageCount;
  unsigned char chrRomPageCount;
  unsigned char controlByte1;
  unsigned char controlByte2;
  unsigned char empty[8];
} nesHeader;

typedef struct
{
  unsigned char data[PRG_BANK_SIZE];
} prgRomPage;

typedef struct
{
  unsigned char data[CHR_BANK_SIZE];
} chrRomPage;

class iNes
{
public:
  iNes(const std::string filename);
  bool init();
  bool isValid();
  int getPrgRomCount() { return prgPageCount; };
  int getChrRomCount() { return chrPageCount; };
  int getMapperId();
  bool hasHorizontalMirroring();
  bool hasVerticalMirroring();
  bool hasSRAM();
  bool hasTrainer();
  bool hasFourScreenMirroring();
  prgRomPage* getPrgRomPage(int page);
  chrRomPage* getChrRomPage(int page);

private:
  const std::string getPreamble();

  bool _isValid;
  std::string filename;
  nesHeader header;
  int prgPageCount;
  int chrPageCount;
  prgRomPage* prgPages;
  chrRomPage* chrPages;
};

#endif
