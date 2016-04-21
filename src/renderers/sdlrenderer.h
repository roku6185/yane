#ifndef _SDLRENDERER_H_
#define _SDLRENDERER_H_

#include <SDL/SDL.h>
#include <SDL/SDL_gfxPrimitives.h>

#include "renderer.h"

// Reserve a special RGB value, which is not in the NES palette, to represent a transparent pixel
#define TRANSPARENT_HACK  0x030201

using namespace std;

class SDLRenderer : public Renderer
{
public:
  SDLRenderer() {};
  void init();
  void cleanup();
  void update();
  void clear(const palette_entry &entry);
  void putPixel(const enum PixelType &type, int x, int y, const palette_entry &color, unsigned char alpha);
  int getPixel(SDL_Surface *surface, int x, int y);
  void setTransparentPixel(int x, int y);
  bool isTransparentPixel(int x, int y);

private:
  SDL_Surface *getSurface(const enum PixelType &type);
  SDL_Surface *screen, *bg, *sprBg, *sprFg;
};

#endif
