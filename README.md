#What Is Yane?

Yane is an acronym for Yet Another Nes Emulator. Yane is a development version of a NES emulator. It supports basic functionality, such as:
* CPU: Official and unofficial opcodes
* Horizontal and vertical scrolling
* Horizontal and vertical flipping of sprites
* 8x8 sprites
* 8x16 sprites (somewhat buggy)
* Basic sprite overflow
* Basic sprite 0 hit
* NROM, UNROM, CNROM, GNROM, AOROM, MMC1, MMC2, MMC3 support
* Controller input

Yane is only capable of playing ROMs of the following types:
* NROM (mapper #0, cartridges without any extra hardware)
* MMC1 (mapper #1)
* UNROM (mapper #2)
* CNROM (mapper #3)
* MMC3 (mapper #4)
* AOROM (mapper #7)
* MMC2 (mapper #9)
* GNROM (mapper #66)

Some games that are playable with Yane:
* Super Mario Bros 1
* Super Mario Bros 3
* Contra
* Castlevania
* Castlevania 2
* Donkey Kong Jr.
* Pacman
* Ice Hockey
* Excitebike
* Arkanoid
* Mega Man
* Mega Man 2
* Mega Man 3
* Mega Man 4
* Mega Man 5
* Metal Gear
* Alpha Mission
* Dragon Power
* Tennis
* Teenage Mutant Ninja Turtles
* Lemmings
* Darkwing Duck
* Donald Duck
* Double Dragon
* Double Dragon 2
* Double Dragon 3
* Crossfire
* Kid Klown
* Duck Tales
* Metal Mech
* Paperboy

Yane was written using C++11 and with the following libraries:
* Boost
* SDL

#Purpose

I created Yane because of several reasons:
* Nostalgia (I grew up with playing the NES console)
* The challenge of creating an emulator
* Update my C++ knowledge
* Learning and appying best practices of "Effective C++" etc.

Yane is not built for speed, cycle accuracy, rom compability, etc. One goal of Yane was to be able to play Super Mario Bros 1. Yane should only be used a reference for other projects. It should probably not be used for serious NES gaming.

Contact: robin (at) alike.se

#Instructions
##Install Libraries

Debian/Ubuntu:

$ sudo apt-get install libsdl1.2debian libsdl-gfx1.2-4 libsdl1.2-dev libsdl-gfx1.2-dev

Red Hat/Fedora:

$ sudo yum install SDL SDL-devel SDL_gfx SDL_gfx-devel

¤¤Compile
$ cmake .
$ make

##Run Yane

Usage:

Usage: yane [options] --rom <ROM_FILE>:
  --version                    Show version
  --help                       Show this help message
  --rom arg                    What iNES rom file to use
  --nes-test                   Treat rom as nestest (adjusted reset vector)
  --blargh-test                Treat rom as blargh test (polls 0x6000 for 
                               results)
  --log                        Enable logging of instructions (nestest format)
  --rom-info                   Display rom headers
  -f [ --fullscreen ]          Use fullscreen mode
  -r [ --renderer ] arg (=sdl) Use another render engine (default: SDL)

#Credits
* The NESDev community
* Blargg for all test ROMs
* Other projects (mappers specifically):
** fakenes (http://fakenes.sourceforge.net)
** halfnes (http://code.google.com/p/halfnes)
** My Nes (http://sourceforge.net/projects/mynes)
