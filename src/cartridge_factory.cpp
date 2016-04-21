#include "cartridge_factory.h"
#include "yane_exception.h"
#include "mappers/mapper0.h"
#include "mappers/mapper1.h"
#include "mappers/mapper2.h"
#include "mappers/mapper3.h"
#include "mappers/mapper4.h"
#include "mappers/mapper7.h"
#include "mappers/mapper9.h"
#include "mappers/mapper66.h"

Cartridge* CartridgeFactory::create(iNes *rom, cpu *cpu)
{
  int mapperId = rom->getMapperId();

  switch (mapperId)
  {
    case 0:
      return new Mapper0(rom);
      break;

    case 1:
      return new Mapper1(rom);
      break;

    case 2:
      return new Mapper2(rom);
      break;

    case 3:
      return new Mapper3(rom);
      break;

    case 4:
      return new Mapper4(rom, cpu);
      break;

    case 7:
      return new Mapper7(rom);
      break;

    case 9:
      return new Mapper9(rom);
      break;

    case 66:
      return new Mapper66(rom);
      break;

    default:
      throw MapperNotSupportedException(mapperId);
      break;
  }
}
