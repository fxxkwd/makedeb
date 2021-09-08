#pragma once

#include <cstdio>
#include <ctime>
#include <map>
#include <string>

using namespace std;

class TDebFile
{
private:
    FILE* h_deb;
    time_t timestamp;

    void put_magic();
    bool put_head(const char* name, size_t size);
    bool put_member(const char* name, const void* data, size_t size);
    bool put_file(const char* fname);

public:
    TDebFile();
    ~TDebFile();
    int createDebFile(const char* deb_file, const char* control_file, const char* data_file);
    int createTarFile(const char* tar_file, const char* dir);
    int createBzip2File(const char* src_file);
    int createGzipFile(const char* src_file);
    int parseControlFile(const char* control_file, map<string, string>& out_map);
};

