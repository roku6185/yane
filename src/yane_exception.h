#ifndef _YANE_EXCEPTION_H_
#define _YANE_EXCEPTION_H_

#include <exception>
#include <boost/lexical_cast.hpp>
#include <SDL/SDL.h>

using namespace std;

class YaneException : public exception
{
public:
  YaneException(const string message) : msg(message) {}
  ~YaneException() throw() {}
  const char* what() const throw() { return msg.c_str(); }

protected:
  string msg;
};

class PrgBankNotSupportedException : public YaneException
{
public:
  PrgBankNotSupportedException(int bank) :
    YaneException("PRG bank not supported: " + boost::lexical_cast<string>(bank)) {}
};

class ChrBankNotSupportedException : public YaneException
{
public:
  ChrBankNotSupportedException(int bank) :
    YaneException("CHR bank not supported: " + boost::lexical_cast<string>(bank)) {}
};

class MapperNotSupportedException : public YaneException
{
public:
  MapperNotSupportedException(int mapper) :
    YaneException("Mapper not supported: " + boost::lexical_cast<string>(mapper)) {}
};

class InvalidOpcodeException : public YaneException
{
public:
  InvalidOpcodeException(unsigned short opcode) :
    YaneException("Invalid opcode: " + boost::lexical_cast<string>(opcode)) {}
};

class InvalidRomException : public YaneException
{
public:
  InvalidRomException(string filename) :
    YaneException("Invalid iNES ROM: " + filename) {}
};

class SDLInitException : public YaneException
{
public:
  SDLInitException(string sdlError) :
    YaneException("Unable to initialize SDL: " + sdlError) {}
};

class SDLVideoException : public YaneException
{
public:
  SDLVideoException(int width, int height, int bpp) :
    YaneException("Unable [" + boost::lexical_cast<string>(SDL_GetError()) + "] to set resolution: " + boost::lexical_cast<string>(width) + "x" + boost::lexical_cast<string>(height) + "x" + boost::lexical_cast<string>(bpp)) {}
};

class RendererNotSupportedException : public YaneException
{
public:
  RendererNotSupportedException(string name) :
    YaneException("Renderer not supported: " + name) {}
};

#endif
