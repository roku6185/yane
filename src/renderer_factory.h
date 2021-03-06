#ifndef _RENDERER_FACTORY_H_
#define _RENDERER_FACTORY_H_

#include "renderer.h"
#include <map>
#include <boost/shared_ptr.hpp>

enum RendererType { SDL, Unknown };

class RendererFactory
{
public:
  static boost::shared_ptr<Renderer> create(const std::string &name);

private:
  RendererFactory();
  static enum RendererType lookupType(const std::string &name);
  static const std::map<std::string, RendererType> lookupTable;
};

#endif
