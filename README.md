# makedeb
A lightweight tool use for create a deb (Debian) file on windows.

## How to build
##### 1. download 3 libraries
* microtar: <https://github.com/rxi/microtar> create .tar file
* bzip2: <http://www.bzip.org/> create .bz2 file
* zlib: <http://zlib.net/> create .bz file

##### 2. compile bzip2, use command:
```sh
cd <bzip2_source_dir>
nmake -f makefile.msc
```
##### 3. compile zlib, use command:
```sh
cd <zlib_source_dir>
nmake -f win32/makefile.msc
```
##### 4. create project
use Visual Studio 2015-2019 create a C++ Windows console application, and remove all file.
##### 5. add files to your project
        TDebFile.cpp
        TDebFile.h
        makeDeb.cpp
        microtar.c
        microtar.h
        libbz2.lib
        zlib.lib
##### 6. compile your project

## How to use
```sh
makeDeb.exe <source dir> [output deb file]
```

## Thank you
this is my first open source project, thank you!
