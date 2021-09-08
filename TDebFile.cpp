#include "TDebFile.h"
#include <cstring>
#include <vector>
#include <iostream>
#include <windows.h>
#include <fstream>
#include <codecvt>

#include "microtar.h"
#include "bzip2-master/bzlib.h"
#include "zlib-1.2.11/zlib.h"

#pragma comment(lib, "bzip2-master/libbz2.lib")
#pragma comment(lib, "zlib-1.2.11/zlib.lib")

#define DPKG_AR_MAGIC   "!<arch>\n"
#define DPKG_AR_FMAG    "`\n"
#define ARCHIVEVERSION  "2.0"
#define DEBMAGIC		"debian-binary"
#define FILE_BUF_SIZE   1024

struct dpkg_ar_hdr {
    char ar_name[16];	   /* Member file name, sometimes / terminated. */
    char ar_date[12];	   /* File date, decimal seconds since Epoch.  */
    char ar_uid[6], ar_gid[6]; /* User and group IDs, in ASCII decimal.  */
    char ar_mode[8];	   /* File mode, in ASCII octal.  */
    char ar_size[10];	   /* File size, in ASCII decimal.  */
    char ar_fmag[2];
};

TDebFile::TDebFile()
{
    h_deb = NULL;
    timestamp = time(NULL);
    //setlocale(LC_ALL, "LC_CTYPE=.utf8");
}

TDebFile::~TDebFile()
{
    if (h_deb != NULL) fclose(h_deb);
}

void TDebFile::put_magic()
{
    fwrite(DPKG_AR_MAGIC, strlen(DPKG_AR_MAGIC), 1, h_deb);
}

bool TDebFile::put_head(const char* name, size_t size)
{
    char header[sizeof(struct dpkg_ar_hdr) + 1];
    int n = snprintf(header, sizeof(struct dpkg_ar_hdr) + 1,
        "%-16s%-12jd%-6lu%-6lu%-8lo%-10jd`\n",
        name, timestamp, 0, 0, 0100644, size);
    return fwrite(header, n, 1, h_deb) > 0;
}

bool TDebFile::put_member(const char* name, const void* data, size_t size)
{
    if (!put_head(name, size)) return false;
    if (fwrite(data, size, 1, h_deb) <= 0) return false;
    if (size & 1)
        if (fwrite("\n", 1, 1, h_deb) <= 0) return false;
    return true;
}

const char* getFilenameFromPath(const char* fullPath)
{
    //int x = strlen(fullPath);
    return strrchr(fullPath, '\\') + 1;
}

bool TDebFile::put_file(const char *fname)
{
    char short_name[16 + 1];
    sprintf(short_name, "%s", getFilenameFromPath(fname));
    //printf("write %s...\n", short_name);
    FILE* gzfd = fopen(fname, "rb");
    if (gzfd == NULL)
    {
        //printf("failed to read archive '%s'", fname);
        return false;
    }
    fseek(gzfd, 0L, SEEK_END);
    long fsize = ftell(gzfd);
    rewind(gzfd);

    if (!put_head(short_name, fsize)) return false;

    char buf[FILE_BUF_SIZE];
    while (!feof(gzfd))
    {
        size_t readed = fread(buf, 1, FILE_BUF_SIZE, gzfd);
        if (readed > 0)
            fwrite(buf, 1, readed, h_deb);
    }

    if (fsize & 1)
        if (fwrite("\n", 1, 1, h_deb) <= 0) return false;
    fclose(gzfd);
    return true;
}

int TDebFile::createDebFile(const char* deb_file, const char * control_file, const char* data_file)
{
    h_deb = fopen(deb_file, "wb");
    if (h_deb == NULL) return -1;

    put_magic();
    const char deb_magic[] = ARCHIVEVERSION "\n";
    if (!put_member(DEBMAGIC, deb_magic, strlen(deb_magic))) return -2;

    if (!put_file(control_file)) return -3;

    if (!put_file(data_file)) return -4;

    fclose(h_deb);
    h_deb = NULL;
    return 1;
}

//=========================================================================================
// create tar file
//=========================================================================================
void listDir(const char* path, std::vector<std::string>* files)
{
    HANDLE hFind;
    WIN32_FIND_DATA findData;

    string dirWithStar(path);
    dirWithStar += "\\*.*";

    hFind = FindFirstFile(dirWithStar.c_str(), &findData);    // 查找目录中的第一个文件
    if (hFind == INVALID_HANDLE_VALUE) return;

    do
    {
        if (strcmp(findData.cFileName, ".") == 0 || strcmp(findData.cFileName, "..") == 0)
            continue;

        string fullname(path);
        fullname += "\\";
        fullname += findData.cFileName;
        if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
            //printf("dir: %s\n", fullname.c_str());
            listDir(fullname.c_str(), files);
        } else {
            //printf("file: %s\n", fullname.c_str());
            files->push_back(fullname);
        }

    } while (FindNextFile(hFind, &findData));
    FindClose(hFind);
}

bool put_file_todeb(mtar_t *tar, const string& fname, size_t root_dir_len)
{
    const string sub_fname = fname.substr(root_dir_len);
    if (sub_fname.compare(0, 7, "DEBIAN\\") == 0) return true;
    //cout << sub_fname << endl;

    FILE* fpSrc = fopen(fname.c_str(), "rb");
    if (fpSrc == NULL) return false;

    fseek(fpSrc, 0L, SEEK_END);
    long fsize = ftell(fpSrc);
    rewind(fpSrc);

    mtar_write_file_header(tar, sub_fname.c_str(), fsize);

    char buf[512];
    while (!feof(fpSrc))
    {
        size_t readed = fread(buf, 1, 512, fpSrc);
        if (readed > 0)
            mtar_write_data(tar, buf, readed);
    }

    fclose(fpSrc);
    return true;
}

int TDebFile::createTarFile(const char* tar_file, const char* dir)
{
    DeleteFile(tar_file);
    mtar_t tar;
    if (mtar_open(&tar, tar_file, "w") != MTAR_ESUCCESS) return -1;

    vector<string> files;
    listDir(dir, &files);
    size_t dir_len = strlen(dir)+1;
    for (auto it = files.begin(); it != files.end(); it++)
    {
        put_file_todeb(&tar, *it, dir_len);
    }

    mtar_finalize(&tar);
    mtar_close(&tar);
    return 1;
}

//=========================================================================================
// create bz2 file
//=========================================================================================
int TDebFile::createBzip2File(const char* src_file)
{
    FILE* fpBz2;
    BZFILE* pbzFile;
    int     bzerror;

    FILE* fpSrc = fopen(src_file, "rb");
    if (fpSrc == NULL) return -1;

    char bz2_filename[MAX_PATH];
    strcpy(bz2_filename, src_file);
    strcat(bz2_filename, ".bz2");
    fpBz2 = fopen(bz2_filename, "wb");
    if (fpBz2 == NULL) return -1;

    pbzFile = BZ2_bzWriteOpen(&bzerror, fpBz2, 9, 0, 0);
    if (bzerror != BZ_OK) {
        BZ2_bzWriteClose(&bzerror, pbzFile, 0, NULL, NULL);
        return -2;
    }

    char    buf[FILE_BUF_SIZE];
    while (!feof(fpSrc))
    {
        size_t readed = fread(buf, 1, FILE_BUF_SIZE, fpSrc);
        if (readed > 0)
        {
            BZ2_bzWrite(&bzerror, pbzFile, (void *)buf, readed);
            if (bzerror == BZ_IO_ERROR) {
                BZ2_bzWriteClose(&bzerror, pbzFile, 0, NULL, NULL);
                return -3;
            }
        }
    }

    BZ2_bzWriteClose(&bzerror, pbzFile, 0, NULL, NULL);
    if (bzerror == BZ_IO_ERROR) {
        return -4;
    }
    fclose(fpSrc);
    fclose(fpBz2);
    return 1;
}


//=========================================================================================
// create gz file
//=========================================================================================
int TDebFile::createGzipFile(const char* src_file)
{
    FILE* fpSrc = fopen(src_file, "rb");
    if (fpSrc == NULL) return -1;

    char bz2_filename[MAX_PATH];
    strcpy(bz2_filename, src_file);
    strcat(bz2_filename, ".gz");
    gzFile fp_gz = gzopen(bz2_filename, "wb");
    if (fp_gz == NULL) return -2;

    char    buf[FILE_BUF_SIZE];
    while (!feof(fpSrc))
    {
        size_t readed = fread(buf, 1, FILE_BUF_SIZE, fpSrc);
        if (readed > 0)
            gzwrite(fp_gz, buf, readed);
    }

    fclose(fpSrc);
    gzclose(fp_gz);
    return 1;
}

//=========================================================================================
// read control file --> Name and Version
//=========================================================================================
/*
std::wstring UTF8ToUnicode(const std::string& str)
{
    std::wstring ret;
    try {
        std::wstring_convert< std::codecvt_utf8<wchar_t> > wcv;
        ret = wcv.from_bytes(str);
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
    }
    return ret;
}

std::string UnicodeToUTF8(const std::wstring& wstr)
{
    std::string ret;
    try {
        std::wstring_convert< std::codecvt_utf8<wchar_t> > wcv;
        ret = wcv.to_bytes(wstr);
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
    }
    return ret;
}

std::string UnicodeToANSI(const std::wstring& wstr)
{
    std::string ret;
    std::mbstate_t state = {};
    const wchar_t* src = wstr.data();
    size_t len = std::wcsrtombs(nullptr, &src, 0, &state);
    if (static_cast<size_t>(-1) != len) {
        std::unique_ptr< char[] > buff(new char[len + 1]);
        len = std::wcsrtombs(buff.get(), &src, len, &state);
        if (static_cast<size_t>(-1) != len) {
            ret.assign(buff.get(), len);
        }
    }
    return ret;
}
std::wstring ANSIToUnicode(const std::string& str)
{
    std::wstring ret;
    std::mbstate_t state = {};
    const char* src = str.data();
    size_t len = std::mbsrtowcs(nullptr, &src, 0, &state);
    if (static_cast<size_t>(-1) != len) {
        std::unique_ptr< wchar_t[] > buff(new wchar_t[len + 1]);
        len = std::mbsrtowcs(buff.get(), &src, len, &state);
        if (static_cast<size_t>(-1) != len) {
            ret.assign(buff.get(), len);
        }
    }
    return ret;
}
*/
string UTF8ToGB(const char* str)
{
    string result;
    WCHAR* strSrc;
    LPSTR szRes;

    int i = MultiByteToWideChar(CP_UTF8, 0, str, -1, NULL, 0)+1;
    strSrc = new WCHAR[i];
    MultiByteToWideChar(CP_UTF8, 0, str, -1, strSrc, i);

    i = WideCharToMultiByte(CP_ACP, 0, strSrc, -1, NULL, 0, NULL, NULL) + 1;
    szRes = new CHAR[i];
    WideCharToMultiByte(CP_ACP, 0, strSrc, -1, szRes, i, NULL, NULL);

    result = szRes;
    delete[]strSrc;
    delete[]szRes;

    return result;
}

int TDebFile::parseControlFile(const char* control_file, map<string,string>& out_map)
{
    ifstream fpSrc;
    fpSrc.open(control_file, ios::in);
    if (!fpSrc.is_open()) return -1;

    string strLine;
    while (getline(fpSrc, strLine))
    {
        if (strLine.empty())
            continue;
        string::size_type pos = strLine.find(':', 0);
        if (pos == string::npos) continue;

        string str_key = strLine.substr(0, pos);
        string str_value = UTF8ToGB(strLine.substr(pos+1).c_str());

        str_key.erase(0, str_key.find_first_not_of(" \t\n\r")); //trim
        str_key.erase(str_key.find_last_not_of(" \t\n\r") + 1);
        str_value.erase(0, str_value.find_first_not_of(" \t\n\r")); //trim
        str_value.erase(str_value.find_last_not_of(" \t\n\r") + 1);

        //printf("%s\t\t\t%s\n", str_key.c_str(), str_value.c_str());
        out_map.emplace(str_key, str_value);
    }
    return 1;
}