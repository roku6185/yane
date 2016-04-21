#ifndef _OPCODE_ENTRY_H_
#define _OPCODE_ENTRY_H_

#include <string>

class cpu;

typedef struct
{
  std::string name;
  void (cpu::*functionPtr)();
  unsigned short (cpu::*addressPtr)(unsigned char dummy);
  char bytes;
  char cycles;
  bool cycles_extra;
  bool skip_bytes;
  unsigned char dummy;
} opcode_entry;

#endif
