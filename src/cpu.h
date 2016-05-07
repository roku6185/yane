#ifndef _CPU_H_
#define _CPU_H_

#include <SDL/SDL.h>
#include <string>
#include <map>
#include <list>

#include "ppu.h"
#include "opcode_entry.h"

class cpu;
class Cartridge;


#define RAM_SIZE 65536
#define STACK_LOWER 0x0100
#define SP_INIT 0xFD
#define STATUS_INIT 0x34

#define INTERRUPT_NMI_LOW 0xFFFA
#define INTERRUPT_NMI_HIGH 0xFFFB
#define INTERRUPT_RESET_LOW 0xFFFC
#define INTERRUPT_RESET_HIGH 0xFFFD
#define INTERRUPT_IRQ_LOW 0xFFFE
#define INTERRUPT_IRQ_HIGH 0xFFFF

#define ADDR_DMA 0x4014
#define ADDR_INPUT_PORT_1 0x4016
#define ADDR_INPUT_PORT_2 0x4017

#define ADDR_PPU_CONTROL 0x2000
#define ADDR_PPU_MASK 0x2001
#define ADDR_PPU_STATUS 0x2002
#define ADDR_PPU_OAM_ADDR 0x2003
#define ADDR_PPU_OAM_DATA 0x2004
#define ADDR_PPU_SCROLL 0x2005
#define ADDR_PPU_ADDR 0x2006
#define ADDR_PPU_DATA 0x2007

#define STATUS_CARRY       0x01
#define STATUS_ZERO        0x02
#define STATUS_INTERRUPT    0x04
#define STATUS_DECIMAL      0x08
#define STATUS_BRK        0x10
#define STATUS_EMPTY      0x20
#define STATUS_OVERFLOW      0x40
#define STATUS_SIGN        0x80

#define TEST_STATUS_ADDR    0x6000
#define TEST_PREAMBLE_ADDR1    0x6001
#define TEST_PREAMBLE_ADDR2    0x6002
#define TEST_PREAMBLE_ADDR3    0x6003
#define TEST_OUTPUT_ADDR    0x6004
#define TEST_PREAMBLE1      0xDE
#define TEST_PREAMBLE2      0xB0
#define TEST_PREAMBLE3      0x61
#define TEST_STATUS_RUNNING    0x80
#define TEST_STATUS_NEED_RESET  0x81
#define TEST_RESET_SIGNATURE  0x4C4C4C4C

#define DUMMY_NONE        0
#define DUMMY_ONCARRY      1
#define DUMMY_ALWAYS      2

#define INTERRUPT_CYCLES    7


enum ControllerStatus { FirstWrite, SecondWrite, Ready };
enum Interrupt { Nmi, Irq, Brk, Reset };


typedef struct
{
  std::string name;
  SDLKey code;
  bool pressed;
} keyEntry;


class cpu
{
public:
  cpu();
  ~cpu();
  void init(boost::shared_ptr<Cartridge> mapper, boost::shared_ptr<ppu> ppu);
  bool isInitialized() { return _isInitialized; }
  void start();
  void stop();
  void reset();
  unsigned short executeOpcode();
  unsigned char read(unsigned short address);
  bool checkTestStatus();
  void enqueueInterrupt(const enum Interrupt &interrupt);
  void dequeueInterrupt();
  void updateControllerKeyStatus(SDL_Event event);

private:
  boost::shared_ptr<Cartridge> _mapper;
  boost::shared_ptr<ppu> _ppu;

  bool is_running;
  bool isAborted;
  unsigned short opcode;
  opcode_entry entry;
  std::map<char, opcode_entry> opcode_table;
  std::map<char, opcode_entry>::const_iterator it;
  std::vector<keyEntry> keyTable;
  std::vector<keyEntry>::iterator keyIterator;
  std::list<Interrupt> interrupts;
  
  bool _isInitialized;
  enum ControllerStatus controllerStatus;
  unsigned char controllerLastWrite;
  unsigned char controllerReadCount[2];

  unsigned char *memory;
  unsigned short reg_pc;
  unsigned char reg_sp;
  unsigned char reg_acc;
  unsigned char reg_index_x;
  unsigned char reg_index_y;
  unsigned char reg_status;

  unsigned short src, value, result, address;

  bool branchTaken;
  bool pageBoundaryCrossed;
  int opcodeHistory;

  void write(unsigned short address, unsigned char value);
  unsigned short normalizeAddress(unsigned short address);
  void log(opcode_entry *entry);
  unsigned short executeInterrupt(const enum Interrupt &interrupt);

  unsigned char readController(unsigned char controllerId);
  void writeController(unsigned char controllerId, unsigned char value);


  inline void setPageBoundaryCrossed(unsigned short address1, unsigned short address2);
  inline void setPageBoundaryCrossed(unsigned short address1, unsigned short address2, unsigned char dummy);
  inline bool hasStatusFlag(unsigned char flag);
  inline void setStatusFlag(unsigned char flags);
  inline void clearStatusFlag(unsigned char flags);
  inline void testAndSet(bool expr, unsigned char flags);
  inline void setSignFlag(unsigned short src);
  inline void setZeroFlag(unsigned short src);
  inline void setCarryFlag(unsigned short src);
  inline void setOverflowFlag(unsigned short value, unsigned short mem);
  inline void pushStack(unsigned char byte);

  inline unsigned char popStack();
  inline unsigned short modeImplied(unsigned char dummy);
  inline unsigned short modeImm(unsigned char dummy);
  inline unsigned short modeAbsolute(unsigned char dummy);
  inline unsigned short modeAbsoluteX(unsigned char dummy);
  inline unsigned short modeAbsoluteY(unsigned char dummy);
  inline unsigned short modeAbsoluteZeroPage(unsigned char dummy);
  inline unsigned short modeAbsoluteXZeroPage(unsigned char dummy);
  inline unsigned short modeAbsoluteYZeroPage(unsigned char dummy);
  inline unsigned short modeIndirect(unsigned char dummy);
  inline unsigned short modePostIndirect(unsigned char dummy);
  inline unsigned short modePostIndirectY(unsigned char dummy);
  inline unsigned short modePreIndirectX(unsigned char dummy);
  inline unsigned short modeRelative(unsigned char dummy);

  inline void funcCompareMemoryAccumulator();
  inline void funcCompareMemoryIndexX();
  inline void funcCompareMemoryIndexY();
  inline void funcLoadAccumulator();
  inline void funcLoadRegisterX();
  inline void funcLoadRegisterY();
  inline void funcStoreAccumulator();
  inline void funcStoreRegisterX();
  inline void funcStoreRegisterY();
  inline void funcIncreaseRegisterX();
  inline void funcIncreaseRegisterY();
  inline void funcIncreaseMemory();
  inline void funcDecreaseRegisterX();
  inline void funcDecreaseRegisterY();
  inline void funcDecreaseMemory();
  inline void funcAnd();
  inline void funcOr();
  inline void funcXor();
  inline void funcCompareRegisterX();
  inline void funcCompareRegisterY();
  inline void funcCompareMemory();
  inline void funcADC();
  inline void funcSBC();
  inline void funcShiftRightToAccumulator();
  inline void funcShiftRightToMemory();
  inline void funcShiftLeftToAccumulator();
  inline void funcShiftLeftToMemory();
  inline void funcRotateRightToAccumulator();
  inline void funcRotateRightToMemory();
  inline void funcRotateLeftToAccumulator();
  inline void funcRotateLeftToMemory();
  inline void funcBreak();
  inline void funcSetInterruptDisable();
  inline void funcSetDecimalMode();
  inline void funcSetCarryFlag();
  inline void funcJumpSaveReturnAddress();
  inline void funcReturnFromSubroutine();
  inline void funcReturnFromInterrupt();
  inline void funcBranchResultNotZero();
  inline void funcBranchResultZero();
  inline void funcBranchCarrySet();
  inline void funcBranchCarryClear();
  inline void funcBranchResultMinus();
  inline void funcBranchResultPlus();
  inline void funcBranchOverflowClear();
  inline void funcBranchOverflowSet();
  inline void funcNop();
  inline void funcTransferIndexXToStackPointer();
  inline void funcTransferIndexXToAccumulator();
  inline void funcTransferStackPointerToIndexX();
  inline void funcTransferAccumulatorToIndexY();
  inline void funcTransferAccumulatorToIndexX();
  inline void funcTransferIndexYToAccumulator();
  inline void funcClearCarryFlag();
  inline void funcClearDecimalMode();
  inline void funcClearInterruptDisable();
  inline void funcClearOverflowFlag();
  inline void funcJump();
  inline void funcBit();
  inline void funcPushStatusToStack();
  inline void funcPopStatusFromStack();
  inline void funcPushAccumulatorToStack();
  inline void funcPopAccumulatorFromStack();
  inline void funcLAX();
  inline void funcSAX();
  inline void funcDCP();
  inline void funcISC();
  inline void funcSLO();
  inline void funcRLA();
  inline void funcSRE();
  inline void funcRRA();
  inline void funcANC();
  inline void funcALR();
  inline void funcARR();
  inline void funcAXS();
  inline void funcSHY();
  inline void funcSHX();

  friend class Cartridge;
  friend class Mapper2;
};

#endif
