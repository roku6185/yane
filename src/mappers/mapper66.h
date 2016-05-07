#ifndef _MAPPER66_H_
#define _MAPPER66_H_

#include "mapper0.h"

class Mapper66 : public Mapper0
{
public:
  Mapper66(boost::shared_ptr<iNes> rom);
  void reset();
  std::string getName() { return "GNROM"; };
  bool writePrgRom(unsigned short address, unsigned char value);
};

#endif
