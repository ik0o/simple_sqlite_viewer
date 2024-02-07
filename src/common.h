#ifndef COMMON_H
#define COMMON_H


#include <cstdio>
#include <fstream>
#include <string>
#include <vector>
#include <map>

extern std::string tmp_folder_location;
extern bool try_adjust_char_len;

#if defined(_WIN32) || defined(_WIN64)
#define sqlr_windows
#include "windows.h"
#endif

#if defined(__linux__) || defined(__linux)
#define sqlr_linux
#include <cstdlib>
#include <filesystem>
#endif


#endif
