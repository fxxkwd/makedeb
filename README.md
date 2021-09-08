# makedeb
a lightweight tool use for create a deb file for windows.

## Used library
* microtar: <https://github.com/rxi/microtar>
* bzip2: <http://www.bzip.org/>
* zlib: <http://zlib.net/>

## How to build
1. use Visual Studio 2015-2019 create a C++ console application.
2. download 3 libraries
3. compile bzip2, use command:
```sh
nmake -f makefile.msc
```
4. compile zlib, use command:
```sh
nmake -f win32/makefile.msc
```
5. add microtar.c and microtar.h to your project
6. add libbz2.lib and zlib.lib to your project
7. add TDebFile.cpp TDebFile.h makeDeb.cpp to your project
8. compile your project

## How to use
```sh
makeDeb.exe <source dir> [output deb file]
```

## Thank you
this is my first open source project, thank you!
