# bsnes-classic
SNES emulator with Qt GUI and debugger
Building on Windows

Get mingw-w64 (http://mingw-w64.yaxm.org/doku.php/download)
Install Qt 4.8.6 (http://download.qt.io/archive/qt/) and make sure its bin directory is in your path
Run mingw32-make
Building with the original MinGW used to be the preferred way to do it, but made building "out of the box" annoying for various reasons (including requiring outdated DirectX headers/libs and problems with some native Windows code) and is no longer supported.

Building on OS X

Install a C++ toolchain (Xcode is probably the easiest route)
Install Qt 4.8 (get Brew and run brew install qt)
Run makefrom the bsnes directory.
Building on Linux / other *nix

As there is no configure step, make sure necessary Qt4/X11 packages are installed. On a Debian/Ubuntu system, it would require a command like:

apt-get install libqt4-dev libqt4-dev-bin libxv-dev libsdl1.2-dev libao-dev libopenal-dev g++
Afterwards, run make and if everything works out correctly you will find the output binary in the out/ directory.

The snesfilter, snesreader, and supergameboy plugins can all be built by running make (or mingw32-make) after you've configured your environment to build bsnes itself. After building, just copy the .dll, .so, or .dylib files into the same directory as bsnes itself.
bsnes v073 and its derivatives are licensed under the GPL v2;
