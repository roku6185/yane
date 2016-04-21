#ifndef _RENDERER_H_
#define _RENDERER_H_

#include "ppu.h"

using namespace std;

enum PixelType { BackgroundTile, ForegroundSprite, BackgroundSprite };

class Renderer
{
public:
  Renderer() {};
  virtual void init() = 0;
  virtual void cleanup() = 0;
  virtual void update() = 0;
  virtual void clear(const palette_entry &entry) = 0;
  virtual void setTransparentPixel(int x, int y) = 0;
  virtual bool isTransparentPixel(int x, int y) = 0;

  void setPixel(const enum PixelType &type, int x, int y, const palette_entry &color)
  {
    if (x < 8 || x > SCREEN_WIDTH - 8)
    {
      return;
    }

    if (type != PixelType::BackgroundTile && (y < 16 || y > SCREEN_HEIGHT - 11))
    {
      return;
    }

    putPixel(type, x, y, color, 255);
  }

protected:
  virtual void putPixel(const enum PixelType &type, int x, int y, const palette_entry &color, unsigned char alpha) = 0;
};

#endif
