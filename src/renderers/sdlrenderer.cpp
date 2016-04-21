#include "renderers/sdlrenderer.h"
#include "config.h"
#include "yane_exception.h"

void SDLRenderer::init()
{
  // Initialize SDL
  if (SDL_Init(SDL_INIT_VIDEO) < 0)
  {
    throw SDLInitException(SDL_GetError());
  }

  int width = SCREEN_WIDTH;
  int height = SCREEN_HEIGHT;
  int bpp = SCREEN_BPP;
  int flags = SDL_HWSURFACE;

  SDL_Rect **modes = SDL_ListModes(NULL, flags);

  if (modes == (SDL_Rect**)0)
  {
    printf("No modes available!\n");
    exit(1);
  }

  if (modes != (SDL_Rect**)-1)
  {
    const SDL_VideoInfo *vInfo = SDL_GetVideoInfo();
    width = vInfo->current_w;
    height = vInfo->current_h;
    bpp = vInfo->vfmt->BitsPerPixel;
  }

  if (Config::instance().isFullscreen)
  {
    flags |= SDL_FULLSCREEN;
  }

  screen = SDL_SetVideoMode(width, height, bpp, flags);
  printf("Using video mode: %dx%dx%d\n", width, height, bpp);

  if (!screen)
  {
    throw SDLVideoException(width, height, bpp);
  }

  bg = SDL_DisplayFormatAlpha(screen);
  sprFg = SDL_DisplayFormatAlpha(screen);
  sprBg = SDL_DisplayFormatAlpha(screen);

  // Set window title
  std::string title = "Yane " + Config::instance().getVersion();
  SDL_WM_SetCaption(title.c_str(), 0);
}

void SDLRenderer::cleanup()
{
  // Cleanup SDL
  SDL_FreeSurface(screen);
  SDL_FreeSurface(bg);
  SDL_FreeSurface(sprFg);
  SDL_FreeSurface(sprBg);

  screen = NULL;
  bg = NULL;
  sprFg = NULL;
  sprBg = NULL;
}

void SDLRenderer::update()
{
  // Update screen with background and sprite surfaces
  SDL_BlitSurface(sprBg, NULL, screen, NULL);
  SDL_BlitSurface(bg, NULL, screen, NULL);
  SDL_BlitSurface(sprFg, NULL, screen, NULL);
  SDL_Flip(screen);
}

void SDLRenderer::clear(const palette_entry &entry)
{
  // Clear screen
  int color = SDL_MapRGBA(screen->format, 0, 0, 0, 0);
  SDL_Rect clearRect = { 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT };
  SDL_FillRect(screen, &clearRect, color);
  SDL_FillRect(sprBg, &clearRect, color);
  SDL_FillRect(sprFg, &clearRect, color);
  SDL_FillRect(bg, &clearRect, color);

  // Fill screen with background color
  SDL_FillRect(screen, &clearRect, SDL_MapRGB(screen->format, entry.r, entry.g, entry.b));
}

void SDLRenderer::putPixel(const enum PixelType &type, int x, int y, const palette_entry &color, unsigned char alpha)
{
  if (alpha == 0)
    return;

  SDL_Surface *surface = getSurface(type);
  int bpp = surface->format->BytesPerPixel;
  char *p = (char*)surface->pixels + y * surface->pitch + x * bpp;
  int pixel = SDL_MapRGBA(surface->format, color.r, color.g, color.b, alpha);

  switch (bpp)
  {
  case 1:
    *p = pixel;
    break;

  case 2:
    *(short*)p = (short)pixel;
    break;

  case 3:
    if (SDL_BYTEORDER == SDL_BIG_ENDIAN)
    {
      p[0] = (pixel >> 16) & 0xff;
      p[1] = (pixel >> 8) & 0xff;
      p[2] = pixel & 0xff;
    }
    else
    {
      p[0] = pixel & 0xff;
      p[1] = (pixel >> 8) & 0xff;
      p[2] = (pixel >> 16) & 0xff;
    }
    break;


  case 4:
    *(int*)p = pixel;
    break;

  default:
    break;
  }
}

int SDLRenderer::getPixel(SDL_Surface *surface, int x, int y)
{
  int bpp = surface->format->BytesPerPixel;
  char *p = (char*)surface->pixels + y * surface->pitch + x * bpp;

  switch (bpp)
  {
  case 1:
    return *p;
    break;

  case 2:
    return *(short*)p;
    break;

  case 3:
    if (SDL_BYTEORDER == SDL_BIG_ENDIAN)
    {
      return p[0] << 16 | p[1] << 8 | p[2];
    }
    else
    {
      return p[0] | p[1] << 8 | p[2] << 16;
    }
    break;

  case 4:
    return *(int*)p;
    break;

  default:
    return 0;
  }
}

void SDLRenderer::setTransparentPixel(int x, int y)
{
  palette_entry color = { 1, 2, 3 };
  putPixel(PixelType::BackgroundTile, x, y, color, 0);
}

bool SDLRenderer::isTransparentPixel(int x, int y)
{
  unsigned char r, g, b, a;
  int pixel = getPixel(bg, x, y);
  SDL_GetRGBA(pixel, bg->format, &r, &g, &b, &a);
  return (r == 1 && g == 2 && b == 3);
}

SDL_Surface *SDLRenderer::getSurface(const enum PixelType &type)
{
  switch (type)
  {
  case PixelType::BackgroundTile:
    return bg;

  case PixelType::ForegroundSprite:
    return sprFg;

  case PixelType::BackgroundSprite:
    return sprBg;
  }

  return bg;
}
