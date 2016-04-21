#include "renderer_factory.h"
#include "yane_exception.h"
#include "renderers/sdlrenderer.h"

Renderer* RendererFactory::create(const std::string &name)
{
  enum RendererType type = lookupType(name);

  switch (type)
  {
    case RendererType::SDL:
      return  new SDLRenderer();
      break;

    default:
      throw RendererNotSupportedException(name);
      break;
  }
}

enum RendererType RendererFactory::lookupType(const std::string &name)
{
  std::map<std::string, RendererType>::const_iterator it = RendererFactory::lookupTable.find(name);

  if (it != RendererFactory::lookupTable.end())
  {
    return it->second;
  }

  return RendererType::Unknown;
}

const std::map<std::string, RendererType> RendererFactory::lookupTable =
{
  {"sdl", RendererType::SDL},
  {"",    RendererType::Unknown},
};
