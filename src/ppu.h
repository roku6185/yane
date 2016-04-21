#ifndef _PPU_H_
#define _PPU_H_

#include <vector>
#include "ines.h"

class cpu;
class Cartridge;
class Renderer;

#define SCREEN_WIDTH      256
#define SCREEN_HEIGHT      240
#define SCREEN_BPP        32
#define SCREEN_UPDATE_TIME_IN_NS  16666666  // 60 Hz

#define VRAM_SIZE      16384
#define SPRITE_RAM_SIZE    256
#define NAME_TABLE_SIZE    960
#define NAME_TABLE_WIDTH  32
#define NAME_TABLE_HEIGHT  30
#define GROUP_WIDTH      8
#define GROUP_HEIGHT    8
#define TILE_WIDTH      8
#define TILE_HEIGHT      8

#define ADDR_PATTERN_TABLE0    0x0000
#define ADDR_PATTERN_TABLE1    0x1000

#define ADDR_PALETTE_BG      0x3F00
#define ADDR_PALETTE_SPRITE    0x3F10
#define SPRITE_FIRST_ENTRY    252
#define SPRITE_ZERO       0
#define SPRITE_ENTRY_SIZE    4
#define SPRITE_OVERFLOW_COUNT  8

#define DMA_CYCLES       512

#define PPU_PER_CPU_CYCLE    3
#define PPU_PER_SCANLINE    341

#define SCANLINE_RENDER_START  0
#define SCANLINE_RENDER_END    239
#define SCANLINE_VBLANK_START  240
#define SCANLINE_VBLANK_END    259
#define SCANLINE_FRAME_END    261
#define SCANLINE_INIT      241

#define PPU_CONTROL_NAMETABLE_ADDR1   0x01
#define PPU_CONTROL_NAMETABLE_ADDR2   0x02
#define PPU_CONTROL_VRAM_ADDR_INCR   0x04
#define PPU_CONTROL_SPRITE_PATTERN_ADDR 0x08
#define PPU_CONTROL_BG_PATTERN_ADDR   0x10
#define PPU_CONTROL_SPRITE_SIZE     0x20
#define PPU_CONTROL_PPU_SELECT     0x40
#define PPU_CONTROL_NMI         0x80

#define PPU_MASK_GRAYSCALE    0x01
#define PPU_MASK_BG_LEFT    0x02
#define PPU_MASK_SPRITE_LEFT  0x04
#define PPU_MASK_SHOW_BG    0x08
#define PPU_MASK_SHOW_SPRITES  0x10
#define PPU_MASK_INTENSE_RED  0x20
#define PPU_MASK_INTENSE_GREEN  0x40
#define PPU_MASK_INTENSE_BLUE  0x80

#define PPU_STATUS_LAST_PPU_WRITES1  0x01
#define PPU_STATUS_LAST_PPU_WRITES2  0x02
#define PPU_STATUS_LAST_PPU_WRITES3  0x04
#define PPU_STATUS_LAST_PPU_WRITES4  0x08
#define PPU_STATUS_LAST_PPU_WRITES5  0x10
#define PPU_STATUS_SPRITE_OVERFLOW  0x20
#define PPU_STATUS_SPRITE_ZERO_HIT  0x40
#define PPU_STATUS_VBLANK_STARTED  0x80

#define SPRITE_ATTR_COLOR      0x03
#define SPRITE_ATTR_BG_PRIO      0x20
#define SPRITE_ATTR_HORIZONTAL_FLIP 0x40
#define SPRITE_ATTR_VERTICAL_FLIP  0x80

typedef struct
{
  unsigned char r;
  unsigned char g;
  unsigned char b;
} palette_entry;

class ppu
{
public:
  ppu();
  ~ppu();
  void init(Cartridge *mapper, Renderer *renderer, cpu *cpu);
  bool isInitialized() { return _isInitialized; }
  void start();
  void stop();
  void reset();
  void execute(unsigned short cycles);

  // Read operations
  unsigned char readRegisterStatus();
  unsigned char readRegisterOAMData();
  unsigned char readRegisterVRAMData();

  // Write operations
  void writeRegisterControl(unsigned char value);
  void writeRegisterMask(unsigned char value);
  void writeRegisterOAMAddr(unsigned char value);
  void writeRegisterOAMData(unsigned char value);
  void writeRegisterScroll(unsigned char value);
  void writeRegisterVRAMAddr(unsigned char value);
  void writeRegisterVRAMData(unsigned char value);
  void writeDMA(unsigned char value);

private:
  Cartridge *_mapper;
  Renderer *_renderer;
  cpu *_cpu;
  bool _isInitialized;

  unsigned char *video_memory;
  unsigned char *sprite_memory;
  unsigned short vram_address;
  unsigned short vram_latch;
  unsigned char scroll_x;
  unsigned char scroll_y;
  unsigned char sprite_address;
  std::vector<palette_entry> palette_table;
  std::vector<unsigned char> defaultPalette;

  bool transferLatch;
  bool transferLatchScroll;
  bool vram_access_flipflop;
  unsigned char ppu_control;
  unsigned char ppu_mask;
  unsigned char ppu_status;
  unsigned char ppu_oam_addr;
  unsigned char ppu_oam_data;
  unsigned char ppu_scroll_origin;
  unsigned char ppu_addr;
  unsigned char ppu_data;
  unsigned char vramDataLatch;

  bool isVblank;
  bool isNmiExecuted;
  bool ppuSelection;
  bool spriteSize;
  bool bgPattern;
  bool spritePattern;
  unsigned char spritesOnScanline;

  timespec lastScreenUpdate, now, diff;
  unsigned short ppuCycles;
  unsigned short scanline;
  unsigned short previousScanline;

  unsigned char read(unsigned short address);
  void write(unsigned short address, unsigned char value);
  unsigned char read();
  void write(unsigned char value);
  unsigned short normalizeAddress(unsigned short address);
  unsigned short mirrorNameTables(unsigned short address);

  void renderToBuffer();
  void renderBackground();
  void renderSprites(unsigned char backgroundPriority = 0);
  void renderTile8x8(unsigned char backgroundPriority);
  void renderTile8x16(unsigned char backgroundPriority);
  void updateScreen();

  void log();
};

#endif
