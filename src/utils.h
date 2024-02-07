#ifndef UTILS_H
#define UTILS_H


#include "common.h"


// convert int to hex string
template <typename I> std::string toHex(I num, unsigned long long int hex_len = sizeof(I)<<1) {
    static const char* alph = "0123456789ABCDEF";
    std::string res(hex_len,'0');
    for(unsigned long long int i=0, j=(hex_len-1)*4 ; i<hex_len; i++,j-=4)
        res[i] = alph[(num>>j) & 0x0f];
    return res;
}

std::string exeFolderPath(std::string exe_name);
std::string absolutePath(std::string file_path);
int deleteFile(std::string file_path);
int createDir(std::string dir_path);
int existCheck(std::string path);
int initFiles();
std::string getLine();
long long unsigned int multiByteLength(std::string str);
long long unsigned int multiByteIndex(std::string str, long long unsigned int char_num);


#endif
