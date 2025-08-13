clear
clear

x86_64-w64-mingw32-g++ -std=c++11 -msse4.2 -DDEBUG_BUILD=1 \
  -Wno-deprecated-declarations -Wno-write-strings \
  -o ../../bin/main.exe ./platform_layer.cpp \
  -I../../windows_sdl_64bit/include \
  -L../../windows_sdl_64bit/lib \
  -lopengl32 -lSDL2main -lSDL2 -lole32 -lcomdlg32 -luuid -loleaut32 