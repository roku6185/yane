#include <iostream>
#include <string.h>

#include "cartridge.h"
#include "renderer.h"
#include "ppu.h"
#include "cpu.h"
#include "config.h"
#include "yane.h"
#include "yane_exception.h"
#include "utils.h"

using namespace std;

ppu::ppu()
{
  _mapper = nullptr;
  _cpu = nullptr;
  _isInitialized = false;

  isVblank = false;
  isNmiExecuted = false;

  transferLatch = false;
  transferLatchScroll = false;

  // Initiate RAM
  video_memory = new unsigned char[VRAM_SIZE];
  sprite_memory = new unsigned char[SPRITE_RAM_SIZE];
  memset(video_memory, 0, VRAM_SIZE);
  memset(sprite_memory, 0, SPRITE_RAM_SIZE);
  vram_access_flipflop = false;
  vram_address = 0;
  vramDataLatch = 0;

  // Initialize PPU registers
  ppu_control = 0;
  ppu_mask = 0;
  ppu_status = 0;
  ppu_oam_addr = 0;
  ppu_oam_data = 0;
  ppu_scroll_origin = 0;
  ppu_addr = 0;
  ppu_data = 0;

  // Palette entries at PPU startup
  defaultPalette =
  {
    0x09, 0x01, 0x00, 0x01,
    0x00, 0x02, 0x02, 0x0D,
    0x08, 0x10, 0x08, 0x24,
    0x00, 0x00, 0x04, 0x2C,
    0x09, 0x01, 0x34, 0x03,
    0x00, 0x04, 0x00, 0x14,
    0x08, 0x3A, 0x00, 0x02,
    0x00, 0x20, 0x2C, 0x08,
  };

  // System palette
  palette_table =
  {
    {124,124,124}, {0,0,252}, {0,0,188}, {68,40,188},
    {148,0,132}, {168,0,32}, {168,16,0}, {136,20,0},
    {80,48,0}, {0,120,0}, {0,104,0}, {0,88,0},
    {0,64,88}, {0,0,0}, {0,0,0}, {0,0,0},
    {188,188,188}, {0,120,248},  {0,88,248}, {104,68,252},
    {216,0,204}, {228,0,88}, {248,56,0}, {228,92,16},
    {172,124,0}, {0,184,0}, {0,168,0}, {0,168,68},
    {0,136,136}, {0,0,0}, {0,0,0}, {0,0,0},
    {248,248,248}, {60,188,252}, {104,136,252}, {152,120,248},
    {248,120,248}, {248,88,152}, {248,120,88}, {252,160,68},
    {248,184,0}, {184,248,24}, {88,216,84}, {88,248,152},
    {0,232,216}, {120,120,120}, {0,0,0}, {0,0,0},
    {252,252,252}, {164,228,252}, {184,184,248}, {216,184,248},
    {248,184,248}, {248,164,192}, {240,208,176}, {252,224,168},
    {248,216,120}, {216,248,120}, {184,248,184}, {184,248,216},
    {0,252,252}, {248,216,248}, {0,0,0}, {0,0,0},
  };
}

ppu::~ppu()
{
  delete[] video_memory;
  delete[] sprite_memory;
}

void ppu::init(
  boost::shared_ptr<Cartridge> mapper,
  boost::shared_ptr<Renderer> renderer,
  boost::shared_ptr<cpu> cpu)
{
  _mapper = mapper;
  _renderer = renderer;
  _isInitialized = cpu ? true : false;
  _cpu = cpu;

  renderer->init();
}

void ppu::start()
{
  BOOST_ASSERT_MSG(_cpu, "PPU is not initialized");

  clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &lastScreenUpdate);
  scanline = previousScanline = SCANLINE_INIT;
  ppuCycles = 0;

  // Fill palette region in VRAM with default values
  int i = 0;

  for (std::vector<unsigned char>::iterator it = defaultPalette.begin(); it != defaultPalette.end(); ++it)
  {
    write(ADDR_PALETTE_BG + i++, *it);
  }
}

void ppu::stop()
{
  _renderer->cleanup();
}

void ppu::reset()
{
  scanline = SCANLINE_INIT;
  ppuCycles = 0;
}

void ppu::execute(unsigned short cycles)
{
  log();

  // Calculate PPU cycles and scanline information
  unsigned short ppuCyclesInstruction = cycles * PPU_PER_CPU_CYCLE;
  ppuCycles += ppuCyclesInstruction;

  if (ppuCycles >= PPU_PER_SCANLINE)
  {
    scanline += ppuCycles / PPU_PER_SCANLINE;
    ppuCycles = ppuCycles % PPU_PER_SCANLINE;
  }

  // Rendering time (240 scanlines)?
  if (scanline >= SCANLINE_RENDER_START && scanline <= SCANLINE_RENDER_END)
  {
    if (!transferLatch &&
      ((ppu_mask & PPU_MASK_SHOW_SPRITES) || (ppu_mask & PPU_MASK_SHOW_BG))
    )
    {
      vram_address = vram_latch;
      transferLatch = true;
    }

    if (scanline != previousScanline)
    {
      renderToBuffer();
      previousScanline = scanline;

      if ((ppu_mask & PPU_MASK_SHOW_SPRITES) || (ppu_mask & PPU_MASK_SHOW_BG)
      )
      {
        vram_address = (vram_address & (~0x1F & ~(1 << 10))) | (vram_latch & (0x1F | (1 << 10)));
        _mapper->irqTick();
      }
    }
  }
  // Start of vblank?
  else if (scanline == SCANLINE_VBLANK_START)
  {
    if (!isVblank)
    {
      isVblank = true;
      ppu_status |= PPU_STATUS_VBLANK_STARTED;
    }
  }
  // End of frame?
  else if (scanline > SCANLINE_FRAME_END)
  {
    ppu_status &= ~PPU_STATUS_SPRITE_OVERFLOW;
    ppu_status &= ~PPU_STATUS_SPRITE_ZERO_HIT;

    // Update screen
    updateScreen();

    transferLatch = false;
    transferLatchScroll = false;
    isNmiExecuted = false;
    scanline = previousScanline = 0;
  }
  // End of vblank?
  else if (scanline > SCANLINE_VBLANK_END)
  {
    if (isVblank)
    {
      isVblank = false;
      ppu_status &= ~PPU_STATUS_VBLANK_STARTED;
      //vram_address &= ~0x7BE0;
      //vram_address |= (vram_latch & 0x7BE0);
    }
  }

  if (isVblank &&
    !isNmiExecuted &&
    (ppu_control & PPU_CONTROL_NMI) &&
    (ppu_status & PPU_STATUS_VBLANK_STARTED)
   )
   {
     isNmiExecuted = true;
     _cpu->enqueueInterrupt(Interrupt::Nmi);
   }
}

void ppu::log()
{
  if (Config::instance().doInstructionLogging)
  {
    printf(" CYC:%3d SL:%d\n", ppuCycles, scanline);
  }
}

void ppu::renderToBuffer()
{
  if ((ppu_mask & (PPU_MASK_SHOW_SPRITES | PPU_MASK_SHOW_BG)) == 0)
  {
    return;
  }

  renderSprites(SPRITE_ATTR_BG_PRIO);
  renderBackground();
  renderSprites();
}

void ppu::renderSprites(unsigned char backgroundPriority)
{
  if (ppu_control & PPU_CONTROL_SPRITE_SIZE)
  {
    renderTile8x16(backgroundPriority);
  }
  else
  {
    renderTile8x8(backgroundPriority);
  }
}

void ppu::renderTile8x8(unsigned char backgroundPriority)
{
  unsigned short patternTableAddr;
  unsigned char patternPlane1, patternPlane2, paletteIndex, paletteUpperBits;
  unsigned char inRange, spriteX, spriteY, tileIndex, spriteAttribute;

  spritesOnScanline = 0;

  if (ppu_control & PPU_CONTROL_SPRITE_PATTERN_ADDR)
  {
    patternTableAddr = ADDR_PATTERN_TABLE1;
  }
  else
  {
    patternTableAddr = ADDR_PATTERN_TABLE0;
  }

  // Iterate through sprites (lowest priority first)
  for (int spriteNum = SPRITE_FIRST_ENTRY; spriteNum >= 0; spriteNum -= SPRITE_ENTRY_SIZE)
  {
    tileIndex = sprite_memory[spriteNum + 1];
    spriteAttribute = sprite_memory[spriteNum + 2];
    spriteX = sprite_memory[spriteNum + 3];
    spriteY = sprite_memory[spriteNum] + 1;

    if (sprite_memory[spriteNum] == 239)
    {
      ppu_status |= PPU_STATUS_SPRITE_OVERFLOW;
    }
    else if (sprite_memory[spriteNum] == 255)
    {
      ppu_status &= ~PPU_STATUS_SPRITE_OVERFLOW;
    }

    // Skip sprite if other priority
    if (backgroundPriority != (spriteAttribute & SPRITE_ATTR_BG_PRIO))
    {
      continue;
    }

    // Is the sprite positioned within the current scanline?
    inRange = scanline - spriteY;

    if (inRange < 8)
    {
      // Sprite overflow?
      if (spritesOnScanline++ >= SPRITE_OVERFLOW_COUNT)
      {
        ppu_status |= PPU_STATUS_SPRITE_OVERFLOW;
      }

      // Flip vertically?
      if (spriteAttribute & SPRITE_ATTR_VERTICAL_FLIP)
      {
        inRange ^= 0x07;
      }

      // Get pattern data
      patternPlane1 = read(patternTableAddr + (tileIndex << 4) + inRange);
      patternPlane2 = read(patternTableAddr + (tileIndex << 4) + inRange + 8);
      paletteUpperBits = (spriteAttribute & SPRITE_ATTR_COLOR) << 2;

      for (int pixelNum = 0; pixelNum < TILE_WIDTH; pixelNum++)
      {
        paletteIndex = paletteUpperBits;

        // Flip horizontally?
        if (spriteAttribute & SPRITE_ATTR_HORIZONTAL_FLIP)
        {
          paletteIndex |= patternPlane1 & (0x01 << pixelNum) ? 0x1 : 0;
          paletteIndex |= patternPlane2 & (0x01 << pixelNum) ? 0x2 : 0;
        }
        else
        {
          paletteIndex |= patternPlane1 & (0x80 >> pixelNum) ? 0x1 : 0;
          paletteIndex |= patternPlane2 & (0x80 >> pixelNum) ? 0x2 : 0;
        }

        // If color bits from pattern tables are zero => transparent pixel
        if ((paletteIndex & 0x3) == 0)
        {
          //setTransparentPixel(backgroundPriority ? sprBg : sprFg, spriteX + pixelNum, scanline);
        }
        else
        {
          if (spriteNum == SPRITE_ZERO &&
            (ppu_mask & PPU_MASK_SHOW_SPRITES) &&
            (ppu_mask & PPU_MASK_SHOW_BG))
          {
            if (!_renderer->isTransparentPixel(spriteX + pixelNum, scanline))
            {
              ppu_status |= PPU_STATUS_SPRITE_ZERO_HIT;
            }
          }

          _renderer->setPixel(backgroundPriority ? PixelType::BackgroundSprite : PixelType::ForegroundSprite, spriteX + pixelNum, scanline, palette_table[read(ADDR_PALETTE_SPRITE + paletteIndex)]);
        }
      }
    }
  }
}

// Based on 8x16 rendering from My Nes
void ppu::renderTile8x16(unsigned char backgroundPriority)
{
  unsigned short patternTableAddr;
  unsigned char patternPlane1, patternPlane2, paletteIndex, paletteUpperBits;
  unsigned char inRange, spriteX, spriteY, tileIndex, spriteAttribute;

  spritesOnScanline = 0;

  // Iterate through sprites (lowest priority first)
  for (int spriteNum = SPRITE_FIRST_ENTRY; spriteNum >= 0; spriteNum -= SPRITE_ENTRY_SIZE)
  {
    tileIndex = sprite_memory[spriteNum + 1];
    spriteAttribute = sprite_memory[spriteNum + 2];
    spriteX = sprite_memory[spriteNum + 3];
    spriteY = sprite_memory[spriteNum] + 1;
    int spriteId = tileIndex;

    if (sprite_memory[spriteNum] == 239)
    {
      ppu_status |= PPU_STATUS_SPRITE_OVERFLOW;
    }
    else if (sprite_memory[spriteNum] == 255)
    {
      ppu_status &= ~PPU_STATUS_SPRITE_OVERFLOW;
    }

    // Skip sprite if other priority
    if (backgroundPriority != (spriteAttribute & SPRITE_ATTR_BG_PRIO))
    {
      continue;
    }

    // Is the sprite positioned within the current scanline?
    inRange = scanline - spriteY;

    // Flip vertically?
    if (spriteAttribute & SPRITE_ATTR_VERTICAL_FLIP)
    {
      inRange = spriteY + 15 - scanline;
    }

    if (inRange < 16)
    {
      if (inRange < 8)
      {
        if (spriteId % 2 == 0)
        {
          patternTableAddr = 0x0000;
        }
        else
        {
          patternTableAddr = 0x1000;
          tileIndex -= 1;
        }
      }
      else
      {
        inRange += 8;

        if (spriteId % 2 == 0)
        {
          patternTableAddr = 0x0000;
          tileIndex += 1;
        }
        else
        {
          patternTableAddr = 0x1000;
          tileIndex -= 1;
        }
      }

      // Get pattern data
      patternPlane1 = read(patternTableAddr + (tileIndex << 4) + inRange);
      patternPlane2 = read(patternTableAddr + (tileIndex << 4) + inRange + 8);
      paletteUpperBits = (spriteAttribute & SPRITE_ATTR_COLOR) << 2;

      for (int pixelNum = 0; pixelNum < TILE_WIDTH; pixelNum++)
      {
        paletteIndex = paletteUpperBits;

        // Flip horizontally?
        if (spriteAttribute & SPRITE_ATTR_HORIZONTAL_FLIP)
        {
          paletteIndex |= patternPlane1 & (0x01 << pixelNum) ? 0x1 : 0;
          paletteIndex |= patternPlane2 & (0x01 << pixelNum) ? 0x2 : 0;
        }
        else
        {
          paletteIndex |= patternPlane1 & (0x80 >> pixelNum) ? 0x1 : 0;
          paletteIndex |= patternPlane2 & (0x80 >> pixelNum) ? 0x2 : 0;
        }

        // If color bits from pattern tables are zero => transparent pixel
        if ((paletteIndex & 0x3) == 0)
        {
          //setTransparentPixel(backgroundPriority ? sprBg : sprFg, spriteX + pixelNum, scanline);
        }
        else
        {
          if (spriteNum == SPRITE_ZERO &&
            (ppu_mask & PPU_MASK_SHOW_SPRITES) &&
            (ppu_mask & PPU_MASK_SHOW_BG))
          {
            if (!_renderer->isTransparentPixel(spriteX + pixelNum, scanline))
            {
              ppu_status |= PPU_STATUS_SPRITE_ZERO_HIT;
            }
          }

          _renderer->setPixel(backgroundPriority ? PixelType::BackgroundSprite : PixelType::ForegroundSprite, spriteX + pixelNum, scanline, palette_table[read(ADDR_PALETTE_SPRITE + paletteIndex)]);
        }
      }
    }
  }
}

void ppu::renderBackground()
{
  unsigned short nameTableAddr, patternTableAddr, attributeTableAddr, tileAddress, attributeAddress;
  unsigned char attributeValue, tileIndex, groupIndex, paletteIndex;
  unsigned char patternPlane1, patternPlane2, paletteUpperBits;
  unsigned char tileX, tileY, tileScrollY, tileScrollX;

  // Select base address for pattern table
  if (ppu_control & PPU_CONTROL_BG_PATTERN_ADDR)
  {
    patternTableAddr = 0x1000;
  }
  else
  {
    patternTableAddr = 0x0000;
  }

  // Iterate through tiles in name table
  for (int tileNum = 0; tileNum < NAME_TABLE_WIDTH; tileNum++)
  {
    switch ((vram_address >> 10) & 0x03)
    {
      case 0: nameTableAddr = 0x2000; break;
      case 1: nameTableAddr = 0x2400; break;
      case 2: nameTableAddr = 0x2800; break;
      case 3: nameTableAddr = 0x2C00; break;
    }

    attributeTableAddr = nameTableAddr + 0x03C0;
    tileX = vram_address & 0x1F;
    tileY = (vram_address >> 5) & 0x1F;
    tileScrollY = (vram_address >> 12) & 0x07;
    tileScrollX = scroll_x;
    tileAddress = nameTableAddr | (vram_address & 0x03FF);

    // Iterate through pixels in tile
    for (int pixelNum = 0; pixelNum < TILE_WIDTH; pixelNum++)
    {
      // Get pattern data
      tileIndex = read(tileAddress);
      patternPlane1 = read(patternTableAddr + (tileIndex << 4) + tileScrollY);
      patternPlane2 = read(patternTableAddr + (tileIndex << 4) + tileScrollY + 8);

      // Get attribute data
      //attributeAddress = attributeTableAddr | (vram_address & 0x0C00) | ((vram_address >> 4) & 0x38) | ((vram_address >> 2) & 0x07);
      attributeAddress = attributeTableAddr | ( ((((tileY * TILE_WIDTH) + tileScrollY) / 32) * (SCREEN_WIDTH / 32)) + (((tileX * TILE_WIDTH) + tileScrollX) / 32) );
      attributeValue = read(attributeAddress);
      groupIndex = (((tileX % 4) & 0x2) >> 1) + ((tileY % 4) & 0x2);
      paletteUpperBits = ((attributeValue >> (groupIndex<<1)) & 0x3) << 2;

      // Get color bits
      paletteIndex = paletteUpperBits;
      paletteIndex |= patternPlane1 & (0x80 >> tileScrollX) ? 0x1 : 0;
      paletteIndex |= patternPlane2 & (0x80 >> tileScrollX) ? 0x2 : 0;

      // If color bits from pattern tables are zero => transparent pixel
      if ((paletteIndex & 0x3) == 0)
      {
        _renderer->setTransparentPixel((tileNum << 3) + pixelNum, scanline);
      }
      else
      {
        // Render pixel to buffer
        _renderer->setPixel(PixelType::BackgroundTile, (tileNum << 3) + pixelNum, scanline, palette_table[read(ADDR_PALETTE_BG + paletteIndex)]);
      }

      // Update fine X for this tile
      tileScrollX++;

      // Wrap to beginning of tile
      if (tileScrollX >= TILE_WIDTH)
      {
        tileScrollX = 0;
        tileAddress++;
        tileX++;

        // Wrap name table horizontally
        if ((tileAddress & 0x1F) == 0)
        {
          tileAddress--;
          tileX--;
          tileAddress &= ~0x001F;
          tileAddress ^= 0x0400;
        }
      }
    }

    // Scroll X increment (http://wiki.nesdev.com/w/index.php/The_skinny_on_NES_scrolling)
    if ((vram_address & 0x001F) == 31)
    {
      vram_address &= ~0x001F;
      vram_address ^= 0x0400;
    }
    else
    {
      vram_address++;
    }
  }

  // Scroll Y increment (http://wiki.nesdev.com/w/index.php/The_skinny_on_NES_scrolling)
  if ((vram_address & 0x7000) != 0x7000)
  {
    vram_address += 0x1000;
  }
  else
  {
    vram_address &= 0x0FFF;
    unsigned short y = (vram_address & 0x03E0) >> 5;

    if (y == 29)
    {
      y = 0;
      vram_address ^= 0x0800;
    }
    else if (y == 31)
    {
      y = 0;
    }
    else
    {
      y++;
    }

    vram_address = (vram_address & ~0x03E0) | (y << 5);
  }
}

void ppu::updateScreen()
{
  clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &now);
  diff = utils::timespecDiff(&lastScreenUpdate, &now);

  // If last updateScreen() happens faster than 60 Hz then sleep for a while
  if (diff.tv_sec == 0 && diff.tv_nsec < SCREEN_UPDATE_TIME_IN_NS)
  {
    diff.tv_nsec = SCREEN_UPDATE_TIME_IN_NS - diff.tv_nsec;
    nanosleep(NULL, &diff);
  }

  // Update screen
  _renderer->update();

  // Clear screen
  palette_entry entry = palette_table[read(ADDR_PALETTE_BG)];
  _renderer->clear(entry);

  clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &lastScreenUpdate);
}

unsigned char ppu::read(unsigned short address)
{
  unsigned short tmp = vram_address;
  vram_address = address;
  unsigned char value = read();
  vram_address = tmp;
  return value;
}

unsigned char ppu::read()
{
  if (vram_address < 0x2000)
  {
    unsigned char value;
    _mapper->readChrRom(vram_address, value);
    return value;
  }

  unsigned short address = normalizeAddress(vram_address);
  return video_memory[address];
}

void ppu::write(unsigned short address, unsigned char value)
{
  unsigned short tmp = vram_address;
  vram_address = address;
  write(value);
  vram_address = tmp;
}

void ppu::write(unsigned char value)
{
  if (vram_address < 0x2000)
  {
    _mapper->writeChrRom(vram_address, value);
    return;
  }

  unsigned short address = normalizeAddress(vram_address);
  video_memory[address] = value;
}

unsigned short ppu::normalizeAddress(unsigned short address)
{
  address &= 0x3FFF;
  address = mirrorNameTables(address);

  // Nametables/attributetables (0x2000 to 0x2EFF) are mirrored from 0x3000 to ox3EFF
  if (address >= 0x2000 && address <= 0x3EFF)
  {
    address &= ~0x1000;
  }
  // Palettes (0x3F00 to 0x3F1F) are mirrored from 0x3F20 to 0x3FFF
  else if (address >= 0x3F00 && address <= 0x3FFF)
  {
    address &= ~0x00E0;
  }

  // Palette entries 0x3F10, 0x3F14, 0x3F18 and 0x3F1C are mirrored to
  // 0x3F00, 0x3F04, 0x3F08 and 0x3F0C, respectively
  if (address == 0x3F10 ||
    address == 0x3F14 ||
    address == 0x3F18 ||
    address == 0x3F1C)
  {
    address &= ~0xF0;
  }

  return address;
}

unsigned short ppu::mirrorNameTables(unsigned short address)
{
  // NT0=0x2000, NT1=0x2400, NT2=0x2800, NT3=0x2C00
  switch (_mapper->getMirroring())
  {
    // Remap 0x2000/0x2400 to NT0 and 0x2800/0x2C00 to NT1
    case Mirroring::Horizontal:
    {
      if (address >= 0x2400 && address <= 0x27FF)
      {
        return address - 0x0400;
      }
      else if (address >= 0x2800 && address <= 0x2BFF)
      {
        return address - 0x0400;
      }
      else if (address >= 0x2C00 && address <= 0x2FFF)
      {
        return address - 0x0800;
      }
      break;
    }

    // Remap 0x2000/0x2800 to NT0 and 0x2400/0x2C00 to NT1
    case Mirroring::Vertical:
    {
      if (address >= 0x2800 && address <= 0x2FFF)
      {
        return address - 0x0800;
      }
      break;
    }

    // Remap 0x2000/0x2400/0x2800/0x2C00 to NT0
    case Mirroring::SingleScreen:
    case Mirroring::SingleScreenLowerBank:
    {
      if (address >= 0x2000 && address <= 0x2FFF)
      {
        return (address & 0x03FF) | 0x2000;
      }
      break;
    }

    // Remap 0x2000/0x2400/0x2800/0x2C00 to NT1
    case Mirroring::SingleScreenUpperBank:
    {
      if (address >= 0x2000 && address <= 0x2FFF)
      {
        return (address & 0x03FF) | 0x2400;
      }
      break;
    }

    // Map 0x2000 to NT0, 0x2400 to NT1, 0x2800 to NT2 and 0x2C00 to NT3 (i.e. don't remap nothing)
    case Mirroring::FourScreen:
    {
      break;
    }
  }

  return address;
}

unsigned char ppu::readRegisterStatus()
{
  unsigned char value = ppu_status;
  vram_access_flipflop = false;
  ppu_status &= ~PPU_STATUS_VBLANK_STARTED;
  return value;
}

unsigned char ppu::readRegisterOAMData()
{
  unsigned char value = sprite_memory[sprite_address];
  return value;
}

unsigned char ppu::readRegisterVRAMData()
{
  unsigned char value = 0;

  // Return buffered latch value if not palette address
  if (vram_address >= 0x0000 && vram_address <= 0x3EFF)
  {
    value = vramDataLatch;
    vramDataLatch = read();
  }
  // Reading from palette addresses gives direct access,
  // but also alters vramDataLatch using a manipulated vram_address
  // (see: blargg_ppu_tests_2005.09.15b, vram_access.asm)
  else
  {
    vramDataLatch = read(normalizeAddress(vram_address & (~0x1000)));
    value = read();
  }

  vram_address += (ppu_control & PPU_CONTROL_VRAM_ADDR_INCR ? 32 : 1);
  return value;
}

void ppu::writeRegisterControl(unsigned char value)
{
  //BOOST_ASSERT_MSG(!(ppu_control & PPU_CONTROL_SPRITE_SIZE), "8x16 sprites not supported yet\n");

  vram_latch = (vram_latch & ~(3 << 10)) | ((value & 3) << 10);
  ppu_control = value;
}

void ppu::writeRegisterMask(unsigned char value)
{
  ppu_mask = value;
}

void ppu::writeRegisterOAMAddr(unsigned char value)
{
  sprite_address = value;
}

void ppu::writeRegisterOAMData(unsigned char value)
{
  sprite_memory[sprite_address] = value;
  sprite_address++;
}

void ppu::writeRegisterScroll(unsigned char value)
{
  if (!vram_access_flipflop)
  {
    ppu_scroll_origin = value << 8;
    scroll_x = value & 0x07;
    vram_latch = (vram_latch & ~0x1F) | ((value >> 3) & 0x1F);
  }
  else
  {
    ppu_scroll_origin |= value;
    scroll_y = value;
    vram_latch = (vram_latch & ~(0x1F << 5)) | (((value >> 3) & 0x1F) << 5);
    vram_latch = (vram_latch & ~(0x07 << 12)) | ((value & 0x07) << 12);
  }

  vram_access_flipflop = !vram_access_flipflop;
}

void ppu::writeRegisterVRAMAddr(unsigned char value)
{
  if (!vram_access_flipflop)
  {
    vram_address = value << 8; // TODO: remove?
    vram_latch = (vram_latch & 0xFF) | ((value & 0x3F) << 8);
  }
  else
  {
    //vram_address |= value;  // TODO: remove?
    vram_latch = (vram_latch & ~0xFF) | value;
    vram_address = vram_latch;
  }

  vram_access_flipflop = !vram_access_flipflop;
}

void ppu::writeRegisterVRAMData(unsigned char value)
{
  write(value);
  vram_address += (ppu_control & PPU_CONTROL_VRAM_ADDR_INCR ? 32 : 1);
}

void ppu::writeDMA(unsigned char value)
{
  unsigned short baseAddress = value * SPRITE_RAM_SIZE;
  int i = 0;

  while (i < SPRITE_RAM_SIZE)
  {
    sprite_memory[(sprite_address + i) % SPRITE_RAM_SIZE] = _cpu->read(baseAddress + i);
    i++;
  }

  ppuCycles += DMA_CYCLES * PPU_PER_CPU_CYCLE;
}
