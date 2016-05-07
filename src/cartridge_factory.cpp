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
#include <boost/make_shared.hpp>

boost::shared_ptr<Cartridge> CartridgeFactory::create(
  boost::shared_ptr<iNes> rom,
  boost::shared_ptr<cpu> cpu)
{
  int mapperId = rom->getMapperId();

  switch (mapperId)
  {
    case 0:
      return boost::make_shared<Mapper0>(rom);
      break;

    case 1:
      return boost::make_shared<Mapper1>(rom);
      break;

    case 2:
      return boost::make_shared<Mapper2>(rom);
      break;

    case 3:
      return boost::make_shared<Mapper3>(rom);
      break;

    case 4:
      return boost::make_shared<Mapper4>(rom, cpu);
      break;

    case 7:
      return boost::make_shared<Mapper7>(rom);
      break;

    case 9:
      return boost::make_shared<Mapper9>(rom);
      break;

    case 66:
      return boost::make_shared<Mapper66>(rom);
      break;

    default:
      throw MapperNotSupportedException(mapperId);
      break;
  }
}
