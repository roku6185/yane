#ifndef _RENDERER_FACTORY_H_
#define _RENDERER_FACTORY_H_

#include "renderer.h"
#include <map>

enum RendererType { SDL, Unknown };

class RendererFactory
{
public:
  static Renderer* create(const std::string &name);

private:
  RendererFactory();
  static enum RendererType lookupType(const std::string &name);
  static const std::map<std::string, RendererType> lookupTable;
};

#endif
