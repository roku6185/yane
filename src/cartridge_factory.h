#ifndef _CARTRIDGE_FACTORY_H_
#define _CARTRIDGE_FACTORY_H_

#include "ines.h"
#include "cartridge.h"
#include <boost/shared_ptr.hpp>

class cpu;

class CartridgeFactory
{
public:
  static boost::shared_ptr<Cartridge> create(
    boost::shared_ptr<iNes> rom,
    boost::shared_ptr<cpu> cpu);

private:
  CartridgeFactory();
};

#endif
