# makedeb
A lightweight tool use for create a deb (Debian) file on windows.

## Used library
* microtar: <https://github.com/rxi/microtar> create .tar file
* bzip2: <http://www.bzip.org/> create .bz2 file
* zlib: <http://zlib.net/> create .bz file

## How to build
1. use Visual Studio 2015-2019 create a C++ console application, remove all file.
2. download 3 libraries
3. compile bzip2, use command:
```sh
cd <bzip2_source_dir>
nmake -f makefile.msc
```
4. compile zlib, use command:
```sh
cd <zlib_source_dir>
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
