#include <iostream>
#include <sstream>
#include <iomanip>
#include <bitset>
#include <initializer_list>
#include <map>
#include <string.h>
#include <boost/assign.hpp>

#include "cartridge.h"
#include "ines.h"
#include "cpu.h"
#include "ppu.h"
#include "opcodes.h"
#include "config.h"
#include "yane_exception.h"
#include "utils.h"


using namespace std;

void cpu::start()
{
  BOOST_ASSERT_MSG(_ppu, "CPU is not initialized");

  is_running = true;
  isAborted = false;
  opcodeHistory = 0;

  // Set valid register values on startup
  reg_sp = SP_INIT;
  reg_status = STATUS_INIT;
  reg_acc = 0;
  reg_index_x = 0;
  reg_index_y = 0;
  reg_pc = executeInterrupt(Interrupt::Reset);

  // NESTEST has an invalid reset vector, lets adjust it
  if (Config::instance().isNesTest)
  {
    reg_pc -= 4;
  }
}

void cpu::stop()
{
}

unsigned short cpu::executeOpcode()
{
  // Take care of pending interrupts
  if (interrupts.size() > 0)
  {
    bool interruptShouldExecute = true;

    // Ignore interrupt for now if disabled flag is set, besides NMI, which always get executed
    if (interrupts.front() != Interrupt::Nmi && hasStatusFlag(STATUS_INTERRUPT))
    {
      interruptShouldExecute = false;
    }

    if (interruptShouldExecute)
    {
      reg_pc = executeInterrupt(interrupts.front());
      interrupts.pop_front();

      // Interrupt takes 7 cycles to complete
      return INTERRUPT_CYCLES;
    }
  }

  // Reset internal flags
  branchTaken = false;
  pageBoundaryCrossed = false;

  // Fetch and decode opcode
  opcode = read(reg_pc);
  opcodeHistory <<= 8;
  opcodeHistory |= opcode & 0xFF;
  setStatusFlag(STATUS_EMPTY);
  it = opcode_table.find(opcode);

  // Invalid opcode
  if (it == opcode_table.end())
  {
    throw InvalidOpcodeException(opcode);
  }

  // Execute opcode
  entry = it->second;

  // Log instruction?
  if (Config::instance().doInstructionLogging)
  {
    log(&entry);
  }

  src = (this->*entry.addressPtr)(entry.dummy);
  (this->*entry.functionPtr)();


  // Create opcode_entry for PPU
  //opcode_entry ppuOpcodeEntry = entry;
  unsigned short cycles = entry.cycles;

  // Add extra cycles if branch was taken
  if (branchTaken)
  {
    if (pageBoundaryCrossed)
    {
      // TODO: shouldn't this be +=2? but that fails instr_misc/03-dummy_reads
      cycles += 2;
    }
    else
    {
      cycles++;
    }
  }
  // Add an extra cycle if crossing page boundary
  else if (entry.cycles_extra && pageBoundaryCrossed)
  {
    cycles++;
  }

  // Increase the program counter if needed
  if (entry.skip_bytes)
  {
    reg_pc += entry.bytes;
  }

  return cycles;
}

void cpu::enqueueInterrupt(const enum Interrupt &interrupt)
{
  switch (interrupt)
  {
  // NMI has priority over everything! (=> non-maskable)
  case Interrupt::Nmi:
    interrupts.push_front(interrupt);
    break;

  default:
    interrupts.push_back(interrupt);
    break;
  }
}

void cpu::dequeueInterrupt()
{
  if (interrupts.size() > 0)
  {
    interrupts.pop_front();
  }
}

cpu::cpu()
{
  _mapper = nullptr;
  _ppu = nullptr;
  _isInitialized = false;
  controllerStatus = ControllerStatus::FirstWrite;
  controllerReadCount[0] = 0;
  controllerReadCount[1] = 0;

  memory = new unsigned char[RAM_SIZE];
  memset(memory, 0, RAM_SIZE);

  keyTable =
  {
    {"A",      SDLK_a,      false},
    {"B",      SDLK_b,      false},
    {"Select", SDLK_LSHIFT, false},
    {"Start",  SDLK_RETURN, false},
    {"Up",     SDLK_UP,     false},
    {"Down",   SDLK_DOWN,   false},
    {"Left",   SDLK_LEFT,   false},
    {"Right",  SDLK_RIGHT,  false},
  };

  opcode_table =
  {
    {LDA_IMM,    {"LDA_IMM",    &cpu::funcLoadAccumulator, &cpu::modeImm, 2, 2, false, true, DUMMY_NONE}},
    {LDA_ZERO,   {"LDA_ZERO",   &cpu::funcLoadAccumulator, &cpu::modeAbsoluteZeroPage, 2, 3, false, true, DUMMY_NONE}},
    {LDA_ZERO_X, {"LDA_ZERO_X", &cpu::funcLoadAccumulator, &cpu::modeAbsoluteXZeroPage, 2, 4, false, true, DUMMY_NONE}},
    {LDA_ABS,    {"LDA_ABS",    &cpu::funcLoadAccumulator, &cpu::modeAbsolute, 3, 4, false, true, DUMMY_NONE}},
    {LDA_ABS_X,  {"LDA_ABS_X",  &cpu::funcLoadAccumulator, &cpu::modeAbsoluteX, 3, 4, true, true, DUMMY_ONCARRY}},
    {LDA_ABS_Y,  {"LDA_ABS_Y",  &cpu::funcLoadAccumulator, &cpu::modeAbsoluteY, 3, 4, true, true, DUMMY_ONCARRY}},
    {LDA_IND_X,  {"LDA_IND_X",  &cpu::funcLoadAccumulator, &cpu::modePreIndirectX, 2, 6, false, true, DUMMY_NONE}},
    {LDA_IND_Y,  {"LDA_IND_Y",  &cpu::funcLoadAccumulator, &cpu::modePostIndirectY, 2, 5, true, true, DUMMY_ONCARRY}},

    {STA_ZERO,   {"STA_ZERO",   &cpu::funcStoreAccumulator, &cpu::modeAbsoluteZeroPage, 2, 3, false, true, DUMMY_NONE}},
    {STA_ZERO_X, {"STA_ZERO_X", &cpu::funcStoreAccumulator, &cpu::modeAbsoluteXZeroPage, 2, 4, false, true, DUMMY_NONE}},
    {STA_ABS,    {"STA_ABS2",   &cpu::funcStoreAccumulator, &cpu::modeAbsolute, 3, 4, false, true, DUMMY_NONE}},
    {STA_ABS_X,  {"STA_ABS_X",  &cpu::funcStoreAccumulator, &cpu::modeAbsoluteX, 3, 5, false, true, DUMMY_ALWAYS}},
    {STA_ABS_Y,  {"STA_ABS_Y",  &cpu::funcStoreAccumulator, &cpu::modeAbsoluteY, 3, 5, false, true, DUMMY_ALWAYS}},
    {STA_IND_X,  {"STA_IND_X",  &cpu::funcStoreAccumulator, &cpu::modePreIndirectX, 2, 6, false, true, DUMMY_NONE}},
    {STA_IND_Y,  {"STA_IND_Y",  &cpu::funcStoreAccumulator, &cpu::modePostIndirectY, 2, 6, false, true, DUMMY_ALWAYS}},


    {LDX_IMM,    {"LDX_IMM",    &cpu::funcLoadRegisterX, &cpu::modeImm, 2, 2, false, true, DUMMY_NONE}},
    {LDX_ZERO,   {"LDX_ZERO",   &cpu::funcLoadRegisterX, &cpu::modeAbsoluteZeroPage, 2, 3, false, true, DUMMY_NONE}},
    {LDX_ZERO_Y, {"LDX_ZERO_Y", &cpu::funcLoadRegisterX, &cpu::modeAbsoluteYZeroPage, 2, 4, false, true, DUMMY_NONE}},
    {LDX_ABS,    {"LDX_ABS",    &cpu::funcLoadRegisterX, &cpu::modeAbsolute, 3, 4, false, true, DUMMY_NONE}},
    {LDX_ABS_Y,  {"LDX_ABS_Y",  &cpu::funcLoadRegisterX, &cpu::modeAbsoluteY, 3, 4, true, true, DUMMY_ONCARRY}},

    {STX_ZERO,   {"STX_ZERO",   &cpu::funcStoreRegisterX, &cpu::modeAbsoluteZeroPage, 2, 3, false, true, DUMMY_NONE}},
    {STX_ZERO_Y, {"STX_ZERO_Y", &cpu::funcStoreRegisterX, &cpu::modeAbsoluteYZeroPage, 2, 4, false, true, DUMMY_NONE}},
    {STX_ABS,    {"STX_ABS",    &cpu::funcStoreRegisterX, &cpu::modeAbsolute, 3, 4, false, true, DUMMY_NONE}},


    {LDY_IMM,    {"LDY_IMM",    &cpu::funcLoadRegisterY, &cpu::modeImm, 2, 2, false, true, DUMMY_NONE}},
    {LDY_ZERO,   {"LDY_ZERO",   &cpu::funcLoadRegisterY, &cpu::modeAbsoluteZeroPage, 2, 3, false, true, DUMMY_NONE}},
    {LDY_ZERO_X, {"LDY_ZERO_X", &cpu::funcLoadRegisterY, &cpu::modeAbsoluteXZeroPage, 2, 4, false, true, DUMMY_NONE}},
    {LDY_ABS,    {"LDY_ABS",    &cpu::funcLoadRegisterY, &cpu::modeAbsolute, 3, 4, false, true, DUMMY_NONE}},
    {LDY_ABS_X,  {"LDY_ABS_Y",  &cpu::funcLoadRegisterY, &cpu::modeAbsoluteX, 3, 4, true, true, DUMMY_ONCARRY}},

    {STY_ZERO,   {"STY_ZERO",   &cpu::funcStoreRegisterY, &cpu::modeAbsoluteZeroPage, 2, 3, false, true, DUMMY_NONE}},
    {STY_ZERO_X, {"STY_ZERO_X", &cpu::funcStoreRegisterY, &cpu::modeAbsoluteXZeroPage, 2, 4, false, true, DUMMY_NONE}},
    {STY_ABS,    {"STY_ABS",    &cpu::funcStoreRegisterY, &cpu::modeAbsolute, 3, 4, false, true, DUMMY_NONE}},


    {INX, {"INX", &cpu::funcIncreaseRegisterX, &cpu::modeImplied, 1, 2, false, true, DUMMY_NONE}},
    {DEX, {"DEX", &cpu::funcDecreaseRegisterX, &cpu::modeImplied, 1, 2, false, true, DUMMY_NONE}},
    {INY, {"INY", &cpu::funcIncreaseRegisterY, &cpu::modeImplied, 1, 2, false, true, DUMMY_NONE}},
    {DEY, {"DEY", &cpu::funcDecreaseRegisterY, &cpu::modeImplied, 1, 2, false, true, DUMMY_NONE}},

    {INC_ZERO,   {"INC_ZERO",   &cpu::funcIncreaseMemory, &cpu::modeAbsoluteZeroPage, 2, 5, false, true, DUMMY_NONE}},
    {INC_ZERO_X, {"INC_ZERO_X", &cpu::funcIncreaseMemory, &cpu::modeAbsoluteXZeroPage, 2, 6, false, true, DUMMY_NONE}},
    {INC_ABS,    {"INC_ABS",    &cpu::funcIncreaseMemory, &cpu::modeAbsolute, 3, 6, false, true, DUMMY_NONE}},
    {INC_ABS_X,  {"INC_ABS_X",  &cpu::funcIncreaseMemory, &cpu::modeAbsoluteX, 3, 7, false, true, DUMMY_ALWAYS}},

    {DEC_ZERO,   {"DEC_ZERO",   &cpu::funcDecreaseMemory, &cpu::modeAbsoluteZeroPage, 2, 5, false, true, DUMMY_NONE}},
    {DEC_ZERO_X, {"DEC_ZERO_X", &cpu::funcDecreaseMemory, &cpu::modeAbsoluteXZeroPage, 2, 6, false, true, DUMMY_NONE}},
    {DEC_ABS,    {"DEC_ABS",    &cpu::funcDecreaseMemory, &cpu::modeAbsolute, 3, 6, false, true, DUMMY_NONE}},
    {DEC_ABS_X,  {"DEC_ABS_X",  &cpu::funcDecreaseMemory, &cpu::modeAbsoluteX, 3, 7, false, true, DUMMY_ALWAYS}},


    {CPX_IMM,  {"CPX_IMM",  &cpu::funcCompareRegisterX, &cpu::modeImm, 2, 2, false, true, DUMMY_NONE}},
    {CPX_ZERO, {"CPX_ZERO", &cpu::funcCompareRegisterX, &cpu::modeAbsoluteZeroPage, 2, 3, false, true, DUMMY_NONE}},
    {CPX_ABS,  {"CPX_ABS",  &cpu::funcCompareRegisterX, &cpu::modeAbsolute, 3, 4, false, true, DUMMY_NONE}},

    {CPY_IMM,  {"CPY_IMM",  &cpu::funcCompareRegisterY, &cpu::modeImm, 2, 2, false, true, DUMMY_NONE}},
    {CPY_ZERO, {"CPY_ZERO", &cpu::funcCompareRegisterY, &cpu::modeAbsoluteZeroPage, 2, 3, false, true, DUMMY_NONE}},
    {CPY_ABS,  {"CPY_ABS",  &cpu::funcCompareRegisterY, &cpu::modeAbsolute, 3, 4, false, true, DUMMY_NONE}},

    {CMP_IMM,    {"CMP_IMM",    &cpu::funcCompareMemory, &cpu::modeImm, 2, 2, false, true, DUMMY_NONE}},
    {CMP_ZERO,   {"CMP_ZERO",   &cpu::funcCompareMemory, &cpu::modeAbsoluteZeroPage, 2, 3, false, true, DUMMY_NONE}},
    {CMP_ZERO_X, {"CMP_ZERO_X", &cpu::funcCompareMemory, &cpu::modeAbsoluteXZeroPage, 2, 4, false, true, DUMMY_NONE}},
    {CMP_ABS,    {"CMP_ABS",    &cpu::funcCompareMemory, &cpu::modeAbsolute, 3, 4, false, true, DUMMY_NONE}},
    {CMP_ABS_X,  {"CMP_ABS_X",  &cpu::funcCompareMemory, &cpu::modeAbsoluteX, 3, 4, true, true, DUMMY_ONCARRY}},
    {CMP_ABS_Y,  {"CMP_ABS_Y",  &cpu::funcCompareMemory, &cpu::modeAbsoluteY, 3, 4, true, true, DUMMY_ONCARRY}},
    {CMP_IND_X,  {"CMP_IND_X",  &cpu::funcCompareMemory, &cpu::modePreIndirectX, 2, 6, false, true, DUMMY_NONE}},
    {CMP_IND_Y,  {"CMP_IND_Y",  &cpu::funcCompareMemory, &cpu::modePostIndirectY, 2, 5, true, true, DUMMY_ONCARRY}},


    {AND_IMM,    {"AND_IMM",    &cpu::funcAnd, &cpu::modeImm, 2, 2, false, true, DUMMY_NONE}},
    {AND_ZERO,   {"AND_ZERO",   &cpu::funcAnd, &cpu::modeAbsoluteZeroPage, 2, 3, false, true, DUMMY_NONE}},
    {AND_ZERO_X, {"AND_ZERO_X", &cpu::funcAnd, &cpu::modeAbsoluteXZeroPage, 2, 4, false, true, DUMMY_NONE}},
    {AND_ABS,    {"AND_ABS",    &cpu::funcAnd, &cpu::modeAbsolute, 3, 4, false, true, DUMMY_NONE}},
    {AND_ABS_X,  {"AND_ABS_X",  &cpu::funcAnd, &cpu::modeAbsoluteX, 3, 4, true, true, DUMMY_ONCARRY}},
    {AND_ABS_Y,  {"AND_ABS_Y",  &cpu::funcAnd, &cpu::modeAbsoluteY, 3, 4, true, true, DUMMY_ONCARRY}},
    {AND_IND_X,  {"AND_IND_X",  &cpu::funcAnd, &cpu::modePreIndirectX, 2, 6, false, true, DUMMY_NONE}},
    {AND_IND_Y,   {"AND_IND_Y", &cpu::funcAnd, &cpu::modePostIndirectY, 2, 5, true, true, DUMMY_ONCARRY}},

    {OR_IMM,    {"OR_IMM",    &cpu::funcOr, &cpu::modeImm, 2, 2, false, true, DUMMY_NONE}},
    {OR_ZERO,   {"OR_ZERO",   &cpu::funcOr, &cpu::modeAbsoluteZeroPage, 2, 3, false, true, DUMMY_NONE}},
    {OR_ZERO_X, {"OR_ZERO_X", &cpu::funcOr, &cpu::modeAbsoluteXZeroPage, 2, 4, false, true, DUMMY_NONE}},
    {OR_ABS,    {"OR_ABS",    &cpu::funcOr, &cpu::modeAbsolute, 3, 4, false, true, DUMMY_NONE}},
    {OR_ABS_X,  {"OR_ABS_X",  &cpu::funcOr, &cpu::modeAbsoluteX, 3, 4, true, true, DUMMY_ONCARRY}},
    {OR_ABS_Y,  {"OR_ABS_Y",  &cpu::funcOr, &cpu::modeAbsoluteY, 3, 4, true, true, DUMMY_ONCARRY}},
    {OR_IND_X,  {"OR_IND_X",  &cpu::funcOr, &cpu::modePreIndirectX, 2, 6, false, true, DUMMY_NONE}},
    {OR_IND_Y,  {"OR_IND_Y",  &cpu::funcOr, &cpu::modePostIndirectY, 2, 5, true, true, DUMMY_ONCARRY}},

    {XOR_IMM,    {"XOR_IMM",    &cpu::funcXor, &cpu::modeImm, 2, 2, false, true, DUMMY_NONE}},
    {XOR_ZERO,   {"XOR_ZERO",   &cpu::funcXor, &cpu::modeAbsoluteZeroPage, 2, 3, false, true, DUMMY_NONE}},
    {XOR_ZERO_X, {"XOR_ZERO_X", &cpu::funcXor, &cpu::modeAbsoluteXZeroPage, 2, 4, false, true, DUMMY_NONE}},
    {XOR_ABS,    {"XOR_ABS",    &cpu::funcXor, &cpu::modeAbsolute, 3, 4, false, true, DUMMY_NONE}},
    {XOR_ABS_X,  {"XOR_ABS_X",  &cpu::funcXor, &cpu::modeAbsoluteX, 3, 4, true, true, DUMMY_ONCARRY}},
    {XOR_ABS_Y,  {"XOR_ABS_Y",  &cpu::funcXor, &cpu::modeAbsoluteY, 3, 4, true, true, DUMMY_ONCARRY}},
    {XOR_IND_X,  {"XOR_IND_X",  &cpu::funcXor, &cpu::modePreIndirectX, 2, 6, false, true, DUMMY_NONE}},
    {XOR_IND_Y,  {"XOR_IND_Y",  &cpu::funcXor, &cpu::modePostIndirectY, 2, 5, true, true, DUMMY_ONCARRY}},


    {LSR_ACC,    {"LSR_ACC",    &cpu::funcShiftRightToAccumulator, &cpu::modeImm, 1, 2, false, true, DUMMY_NONE}},
    {LSR_ZERO,   {"LSR_ZERO",   &cpu::funcShiftRightToMemory, &cpu::modeAbsoluteZeroPage, 2, 5, false, true, DUMMY_NONE}},
    {LSR_ZERO_X, {"LSR_ZERO_X", &cpu::funcShiftRightToMemory, &cpu::modeAbsoluteXZeroPage, 2, 6, false, true, DUMMY_NONE}},
    {LSR_ABS,    {"LSR_ABS",    &cpu::funcShiftRightToMemory, &cpu::modeAbsolute, 3, 6, false, true, DUMMY_NONE}},
    {LSR_ABS_X,  {"LSR_ABS_X",  &cpu::funcShiftRightToMemory, &cpu::modeAbsoluteX, 3, 7, false, true, DUMMY_ALWAYS}},

    {ASL_ACC,    {"ASL_ACC",    &cpu::funcShiftLeftToAccumulator, &cpu::modeImm, 1, 2, false, true, DUMMY_NONE}},
    {ASL_ZERO,   {"ASL_ZERO",   &cpu::funcShiftLeftToMemory, &cpu::modeAbsoluteZeroPage, 2, 5, false, true, DUMMY_NONE}},
    {ASL_ZERO_X, {"ASL_ZERO_X", &cpu::funcShiftLeftToMemory, &cpu::modeAbsoluteXZeroPage, 2, 6, false, true, DUMMY_NONE}},
    {ASL_ABS,    {"ASL_ABS",    &cpu::funcShiftLeftToMemory, &cpu::modeAbsolute, 3, 6, false, true, DUMMY_NONE}},
    {ASL_ABS_X,  {"ASL_ABS_X",  &cpu::funcShiftLeftToMemory, &cpu::modeAbsoluteX, 3, 7, false, true, DUMMY_ALWAYS}},

    {ROR_ACC,    {"ROR_ACC",    &cpu::funcRotateRightToAccumulator, &cpu::modeImm, 1, 2, false, true, DUMMY_NONE}},
    {ROR_ZERO,   {"ROR_ZERO",   &cpu::funcRotateRightToMemory, &cpu::modeAbsoluteZeroPage, 2, 5, false, true, DUMMY_NONE}},
    {ROR_ZERO_X, {"ROR_ZERO_X", &cpu::funcRotateRightToMemory, &cpu::modeAbsoluteXZeroPage, 2, 6, false, true, DUMMY_NONE}},
    {ROR_ABS,    {"ROR_ABS",    &cpu::funcRotateRightToMemory, &cpu::modeAbsolute, 3, 6, false, true, DUMMY_NONE}},
    {ROR_ABS_X,  {"ROR_ABS_X",  &cpu::funcRotateRightToMemory, &cpu::modeAbsoluteX, 3, 7, false, true, DUMMY_ALWAYS}},

    {ROL_ACC,    {"ROL_ACC",    &cpu::funcRotateLeftToAccumulator, &cpu::modeImm, 1, 2, false, true, DUMMY_NONE}},
    {ROL_ZERO,   {"ROL_ZERO",   &cpu::funcRotateLeftToMemory, &cpu::modeAbsoluteZeroPage, 2, 5, false, true, DUMMY_NONE}},
    {ROL_ZERO_X, {"ROL_ZERO_X", &cpu::funcRotateLeftToMemory, &cpu::modeAbsoluteXZeroPage, 2, 6, false, true, DUMMY_NONE}},
    {ROL_ABS,    {"ROL_ABS",    &cpu::funcRotateLeftToMemory, &cpu::modeAbsolute, 3, 6, false, true, DUMMY_NONE}},
    {ROL_ABS_X,  {"ROL_ABS_X",  &cpu::funcRotateLeftToMemory, &cpu::modeAbsoluteX, 3, 7, false, true, DUMMY_ALWAYS}},


    {ADC_IMM,    {"ADC_IMM",    &cpu::funcADC, &cpu::modeImm, 2, 2, false, true, DUMMY_NONE}},
    {ADC_ZERO,   {"ADC_ZERO",   &cpu::funcADC, &cpu::modeAbsoluteZeroPage, 2, 3, false, true, DUMMY_NONE}},
    {ADC_ZERO_X, {"ADC_ZERO_X", &cpu::funcADC, &cpu::modeAbsoluteXZeroPage, 2, 4, false, true, DUMMY_NONE}},
    {ADC_ABS,    {"ADC_ABS",    &cpu::funcADC, &cpu::modeAbsolute, 3, 4, false, true, DUMMY_NONE}},
    {ADC_ABS_X,  {"ADC_ABS_X",  &cpu::funcADC, &cpu::modeAbsoluteX, 3, 4, true, true, DUMMY_ONCARRY}},
    {ADC_ABS_Y,  {"ADC_ABS_Y",  &cpu::funcADC, &cpu::modeAbsoluteY, 3, 4, true, true, DUMMY_ONCARRY}},
    {ADC_IND_X,  {"ADC_IND_X",  &cpu::funcADC, &cpu::modePreIndirectX, 2, 6, false, true, DUMMY_NONE}},
    {ADC_IND_Y,  {"ADC_IND_Y",  &cpu::funcADC, &cpu::modePostIndirectY, 2, 5, true, true, DUMMY_ONCARRY}},

    {SBC_IMM,    {"SBC_IMM",    &cpu::funcSBC, &cpu::modeImm, 2, 2, false, true, DUMMY_NONE}},
    {SBC_IMM2,   {"SBC_IMM2",   &cpu::funcSBC, &cpu::modeImm, 2, 2, false, true, DUMMY_NONE}},
    {SBC_ZERO,   {"SBC_ZERO",   &cpu::funcSBC, &cpu::modeAbsoluteZeroPage, 2, 3, false, true, DUMMY_NONE}},
    {SBC_ZERO_X, {"SBC_ZERO_X", &cpu::funcSBC, &cpu::modeAbsoluteXZeroPage, 2, 4, false, true, DUMMY_NONE}},
    {SBC_ABS,    {"SBC_ABS",    &cpu::funcSBC, &cpu::modeAbsolute, 3, 4, false, true, DUMMY_NONE}},
    {SBC_ABS_X,  {"SBC_ABS_X",  &cpu::funcSBC, &cpu::modeAbsoluteX, 3, 4, true, true, DUMMY_ONCARRY}},
    {SBC_ABS_Y,  {"SBC_ABS_Y",  &cpu::funcSBC, &cpu::modeAbsoluteY, 3, 4, true, true, DUMMY_ONCARRY}},
    {SBC_IND_X,  {"SBC_IND_X",  &cpu::funcSBC, &cpu::modePreIndirectX, 2, 6, false, true, DUMMY_NONE}},
    {SBC_IND_Y,  {"SBC_IND_Y",  &cpu::funcSBC, &cpu::modePostIndirectY, 2, 5, true, true, DUMMY_ONCARRY}},


    {PHP, {"PHP", &cpu::funcPushStatusToStack, &cpu::modeImplied, 1, 3, false, true, DUMMY_NONE}},
    {PLP, {"PLP", &cpu::funcPopStatusFromStack, &cpu::modeImplied, 1, 4, false, true, DUMMY_NONE}},
    {PHA, {"PHA", &cpu::funcPushAccumulatorToStack, &cpu::modeImplied, 1, 3, false, true, DUMMY_NONE}},
    {PLA, {"PLA", &cpu::funcPopAccumulatorFromStack, &cpu::modeImplied, 1, 4, false, true, DUMMY_NONE}},


    {JSR,     {"JSR",     &cpu::funcJumpSaveReturnAddress, &cpu::modeAbsolute, 3, 6, false, false, DUMMY_NONE}},
    {JMP_ABS, {"JMP_ABS", &cpu::funcJump, &cpu::modeAbsolute, 3, 3, false, false, DUMMY_NONE}},
    {JMP_IND, {"JMP_IND", &cpu::funcJump, &cpu::modeIndirect, 3, 5, false, false, DUMMY_NONE}},


    {BIT_ZERO, {"BIT_ZERO", &cpu::funcBit, &cpu::modeAbsoluteZeroPage, 2, 3, false, true, DUMMY_NONE}},
    {BIT_ABS,  {"BIT_ABS",  &cpu::funcBit, &cpu::modeAbsolute, 3, 4, false, true, DUMMY_NONE}},


    {SEC, {"SEC", &cpu::funcSetCarryFlag, &cpu::modeImplied, 1, 2, false, true, DUMMY_NONE}},
    {SED, {"SED", &cpu::funcSetDecimalMode, &cpu::modeImplied, 1, 2, false, true, DUMMY_NONE}},
    {SEI, {"SEI", &cpu::funcSetInterruptDisable, &cpu::modeImplied, 1, 2, false, true, DUMMY_NONE}},

    {CLC, {"CLC", &cpu::funcClearCarryFlag, &cpu::modeImplied, 1, 2, false, true, DUMMY_NONE}},
    {CLD, {"CLD", &cpu::funcClearDecimalMode, &cpu::modeImplied, 1, 2, false, true, DUMMY_NONE}},
    {CLI, {"CLI", &cpu::funcClearInterruptDisable, &cpu::modeImplied, 1, 2, false, true, DUMMY_NONE}},
    {CLV, {"CLV", &cpu::funcClearOverflowFlag, &cpu::modeImplied, 1, 2, false, true, DUMMY_NONE}},


    {TXS, {"TXS", &cpu::funcTransferIndexXToStackPointer, &cpu::modeImplied, 1, 2, false, true, DUMMY_NONE}},
    {TXA, {"TXA", &cpu::funcTransferIndexXToAccumulator, &cpu::modeImplied, 1, 2, false, true, DUMMY_NONE}},
    {TSX, {"TSX", &cpu::funcTransferStackPointerToIndexX, &cpu::modeImplied, 1, 2, false, true, DUMMY_NONE}},
    {TAY, {"TAY", &cpu::funcTransferAccumulatorToIndexY, &cpu::modeImplied, 1, 2, false, true, DUMMY_NONE}},
    {TAX, {"TAX", &cpu::funcTransferAccumulatorToIndexX, &cpu::modeImplied, 1, 2, false, true, DUMMY_NONE}},
    {TYA, {"TYA", &cpu::funcTransferIndexYToAccumulator, &cpu::modeImplied, 1, 2, false, true, DUMMY_NONE}},

    {LAX_IND_X,  {"LAX_IND_X",  &cpu::funcLAX, &cpu::modePreIndirectX, 2, 6, false, true, DUMMY_NONE}},
    {LAX_ZERO,   {"LAX_ZERO",   &cpu::funcLAX, &cpu::modeAbsoluteZeroPage, 2, 3, false, true, DUMMY_NONE}},
    {LAX_IMM,    {"LAX_IMM",    &cpu::funcLAX, &cpu::modeImm, 2, 2, false, true, DUMMY_NONE}},
    {LAX_ABS,    {"LAX_ABS",    &cpu::funcLAX, &cpu::modeAbsolute, 3, 4, false, true, DUMMY_NONE}},
    {LAX_IND_Y,  {"LAX_IND_Y",  &cpu::funcLAX, &cpu::modePostIndirectY, 2, 5, true, true, DUMMY_ONCARRY}},
    {LAX_ZERO_Y, {"LAX_ZERO_Y", &cpu::funcLAX, &cpu::modeAbsoluteYZeroPage, 2, 4, false, true, DUMMY_NONE}},
    {LAX_ABS_Y,  {"LAX_ABS_Y",  &cpu::funcLAX, &cpu::modeAbsoluteY, 3, 4, true, true, DUMMY_ONCARRY}},

    {SAX_IND_X,  {"SAX_IND_X",  &cpu::funcSAX, &cpu::modePreIndirectX, 2, 6, false, true, DUMMY_NONE}},
    {SAX_ZERO,   {"SAX_ZERO",   &cpu::funcSAX, &cpu::modeAbsoluteZeroPage, 2, 3, false, true, DUMMY_NONE}},
    {SAX_ABS,    {"SAX_ABS",    &cpu::funcSAX, &cpu::modeAbsolute, 3, 4, false, true, DUMMY_NONE}},
    {SAX_ZERO_Y, {"SAX_ZERO_Y", &cpu::funcSAX, &cpu::modeAbsoluteYZeroPage, 2, 4, false, true, DUMMY_NONE}},

    {DCP_IND_X,  {"DCP_IND_X",  &cpu::funcDCP, &cpu::modePreIndirectX, 2, 8, false, true, DUMMY_NONE}},
    {DCP_ZERO,   {"DCP_ZERO",   &cpu::funcDCP, &cpu::modeAbsoluteZeroPage, 2, 5, false, true, DUMMY_NONE}},
    {DCP_ABS,    {"DCP_ABS",    &cpu::funcDCP, &cpu::modeAbsolute, 3, 6, false, true, DUMMY_NONE}},
    {DCP_IND_Y,  {"DCP_IND_Y",  &cpu::funcDCP, &cpu::modePostIndirectY, 2, 8, false, true, DUMMY_ALWAYS}},
    {DCP_ZERO_X, {"DCP_ZERO_X", &cpu::funcDCP, &cpu::modeAbsoluteXZeroPage, 2, 6, false, true, DUMMY_NONE}},
    {DCP_ABS_Y,  {"DCP_ABS_Y",  &cpu::funcDCP, &cpu::modeAbsoluteY, 3, 7, false, true, DUMMY_ALWAYS}},
    {DCP_ABS_X,  {"DCP_ABS_X",  &cpu::funcDCP, &cpu::modeAbsoluteX, 3, 7, false, true, DUMMY_ALWAYS}},

    {ISC_IND_X,  {"ISC_IND_X",  &cpu::funcISC, &cpu::modePreIndirectX, 2, 8, false, true, DUMMY_NONE}},
    {ISC_ZERO,   {"ISC_ZERO",   &cpu::funcISC, &cpu::modeAbsoluteZeroPage, 2, 5, false, true, DUMMY_NONE}},
    {ISC_ABS,    {"ISC_ABS",    &cpu::funcISC, &cpu::modeAbsolute, 3, 6, false, true, DUMMY_NONE}},
    {ISC_IND_Y,  {"ISC_IND_Y",  &cpu::funcISC, &cpu::modePostIndirectY, 2, 8, false, true, DUMMY_ALWAYS}},
    {ISC_ZERO_X, {"ISC_ZERO_X", &cpu::funcISC, &cpu::modeAbsoluteXZeroPage, 2, 6, false, true, DUMMY_NONE}},
    {ISC_ABS_Y,  {"ISC_ABS_Y",  &cpu::funcISC, &cpu::modeAbsoluteY, 3, 7, false, true, DUMMY_ALWAYS}},
    {ISC_ABS_X,  {"ISC_ABS_X",  &cpu::funcISC, &cpu::modeAbsoluteX, 3, 7, false, true, DUMMY_ALWAYS}},

    {SLO_IND_X,  {"SLO_IND_X",  &cpu::funcSLO, &cpu::modePreIndirectX, 2, 8, false, true, DUMMY_NONE}},
    {SLO_ZERO,   {"SLO_ZERO",   &cpu::funcSLO, &cpu::modeAbsoluteZeroPage, 2, 5, false, true, DUMMY_NONE}},
    {SLO_ABS,    {"SLO_ABS",    &cpu::funcSLO, &cpu::modeAbsolute, 3, 6, false, true, DUMMY_NONE}},
    {SLO_IND_Y,  {"SLO_IND_Y",  &cpu::funcSLO, &cpu::modePostIndirectY, 2, 8, false, true, DUMMY_ALWAYS}},
    {SLO_ZERO_X, {"SLO_ZERO_X", &cpu::funcSLO, &cpu::modeAbsoluteXZeroPage, 2, 6, false, true, DUMMY_NONE}},
    {SLO_ABS_Y,  {"SLO_ABS_Y",  &cpu::funcSLO, &cpu::modeAbsoluteY, 3, 7, false, true, DUMMY_ALWAYS}},
    {SLO_ABS_X,  {"SLO_ABS_X",  &cpu::funcSLO, &cpu::modeAbsoluteX, 3, 7, false, true, DUMMY_ALWAYS}},

    {RLA_IND_X,  {"RLA_IND_X",  &cpu::funcRLA, &cpu::modePreIndirectX, 2, 8, false, true, DUMMY_NONE}},
    {RLA_ZERO,   {"RLA_ZERO",   &cpu::funcRLA, &cpu::modeAbsoluteZeroPage, 2, 5, false, true, DUMMY_NONE}},
    {RLA_ABS,    {"RLA_ABS",    &cpu::funcRLA, &cpu::modeAbsolute, 3, 6, false, true, DUMMY_NONE}},
    {RLA_IND_Y,  {"RLA_IND_Y",  &cpu::funcRLA, &cpu::modePostIndirectY, 2, 8, false, true, DUMMY_ALWAYS}},
    {RLA_ZERO_X, {"RLA_ZERO_X", &cpu::funcRLA, &cpu::modeAbsoluteXZeroPage, 2, 6, false, true, DUMMY_NONE}},
    {RLA_ABS_Y,  {"RLA_ABS_Y",  &cpu::funcRLA, &cpu::modeAbsoluteY, 3, 7, false, true, DUMMY_ALWAYS}},
    {RLA_ABS_X,  {"RLA_ABS_X",  &cpu::funcRLA, &cpu::modeAbsoluteX, 3, 7, false, true, DUMMY_ALWAYS}},

    {SRE_IND_X,  {"SRE_IND_X",  &cpu::funcSRE, &cpu::modePreIndirectX, 2, 8, false, true, DUMMY_NONE}},
    {SRE_ZERO,   {"SRE_ZERO",   &cpu::funcSRE, &cpu::modeAbsoluteZeroPage, 2, 5, false, true, DUMMY_NONE}},
    {SRE_ABS,    {"SRE_ABS",    &cpu::funcSRE, &cpu::modeAbsolute, 3, 6, false, true, DUMMY_NONE}},
    {SRE_IND_Y,  {"SRE_IND_Y",  &cpu::funcSRE, &cpu::modePostIndirectY, 2, 8, false, true, DUMMY_ALWAYS}},
    {SRE_ZERO_X, {"SRE_ZERO_X", &cpu::funcSRE, &cpu::modeAbsoluteXZeroPage, 2, 6, false, true, DUMMY_NONE}},
    {SRE_ABS_Y,  {"SRE_ABS_Y",  &cpu::funcSRE, &cpu::modeAbsoluteY, 3, 7, false, true, DUMMY_ALWAYS}},
    {SRE_ABS_X,  {"SRE_ABS_X",  &cpu::funcSRE, &cpu::modeAbsoluteX, 3, 7, false, true, DUMMY_ALWAYS}},

    {RRA_IND_X,  {"RRA_IND_X",  &cpu::funcRRA, &cpu::modePreIndirectX, 2, 8, false, true, DUMMY_NONE}},
    {RRA_ZERO,   {"RRA_ZERO",   &cpu::funcRRA, &cpu::modeAbsoluteZeroPage, 2, 5, false, true, DUMMY_NONE}},
    {RRA_ABS,    {"RRA_ABS",    &cpu::funcRRA, &cpu::modeAbsolute, 3, 6, false, true, DUMMY_NONE}},
    {RRA_IND_Y,  {"RRA_IND_Y",  &cpu::funcRRA, &cpu::modePostIndirectY, 2, 8, false, true, DUMMY_ALWAYS}},
    {RRA_ZERO_X, {"RRA_ZERO_X", &cpu::funcRRA, &cpu::modeAbsoluteXZeroPage, 2, 6, false, true, DUMMY_NONE}},
    {RRA_ABS_Y,  {"RRA_ABS_Y",  &cpu::funcRRA, &cpu::modeAbsoluteY, 3, 7, false, true, DUMMY_ALWAYS}},
    {RRA_ABS_X,  {"RRA_ABS_X",  &cpu::funcRRA, &cpu::modeAbsoluteX, 3, 7, false, true, DUMMY_ALWAYS}},

    {ANC_IMM,  {"ANC_IMM", &cpu::funcANC, &cpu::modeImm, 2, 2, false, true, DUMMY_NONE}},
    {ANC_IMM2, {"ANC_IMM", &cpu::funcANC, &cpu::modeImm, 2, 2, false, true, DUMMY_NONE}},

    {ALR_IMM,  {"ALR_IMM", &cpu::funcALR, &cpu::modeImm, 2, 2, false, true, DUMMY_NONE}},

    {ARR_IMM,  {"ARR_IMM", &cpu::funcARR, &cpu::modeImm, 2, 2, false, true, DUMMY_NONE}},

    {AXS_IMM,  {"AXS_IMM", &cpu::funcAXS, &cpu::modeImm, 2, 2, false, true, DUMMY_NONE}},

    {SHY_ABS_X, {"SHY_ABS_X", &cpu::funcSHY, &cpu::modeAbsoluteX, 3, 5, false, true, DUMMY_ALWAYS}},
    {SHX_ABS_Y, {"SHX_ABS_Y", &cpu::funcSHX, &cpu::modeAbsoluteY, 3, 5, false, true, DUMMY_ALWAYS}},


    {BNE, {"BNE", &cpu::funcBranchResultNotZero, &cpu::modeRelative, 2, 2, true, true, DUMMY_NONE}},
    {BEQ, {"BEQ", &cpu::funcBranchResultZero,    &cpu::modeRelative, 2, 2, true, true, DUMMY_NONE}},
    {BCS, {"BCS", &cpu::funcBranchCarrySet,      &cpu::modeRelative, 2, 2, true, true, DUMMY_NONE}},
    {BCC, {"BCC", &cpu::funcBranchCarryClear,    &cpu::modeRelative, 2, 2, true, true, DUMMY_NONE}},
    {BMI, {"BMI", &cpu::funcBranchResultMinus,   &cpu::modeRelative, 2, 2, true, true, DUMMY_NONE}},
    {BPL, {"BPL", &cpu::funcBranchResultPlus,    &cpu::modeRelative, 2, 2, true, true, DUMMY_NONE}},
    {BVC, {"BVC", &cpu::funcBranchOverflowClear, &cpu::modeRelative, 2, 2, true, true, DUMMY_NONE}},
    {BVS, {"BVS", &cpu::funcBranchOverflowSet,   &cpu::modeRelative, 2, 2, true, true, DUMMY_NONE}},


    {RTS, {"RTS", &cpu::funcReturnFromSubroutine, &cpu::modeImplied, 1, 6, false, false, DUMMY_NONE}},
    {RTI, {"RTI", &cpu::funcReturnFromInterrupt,  &cpu::modeImplied, 1, 6, false, false, DUMMY_NONE}},
    {BRK, {"BRK", &cpu::funcBreak,                &cpu::modeImplied, 1, 7, false, false, DUMMY_NONE}},


    {NOP,   {"NOP", &cpu::funcNop, &cpu::modeImplied, 1, 2, false, true, DUMMY_NONE}},
    {NOP2,  {"NOP", &cpu::funcNop, &cpu::modeImplied, 2, 3, false, true, DUMMY_NONE}},
    {NOP3,  {"NOP", &cpu::funcNop, &cpu::modeImplied, 2, 3, false, true, DUMMY_NONE}},
    {NOP4,  {"NOP", &cpu::funcNop, &cpu::modeImplied, 2, 3, false, true, DUMMY_NONE}},
    {NOP5,  {"NOP", &cpu::funcNop, &cpu::modeImplied, 1, 2, false, true, DUMMY_NONE}},
    {NOP6,  {"NOP", &cpu::funcNop, &cpu::modeImplied, 1, 2, false, true, DUMMY_NONE}},
    {NOP7,  {"NOP", &cpu::funcNop, &cpu::modeImplied, 1, 2, false, true, DUMMY_NONE}},
    {NOP8,  {"NOP", &cpu::funcNop, &cpu::modeImplied, 1, 2, false, true, DUMMY_NONE}},
    {NOP9,  {"NOP", &cpu::funcNop, &cpu::modeImplied, 1, 2, false, true, DUMMY_NONE}},
    {NOP10, {"NOP", &cpu::funcNop, &cpu::modeImplied, 1, 2, false, true, DUMMY_NONE}},
    {NOP11, {"NOP", &cpu::funcNop, &cpu::modeImplied, 3, 4, false, true, DUMMY_NONE}},
    {NOP12, {"NOP", &cpu::funcNop, &cpu::modeImplied, 2, 4, false, true, DUMMY_NONE}},
    {NOP13, {"NOP", &cpu::funcNop, &cpu::modeImplied, 2, 4, false, true, DUMMY_NONE}},
    {NOP14, {"NOP", &cpu::funcNop, &cpu::modeImplied, 2, 4, false, true, DUMMY_NONE}},
    {NOP15, {"NOP", &cpu::funcNop, &cpu::modeImplied, 2, 4, false, true, DUMMY_NONE}},
    {NOP16, {"NOP", &cpu::funcNop, &cpu::modeImplied, 2, 4, false, true, DUMMY_NONE}},
    {NOP17, {"NOP", &cpu::funcNop, &cpu::modeImplied, 2, 4, false, true, DUMMY_NONE}},
    {NOP18, {"NOP", &cpu::funcNop, &cpu::modeImplied, 2, 2, false, true, DUMMY_NONE}},
    {NOP19, {"NOP", &cpu::funcNop, &cpu::modeAbsoluteX, 3, 4, true, true, DUMMY_NONE}},
    {NOP20, {"NOP", &cpu::funcNop, &cpu::modeAbsoluteX, 3, 4, true, true, DUMMY_NONE}},
    {NOP21, {"NOP", &cpu::funcNop, &cpu::modeAbsoluteX, 3, 4, true, true, DUMMY_NONE}},
    {NOP22, {"NOP", &cpu::funcNop, &cpu::modeAbsoluteX, 3, 4, true, true, DUMMY_NONE}},
    {NOP23, {"NOP", &cpu::funcNop, &cpu::modeAbsoluteX, 3, 4, true, true, DUMMY_NONE}},
    {NOP24, {"NOP", &cpu::funcNop, &cpu::modeAbsoluteX, 3, 4, true, true, DUMMY_ONCARRY}},
    {NOP25, {"NOP", &cpu::funcNop, &cpu::modeAbsoluteX, 2, 2, false, true, DUMMY_NONE}},
    {NOP26, {"NOP", &cpu::funcNop, &cpu::modeAbsoluteX, 2, 2, false, true, DUMMY_NONE}},
    {NOP27, {"NOP", &cpu::funcNop, &cpu::modeAbsoluteX, 2, 2, false, true, DUMMY_NONE}},
    {NOP28, {"NOP", &cpu::funcNop, &cpu::modeAbsoluteX, 2, 2, false, true, DUMMY_NONE}},
  };
}

cpu::~cpu()
{
  delete[] memory;
}

void cpu::init(
  boost::shared_ptr<Cartridge> mapper,
  boost::shared_ptr<ppu> ppu)
{
  _mapper = mapper;
  _ppu = ppu;
  _isInitialized = ppu ? true : false;
  _mapper->reset();
}

void cpu::reset()
{
  _mapper->reset();
  setStatusFlag(STATUS_INTERRUPT | STATUS_EMPTY);
  reg_sp -= 3;
  reg_pc = executeInterrupt(Interrupt::Reset);
  interrupts.clear();
}

unsigned short cpu::normalizeAddress(unsigned short address)
{
  address &= 0xFFFF;

  if (address >= 0x0000 && address <= 0x1FFF)
  {
    return address & 0x07FF;
  }
  else if (address >= 0x2000 && address <= 0x3FFF)
  {
    return address & 0x2007;
  }

  return address;
}

unsigned char cpu::read(unsigned short address)
{
  unsigned char value = 0;

  // Handle PRG-ROM reads
  if (address >= 0x8000 && address <= 0xFFFF)
  {
    if (_mapper->readPrgRom(address, value))
    {
      return value;
    }
  }

  address = normalizeAddress(address);

  switch (address)
  {
  case ADDR_PPU_STATUS:
    value = _ppu->readRegisterStatus();
    break;

  case ADDR_PPU_OAM_DATA:

    value = _ppu->readRegisterOAMData();
    break;

  case ADDR_PPU_DATA:
    value = _ppu->readRegisterVRAMData();
    break;

  case ADDR_INPUT_PORT_1:
    value = readController(address & 0x01);
    break;

  case ADDR_INPUT_PORT_2:
    value = readController(address & 0x01);
    break;

  default:
    value = memory[address];
    break;
  }

  return value;
}

void cpu::write(unsigned short address, unsigned char value)
{
  // Handle PRG-ROM writes
  if (address >= 0x8000 && address <= 0xFFFF)
  {
    if (_mapper->writePrgRom(address, value))
    {
      return;
    }
  }

  address = normalizeAddress(address);

  switch (address)
  {
  case ADDR_PPU_CONTROL:
    _ppu->writeRegisterControl(value);
    break;

  case ADDR_PPU_MASK:
    _ppu->writeRegisterMask(value);
    break;

  case ADDR_PPU_OAM_ADDR:
    _ppu->writeRegisterOAMAddr(value);
    break;

  case ADDR_PPU_OAM_DATA:
    _ppu->writeRegisterOAMData(value);
    break;

  case ADDR_PPU_SCROLL:
    _ppu->writeRegisterScroll(value);
    break;

  case ADDR_PPU_ADDR:
    _ppu->writeRegisterVRAMAddr(value);
    break;

  case ADDR_PPU_DATA:
    _ppu->writeRegisterVRAMData(value);
    break;

  case ADDR_DMA:
    _ppu->writeDMA(value);
    break;

  case ADDR_INPUT_PORT_1:
    writeController(address & 0x01, value);
    break;

  case ADDR_INPUT_PORT_2:
    // TODO: APU should generate this IRQ
    /*
    if (value == 0)
    {
      // TODO: generate IRQ?
    }
    */
    break;

  default:
    memory[address] = value;
    break;
  }
}

unsigned char cpu::readController(unsigned char id)
{
  if (id == 0)
  {
    if (controllerReadCount[id] == 19)
    {
      // SIGNATURE
      controllerReadCount[id]++;
      return 0x01;
    }
    else if (controllerReadCount[id] > 7 && controllerReadCount[id] < 16)
    {
      // PLAYER 3
      controllerReadCount[id]++;
      return 0x40;
    }
    else if (controllerReadCount[id] > 15 && controllerReadCount[id] < 23)
    {
      // IGNORED
      controllerReadCount[id]++;
      return 0;
    }
    else if (controllerReadCount[id] == 23)
    {
      controllerReadCount[id] = 0;
      return 0;
    }
    else
    {
      unsigned char value = keyTable[controllerReadCount[id]].pressed;
      controllerReadCount[id]++;
      return value | 0x40;
    }
  }
  else
  {
    if (controllerReadCount[id] == 18)
    {
      // SIGNATURE
      controllerReadCount[id]++;
      return 0x01;
    }
    else if (controllerReadCount[id] > 7 && controllerReadCount[id] < 16)
    {
      // PLAYER 4
      controllerReadCount[id]++;
      return 0x40;
    }
    else if (controllerReadCount[id] > 15 && controllerReadCount[id] < 23)
    {
      // IGNORED
      controllerReadCount[id]++;
      return 0;
    }
    else if (controllerReadCount[id] == 23)
    {
      controllerReadCount[id] = 0;
      return 0;
    }
    else
    {
      unsigned char value = 0;
      controllerReadCount[id]++;
      return value | 0x40;
    }
  }
}

void cpu::writeController(unsigned char controllerId, unsigned char value)
{
  // TODO: support multiple controllers
  if (controllerId != 0)
  {
    return;
  }

  if ((controllerLastWrite % 2 != 0) && (value % 2 == 0))
  {
    controllerReadCount[0] = 0;
    controllerReadCount[1] = 0;
  }

  controllerLastWrite = value;
}

void cpu::updateControllerKeyStatus(SDL_Event event)
{
  for (keyIterator = keyTable.begin(); keyIterator != keyTable.end(); keyIterator++)
  {
    if (keyIterator->code == event.key.keysym.sym)
    {
      keyIterator->pressed = (event.type == SDL_KEYDOWN) ? true : false;
      break;
    }
  }
}

bool cpu::checkTestStatus()
{

  // Is the testsuite ready for status reading?
  if (read(TEST_PREAMBLE_ADDR1) == TEST_PREAMBLE1 &&
    read(TEST_PREAMBLE_ADDR2) == TEST_PREAMBLE2 &&
    read(TEST_PREAMBLE_ADDR3) == TEST_PREAMBLE3)
  {
    // Do we need a reset?
    if (read(TEST_STATUS_ADDR) == TEST_STATUS_NEED_RESET &&
      opcodeHistory == TEST_RESET_SIGNATURE)
    {
      reset();
      return true;
    }
    // Is the testsuite finished?
    else if (read(TEST_STATUS_ADDR) < TEST_STATUS_RUNNING)
    {
      printf("\n\n=====TEST RESULT=======\nStatus code: %X\n", read(TEST_STATUS_ADDR));

      int i = TEST_OUTPUT_ADDR;
      while (memory[i] != 0)
      {
        printf("%c", memory[i]);
        i++;
      }
      printf("\n");
      //exit(0);
    }
  }

  return false;
}

void cpu::log(opcode_entry *entry)
{
  char tmp[16];
  stringstream ss(stringstream::in | stringstream::out);

  sprintf(tmp, "%04X ", reg_pc);
  ss << tmp;

  for (int i = 0; i < (int)entry->bytes; i++)
  {
    sprintf(tmp, "%02X", read(reg_pc + i));
    ss << tmp;

    if (i < entry->bytes - 1)
    {
      ss << " ";
    }
  }

  sprintf(tmp, "\t%s", entry->name.c_str());
  ss << tmp;

  std::string spacing(33 - strlen(tmp), ' ');
  printf("%s%s", ss.str().c_str(), spacing.c_str());
  printf("A:%02X X:%02X Y:%02X P:%02X SP:%02X", reg_acc, reg_index_x, reg_index_y, reg_status, reg_sp);
}

void cpu::pushStack(unsigned char byte)
{
  write(STACK_LOWER + reg_sp--, byte);
}

unsigned char cpu::popStack()
{
  return read(STACK_LOWER + ++reg_sp);
}

unsigned short cpu::executeInterrupt(const enum Interrupt &interrupt)
{
  switch (interrupt)
  {
  case Interrupt::Nmi:
    pushStack((reg_pc >> 8) & 0xFF);
    pushStack(reg_pc & 0xFF);
    pushStack(reg_status & ~STATUS_BRK);
    setStatusFlag(STATUS_INTERRUPT);
    return (read(INTERRUPT_NMI_HIGH) << 8) | read(INTERRUPT_NMI_LOW);
    break;

  case Interrupt::Irq:
    pushStack((reg_pc >> 8) & 0xFF);
    pushStack(reg_pc & 0xFF);
    pushStack(reg_status & ~STATUS_BRK);
    setStatusFlag(STATUS_INTERRUPT);
    return (read(INTERRUPT_IRQ_HIGH) << 8) | read(INTERRUPT_IRQ_LOW);
    break;

  case Interrupt::Brk:
    reg_pc += 2;
    pushStack((reg_pc >> 8) & 0xFF);
    pushStack(reg_pc & 0xFF);
    pushStack(reg_status | STATUS_BRK);
    setStatusFlag(STATUS_INTERRUPT);
    return (read(INTERRUPT_IRQ_HIGH) << 8) | read(INTERRUPT_IRQ_LOW);
    break;

  case Interrupt::Reset:
    return (read(INTERRUPT_RESET_HIGH) << 8) | read(INTERRUPT_RESET_LOW);
    break;

  default:
    break;
  }

  return reg_pc;
}

// Operations
void cpu::funcLoadAccumulator()
{
  value = read(src);
  reg_acc = value;
  setZeroFlag(reg_acc);
  setSignFlag(reg_acc);
}

void cpu::funcLoadRegisterX()
{
  value = read(src);
  reg_index_x = value;
  setSignFlag(reg_index_x);
  setZeroFlag(reg_index_x);
}

void cpu::funcLoadRegisterY()
{
  value = read(src);
  reg_index_y = value;
  setSignFlag(reg_index_y);
  setZeroFlag(reg_index_y);
}

void cpu::funcStoreAccumulator()
{
  write(src, reg_acc);
}

void cpu::funcStoreRegisterX()
{
  write(src, reg_index_x);
}

void cpu::funcStoreRegisterY()
{
  write(src, reg_index_y);
}

void cpu::funcIncreaseRegisterX()
{
  reg_index_x++;
  setSignFlag(reg_index_x);
  setZeroFlag(reg_index_x);
}

void cpu::funcIncreaseRegisterY()
{
  reg_index_y++;
  setSignFlag(reg_index_y);
  setZeroFlag(reg_index_y);
}

void cpu::funcIncreaseMemory()
{
  write(src, read(src) + 1);
  setSignFlag(read(src));
  setZeroFlag(read(src));
}

void cpu::funcDecreaseRegisterX()
{
  reg_index_x--;
  setZeroFlag(reg_index_x);
  setSignFlag(reg_index_x);
}

void cpu::funcDecreaseRegisterY()
{
  reg_index_y--;
  setZeroFlag(reg_index_y);
  setSignFlag(reg_index_y);
}

void cpu::funcDecreaseMemory()
{
  write(src, read(src) - 1);
  setZeroFlag(read(src));
  setSignFlag(read(src));
}

void cpu::funcAnd()
{
  value = read(src);
  result = value & reg_acc;
  setZeroFlag(result);
  setSignFlag(result);
  reg_acc = result & 0xFF;
}

void cpu::funcOr()
{
  value = read(src);
  result = value | reg_acc;
  setZeroFlag(result);
  setSignFlag(result);
  reg_acc = result;
}

void cpu::funcXor()
{
  value = read(src);
  result = reg_acc ^ value;
  setZeroFlag(result);
  setSignFlag(result);
  reg_acc = result;
}

void cpu::funcCompareRegisterX()
{
  value = read(src);
  result = reg_index_x - value;
  testAndSet(reg_index_x >= (value & 0xFF), STATUS_CARRY);
  testAndSet(reg_index_x == (value & 0xFF), STATUS_ZERO);
  setSignFlag(result);
}

void cpu::funcCompareRegisterY()
{
  value = read(src);
  result = reg_index_y - value;
  testAndSet(reg_index_y >= (value & 0xFF), STATUS_CARRY);
  testAndSet(reg_index_y == (value & 0xFF), STATUS_ZERO);
  setSignFlag(result);
}

void cpu::funcCompareMemory()
{
  value = read(src);
  result = reg_acc - value;

  testAndSet(reg_acc >= (value & 0xFF), STATUS_CARRY);
  testAndSet(reg_acc == (value & 0xFF), STATUS_ZERO);
  setSignFlag(result);
}

void cpu::funcADC()
{
  value = read(src);
  result = reg_acc + value + (hasStatusFlag(STATUS_CARRY) ? 1 : 0);
  testAndSet(~(reg_acc ^ value) & (reg_acc ^ result) & 0x80, STATUS_OVERFLOW);
  testAndSet(result & 0x100, STATUS_CARRY);
  reg_acc = result & 0xFF;
  testAndSet(reg_acc == 0, STATUS_ZERO);
  testAndSet((reg_acc >> 7) & 1, STATUS_SIGN);
}

void cpu::funcSBC()
{
  value = read(src);
  result = reg_acc + ~value + (hasStatusFlag(STATUS_CARRY) ? 1 : 0);
  testAndSet((reg_acc ^ value) & (reg_acc ^ result) & 0x80, STATUS_OVERFLOW);
  testAndSet(!(result & 0x100), STATUS_CARRY);
  reg_acc = result & 0xFF;
  testAndSet(reg_acc == 0, STATUS_ZERO);
  testAndSet((reg_acc >> 7) & 1, STATUS_SIGN);
}

void cpu::funcShiftRightToAccumulator()
{
  value = reg_acc;
  unsigned short result = value >> 1;
  testAndSet(value & 0x1, STATUS_CARRY);
  setZeroFlag(result);
  setSignFlag(result);
  reg_acc = result;
}

void cpu::funcShiftRightToMemory()
{
  value = read(src);
  unsigned short result = value >> 1;

  testAndSet(value & 0x1, STATUS_CARRY);
  setZeroFlag(result);
  setSignFlag(result);
  write(src, result);
}

void cpu::funcShiftLeftToAccumulator()
{
  value = reg_acc;
  value <<= 1;
  setCarryFlag(value);
  setZeroFlag(value);
  setSignFlag(value);
  reg_acc = value;
}

void cpu::funcShiftLeftToMemory()
{
  value = read(src);
  value <<= 1;
  setCarryFlag(value);
  setZeroFlag(value);
  setSignFlag(value);
  write(src, value);
}

void cpu::funcRotateRightToAccumulator()
{
  value = reg_acc;
  unsigned short result = (value >> 1) | ((reg_status & STATUS_CARRY) << 7);

  testAndSet(value & 0x1, STATUS_CARRY);
  setZeroFlag(result);
  setSignFlag(result);
  reg_acc = result;
}

void cpu::funcRotateRightToMemory()
{
  value = read(src);
  unsigned short result = (value >> 1) | ((reg_status & STATUS_CARRY) << 7);

  testAndSet(value & 0x1, STATUS_CARRY);
  setZeroFlag(result);
  setSignFlag(result);
  write(src, result);
}

void cpu::funcRotateLeftToAccumulator()
{
  value = reg_acc;
  unsigned short result = (value << 1) | (reg_status & STATUS_CARRY);

  setCarryFlag(result);
  setZeroFlag(result);
  setSignFlag(result);
  reg_acc = result;
}

void cpu::funcRotateLeftToMemory()
{
  value = read(src);
  unsigned short result = (value << 1) | (reg_status & STATUS_CARRY);

  setCarryFlag(result);
  setZeroFlag(result);
  setSignFlag(result);
  write(src, result);
}

void cpu::funcBreak()
{
  enqueueInterrupt(Interrupt::Brk);
}

void cpu::funcSetInterruptDisable()
{
  setStatusFlag(STATUS_INTERRUPT);
}

void cpu::funcSetDecimalMode()
{
  setStatusFlag(STATUS_DECIMAL);
}

void cpu::funcSetCarryFlag()
{
  setStatusFlag(STATUS_CARRY);
}

void cpu::funcJumpSaveReturnAddress()
{
  unsigned short ret = reg_pc + 2;
  pushStack((ret >> 8) & 0xFF);
  pushStack(ret & 0xFF);
  reg_pc = src;
}

void cpu::funcReturnFromSubroutine()
{
  unsigned char low = popStack();
  unsigned char high = popStack();
  reg_pc = ((high << 8) | low) + 1;
}

void cpu::funcReturnFromInterrupt()
{
  reg_status = popStack();
  unsigned char low = popStack() & 0xFF;
  unsigned char high = popStack() & 0xFF;
  reg_pc = (high << 8) | low;
}

void cpu::funcBranchResultNotZero()
{
  if (!hasStatusFlag(STATUS_ZERO))
  {
    branchTaken = true;
    reg_pc = modeRelative(DUMMY_NONE);
  }
}

void cpu::funcBranchResultZero()
{
  if (hasStatusFlag(STATUS_ZERO))
  {
    branchTaken = true;
    reg_pc = modeRelative(DUMMY_NONE);
  }
}

void cpu::funcBranchCarrySet()
{
  if (hasStatusFlag(STATUS_CARRY))
  {
    branchTaken = true;
    reg_pc = modeRelative(DUMMY_NONE);
  }
}

void cpu::funcBranchCarryClear()
{
  if (!hasStatusFlag(STATUS_CARRY))
  {
    branchTaken = true;
    reg_pc = modeRelative(DUMMY_NONE);
  }
}

void cpu::funcBranchResultMinus()
{
  if (hasStatusFlag(STATUS_SIGN))
  {
    branchTaken = true;
    reg_pc = modeRelative(DUMMY_NONE);
  }
}

void cpu::funcBranchResultPlus()
{
  if (!hasStatusFlag(STATUS_SIGN))
  {
    branchTaken = true;
    reg_pc = modeRelative(DUMMY_NONE);
  }
}

void cpu::funcBranchOverflowClear()
{
  if (!hasStatusFlag(STATUS_OVERFLOW))
  {
    branchTaken = true;
    reg_pc = modeRelative(DUMMY_NONE);
  }
}

void cpu::funcBranchOverflowSet()
{
  if (hasStatusFlag(STATUS_OVERFLOW))
  {
    branchTaken = true;
    reg_pc = modeRelative(DUMMY_NONE);
  }
}

void cpu::funcNop()
{
}

void cpu::funcTransferIndexXToStackPointer()
{
  reg_sp = reg_index_x;
}

void cpu::funcTransferIndexXToAccumulator()
{
  reg_acc = reg_index_x;
  setZeroFlag(reg_index_x);
  setSignFlag(reg_index_x);
}

void cpu::funcTransferStackPointerToIndexX()
{
  reg_index_x = reg_sp;
  setZeroFlag(reg_index_x);
  setSignFlag(reg_index_x);
}

void cpu::funcTransferAccumulatorToIndexY()
{
  reg_index_y = reg_acc;
  setZeroFlag(reg_index_y);
  setSignFlag(reg_index_y);
}

void cpu::funcTransferAccumulatorToIndexX()
{
  reg_index_x = reg_acc;
  setZeroFlag(reg_index_x);
  setSignFlag(reg_index_x);
}

void cpu::funcTransferIndexYToAccumulator()
{
  reg_acc = reg_index_y;
  setZeroFlag(reg_index_y);
  setSignFlag(reg_index_y);
}

void cpu::funcClearCarryFlag()
{
  clearStatusFlag(STATUS_CARRY);
}

void cpu::funcClearDecimalMode()
{
  clearStatusFlag(STATUS_DECIMAL);
}

void cpu::funcClearInterruptDisable()
{
  clearStatusFlag(STATUS_INTERRUPT);
}

void cpu::funcClearOverflowFlag()
{
  clearStatusFlag(STATUS_OVERFLOW);
}

void cpu::funcJump()
{
  reg_pc = src;
}

void cpu::funcBit()
{
  value = read(src);
  testAndSet((value >> 6) & 1, STATUS_OVERFLOW);
  testAndSet((value >> 7) & 1, STATUS_SIGN);
  testAndSet((value & reg_acc) == 0, STATUS_ZERO);
}

void cpu::funcPushStatusToStack()
{
  pushStack(reg_status | STATUS_BRK | STATUS_EMPTY);
}

void cpu::funcPopStatusFromStack()
{
  reg_status = popStack();
  clearStatusFlag(STATUS_BRK);
}

void cpu::funcPushAccumulatorToStack()
{
  pushStack(reg_acc);
}

void cpu::funcPopAccumulatorFromStack()
{
  reg_acc = popStack();
  setZeroFlag(reg_acc);
  setSignFlag(reg_acc);
}

void cpu::funcLAX()
{
  funcLoadAccumulator();
  funcLoadRegisterX();
}

void cpu::funcDCP()
{
  funcDecreaseMemory();
  funcCompareMemory();
}

void cpu::funcSLO()
{
  funcShiftLeftToMemory();
  funcOr();
}

void cpu::funcRLA()
{
  funcRotateLeftToMemory();
  funcAnd();
}

void cpu::funcSRE()
{
  funcShiftRightToMemory();
  funcXor();
}

void cpu::funcRRA()
{
  funcRotateRightToMemory();
  funcADC();
}

void cpu::funcISC()
{
  funcIncreaseMemory();
  funcSBC();
}

void cpu::funcSAX()
{
  write(src, reg_acc & reg_index_x);
}

void cpu::funcANC()
{
  funcAnd();
  reg_status &= ~STATUS_CARRY;

  if (result & STATUS_SIGN)
  {
    reg_status |= STATUS_CARRY;
  }
}

void cpu::funcALR()
{
  reg_acc &= read(src);
  testAndSet(reg_acc & 0x01, STATUS_CARRY);
  reg_acc >>= 1;
  testAndSet(reg_acc == 0, STATUS_ZERO);
  clearStatusFlag(STATUS_SIGN);
}

void cpu::funcARR()
{
  reg_acc = ((reg_acc & read(src)) >> 1) | (hasStatusFlag(STATUS_CARRY) << 7);
  testAndSet(reg_acc == 0, STATUS_ZERO);
  testAndSet((reg_acc >> 7) & 1, STATUS_SIGN);
  testAndSet((reg_acc >> 6) & 1, STATUS_CARRY);
  testAndSet(hasStatusFlag(STATUS_CARRY) ^ ((reg_acc >> 5) & 1), STATUS_OVERFLOW);
}

void cpu::funcAXS()
{
  int result = (reg_acc & reg_index_x) - read(src);
  testAndSet(result >= 0, STATUS_CARRY);
  testAndSet(result == 0, STATUS_ZERO);
  reg_index_x = result & 0xFF;
  testAndSet((result >> 7) & 1, STATUS_SIGN);
}

// Based on code from halfnes
void cpu::funcSHY()
{
  result = (reg_index_y & ((src >> 8) + 1)) & 0xFF;
  value = (src - reg_index_x) & 0xFF;

  if ((reg_index_x + value) <= 0xFF) {
    write(src, result);
  }
  else
  {
    write(src, read(src));
  }
}

// Based on code from halfnes
void cpu::funcSHX()
{
  result = (reg_index_x & ((src >> 8) + 1)) & 0xFF;
  value = (src - reg_index_y) & 0xFF;

  if ((reg_index_y + value) <= 0xFF) {
    write(src, result);
  }
  else
  {
    write(src, read(src));
  }
}

// ADDRESS MODES
void cpu::setPageBoundaryCrossed(unsigned short address1, unsigned short address2)
{
  setPageBoundaryCrossed(address1, address2, DUMMY_NONE);
}

void cpu::setPageBoundaryCrossed(unsigned short address1, unsigned short address2, unsigned char dummy)
{
  if ((address1 ^ address2) & 0xFF00)
  {
    pageBoundaryCrossed = true;

    if (dummy == DUMMY_ONCARRY)
    {
      read((address1 & 0xFF00) | (address2 & 0xFF));
    }
  }

  if (dummy == DUMMY_ALWAYS)
  {
    read((address1 & 0xFF00) | (address2 & 0xFF));
  }
}

unsigned short cpu::modeAbsolute(unsigned char dummy)
{
  return (read(reg_pc + 2) << 8) | read(reg_pc + 1);
}

unsigned short cpu::modeImplied(unsigned char dummy)
{
  return 0;
}

unsigned short cpu::modeImm(unsigned char dummy)
{
  return reg_pc + 1;
}

unsigned short cpu::modeAbsoluteX(unsigned char dummy)
{
  address = modeAbsolute(dummy);
  result = address + reg_index_x;
  setPageBoundaryCrossed(address, result, dummy);
  return result;
}

unsigned short cpu::modeAbsoluteY(unsigned char dummy)
{
  address = modeAbsolute(dummy);
  result = address + reg_index_y;
  setPageBoundaryCrossed(address, result, dummy);
  return result;
}

unsigned short cpu::modeAbsoluteZeroPage(unsigned char dummy)
{
  return read(reg_pc + 1) & 0xFF;
}

unsigned short cpu::modeAbsoluteXZeroPage(unsigned char dummy)
{
  return (modeAbsoluteZeroPage(dummy) + reg_index_x) & 0xFF;
}

unsigned short cpu::modeAbsoluteYZeroPage(unsigned char dummy)
{
  return (modeAbsoluteZeroPage(dummy) + reg_index_y) & 0xFF;
}

unsigned short cpu::modeIndirect(unsigned char dummy)
{
  unsigned short ref = ((read(reg_pc + 2) & 0xFF) << 8) | (read(reg_pc + 1) & 0xFF);
  unsigned short ref2 = ref + 1;
  unsigned short low = read(ref);
  unsigned short high = read(ref2);

  // Take care of the 6502's famous indirect jump bug
  if ((ref & 0xFF) == 0xFF)
  {
    high = read(ref & 0xFF00);
  }

  return (high << 8) | low;
}

unsigned short cpu::modePostIndirectY(unsigned char dummy)
{
  unsigned short ref = read(reg_pc + 1) & 0xFF;
  unsigned short ref2 = (ref + 1) & 0xFF;
  unsigned short low = read(ref);
  unsigned short high = read(ref2);
  address = ((high << 8) | low);
  result = address + reg_index_y;
  setPageBoundaryCrossed(address, result, dummy);
  return result;
}

unsigned short cpu::modePreIndirectX(unsigned char dummy)
{
  unsigned short ref = (read(reg_pc + 1) + reg_index_x) & 0xFF;
  unsigned short ref2 = (ref + 1) & 0xFF;
  unsigned short low = read(ref);
  unsigned short high = read(ref2);
  return (high << 8) | low;
}

unsigned short cpu::modeRelative(unsigned char dummy)
{
  signed short offset = (signed char)read(reg_pc + 1);
  address = reg_pc;
  result = address + offset;
  setPageBoundaryCrossed(address, result);
  return result;
}

// Helpers
bool cpu::hasStatusFlag(unsigned char flag)
{
  return reg_status & flag;
}

void cpu::setStatusFlag(unsigned char flags)
{
  reg_status |= flags;
}

void cpu::clearStatusFlag(unsigned char flags)
{
  reg_status &= ~flags;
}

void cpu::testAndSet(bool expr, unsigned char flags)
{
  if (expr)
    setStatusFlag(flags);
  else
    clearStatusFlag(flags);
}

void cpu::setSignFlag(unsigned short src)
{
  if (src & 0x0080)
    setStatusFlag(STATUS_SIGN);
  else
    clearStatusFlag(STATUS_SIGN);
}

void cpu::setZeroFlag(unsigned short src)
{
  if (src & 0x00FF)
    clearStatusFlag(STATUS_ZERO);
  else
    setStatusFlag(STATUS_ZERO);
}

void cpu::setCarryFlag(unsigned short src)
{
  if (src & 0xFF00)
    setStatusFlag(STATUS_CARRY);
  else
    clearStatusFlag(STATUS_CARRY);
}

void cpu::setOverflowFlag(unsigned short value, unsigned short mem)
{
  if ((value ^ reg_acc) & (value ^ mem) & 0x80)
    setStatusFlag(STATUS_OVERFLOW);
  else
    clearStatusFlag(STATUS_OVERFLOW);
}
