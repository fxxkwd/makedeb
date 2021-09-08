// makeDeb.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>
#include <sstream>
#include <string>
#include <cmath>
#include <cstdio>

#include <Windows.h>
#include <io.h>
#include <stdlib.h>
#include <fcntl.h>
#include "TDebFile.h"

using namespace std;

string getLastErrorMsg()
{
    char szBuf[128];
    LPVOID* lpMsgBuf;
    DWORD errCode = GetLastError();
    FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        errCode,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR)&lpMsgBuf,
        0, NULL);
    sprintf(szBuf, "%d/%s", errCode, (char*)lpMsgBuf);
    string ret(szBuf);
    LocalFree(lpMsgBuf);
    return ret;
}

string formatFileSize(long size)
{
    string formattedStr;
    stringstream sstm;

    if (size == 0)
        formattedStr = "0";

    else if (size > 0 && size < 1024) {
        sstm << size << " bytes";
        formattedStr = sstm.str();
    }

    else if (size >= 1024 && size < pow(1024, 2)) {
        sstm << (size / 1024.) << " KB";
        formattedStr = sstm.str();
    }

    else if (size >= pow(1024, 2) && size < pow(1024, 3)) {
        sstm << (size / pow(1024, 2)) << " MB";
        formattedStr = sstm.str();
    }

    else if (size >= pow(1024, 3) && size < pow(1024, 4)) {
        sstm << (size / pow(1024, 3)) << " GB";
        formattedStr = sstm.str();
    }

    else if (size >= pow(1024, 4)) {
        sstm << (size / pow(1024, 4)) << " TB";
        formattedStr = sstm.str();
    }

    return formattedStr;
}

string getExePath()
{
    char exeFullPath[MAX_PATH]; // Full path
    GetModuleFileName(NULL, exeFullPath, MAX_PATH);
    string strPath(exeFullPath);
    size_t pos = strPath.find_last_of('\\', strPath.length());
    return strPath.substr(0, pos);
}

string getDirFromPath(const string& fullpath)
{
    string directory;
    const size_t last_slash_idx = fullpath.rfind('\\');
    if (std::string::npos != last_slash_idx)
    {
        directory = fullpath.substr(0, last_slash_idx);
    }
    else {
        directory = ".";
    }
    return directory;
}

#define COLOR_GREEN 1
#define COLOR_RED   2
#define COLOR_WHITE 3
void setOutputTextColor(int color)
{
    switch (color)
    {
    case COLOR_GREEN:
        SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_INTENSITY | FOREGROUND_GREEN);
        break;

    case COLOR_RED:
        SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_INTENSITY | FOREGROUND_RED);
        break;

    default:
        SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
        break;
    }
}
#define print_white(format, ...) {setOutputTextColor(COLOR_WHITE); printf (format, ##__VA_ARGS__);}
#define print_green(format, ...) {setOutputTextColor(COLOR_GREEN); printf (format, ##__VA_ARGS__);}
#define print_red(format, ...) {setOutputTextColor(COLOR_RED); printf (format, ##__VA_ARGS__);}
#define print_ln() printf("\n")
#define check_code(ret, exit_code) if (ret > 0) print_green("OK!\n") else { print_red("error: %d\n", ret) return exit_code; }

void addPathSepa(string& sPath)
{
    if (sPath[sPath.length() - 1] != '\\')
        sPath += "\\";
}

string getPathParentPath(const string& sPath)
{
    string tmp(sPath);
    if (tmp[tmp.length() - 1] == '\\')
        tmp.erase(tmp.length() - 1);
    string::size_type pos = tmp.rfind('\\');
    if (pos != string::npos)
        tmp = tmp.substr(0, pos);
    return tmp;
}

int main(int argc, char* argv[])
{
    if (argc < 2)
    {
        printf("usage:\n\tmakedeb <dir> [output deb dir]\n");
        return 1;
    }
    string tempPath(getExePath());
    tempPath += "\\temp\\";
    if (_access(tempPath.c_str(), 0) == -1) //exepath\\temp not exits, use %temp%
    {
        char _tmp[MAX_PATH];
        GetTempPath(MAX_PATH, _tmp);
        tempPath = _tmp;
        addPathSepa(tempPath);
    }

    string srcPath(argv[1]);
    addPathSepa(srcPath);

    string destPath;
    if (argc >= 3)
        destPath = argv[2];    //use param 2
    else
        destPath = getPathParentPath(srcPath);
    addPathSepa(destPath);

    print_white("1.reading info...");
    map<string, string> map_info;
    TDebFile deb_creater;
    string control_file(srcPath);
    control_file += "DEBIAN\\control";
    int ret = deb_creater.parseControlFile(control_file.c_str(), map_info);
    check_code(ret, 2);

    for(auto it = map_info.cbegin(); it != map_info.cend(); it++)
        print_white("\t%s: %s\n", it->first.c_str(), it->second.c_str());

    print_white("2.creating control.tar...");
    string control_tar_file(tempPath);
    control_tar_file += "control.tar";
    string control_path(srcPath);
    control_path += "DEBIAN\\";
    ret = deb_creater.createTarFile(control_tar_file.c_str(), control_path.c_str());
    check_code(ret, 3);

    print_white("3.creating control.tar.gz...");
    ret = deb_creater.createGzipFile(control_tar_file.c_str());
    check_code(ret, 4);

    print_white("4.creating data.tar...");
    string data_tar_file(tempPath);
    data_tar_file += "data.tar";
    ret = deb_creater.createTarFile(data_tar_file.c_str(), srcPath.c_str());
    check_code(ret, 5);

    print_white("5.creating data.tar.bz2...");
    ret = deb_creater.createBzip2File(data_tar_file.c_str());
    check_code(ret, 6);

    string deb_file(destPath);
    deb_file += map_info["Name"];
    deb_file += "_";
    deb_file += map_info["Version"];
    deb_file += ".deb";
    //print_white("\tdest deb: %s\n", deb_file.c_str());
    print_white("7.create deb %s...", deb_file.c_str());
    string control_gz(control_tar_file);
    control_gz += ".gz";
    string data_bz2(data_tar_file);
    data_bz2 += ".bz2";
    ret = deb_creater.createDebFile(deb_file.c_str(), control_gz.c_str(), data_bz2.c_str());
    check_code(ret, 7);

    DeleteFile(control_tar_file.c_str());
    DeleteFile(control_gz.c_str());
    DeleteFile(data_tar_file.c_str());
    DeleteFile(data_bz2.c_str());

    print_green("all done!");
    print_white("\n");
    return 0;
}

