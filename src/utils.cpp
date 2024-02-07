#include "utils.h"


// ----------------------------------------------------------------------------

// get absolute of current exe (if exe folder saved in path or smth like that)
std::string exeFolderPath(std::string exe_name){
    std::string res_path = "";

    #ifdef sqlr_windows
    char buffer[4096];
    DWORD length = GetModuleFileNameA(NULL, buffer, 4096);
    if(length == 0){ return ""; }
    res_path = buffer;
    int last_separator = 0;
    for(int i=0; i<static_cast<int>(res_path.size()); i++){
        if(res_path[i] == '\\'){ res_path[i] = '/'; }
        if(res_path[i] == '/'){
            last_separator = i;
        }
    }
    res_path = res_path.substr(0,last_separator+1);
    #endif

    #ifdef sqlr_linux
    if(std::filesystem::exists(exe_name)){
        return std::string(std::filesystem::absolute(exe_name).parent_path()) + "/";
    }

    std::string path_var = std::getenv("PATH");
    while(1){
        unsigned long long int separator = path_var.find(":");
        std::string one_path = path_var.substr(0, separator);
        if(one_path[one_path.size()-1] != '/'){ one_path += '/'; }
        if(std::filesystem::exists(one_path+exe_name)){
            res_path = one_path;
            break;
        }
        path_var = path_var.substr(separator+1);
    }
    #endif

    return res_path;
}

// get absolute path to the file
std::string absolutePath(std::string file_path){
    std::string full_file_path = "";
    
    #ifdef sqlr_windows
    TCHAR** tmp_lpstr={NULL};
    char buffer[4096];
    int status = GetFullPathName(file_path.c_str(), 4096, buffer, tmp_lpstr);
    if(!status){
        return "";
    }
    for(int i=0; i<4096; i++){
        if(buffer[i] == 0){ break; }
        else if(buffer[i] == '\\'){ buffer[i] = '/'; }
        full_file_path += buffer[i];
    }
    full_file_path = buffer;
    #endif

    #ifdef sqlr_linux
    full_file_path = std::filesystem::absolute(file_path);
    #endif

    return full_file_path;
}

// delete file
int deleteFile(std::string file_path){
    #ifdef sqlr_windows
    int status = DeleteFileA(file_path.c_str());
    if(!status){
        return -1;
    }
    #endif

    #ifdef sqlr_linux
    bool status = std::filesystem::remove(file_path);
    if(!status){
        return -1;
    }
    #endif

    return 0;
}

// create directory
int createDir(std::string dir_path){
    #ifdef sqlr_windows
    int status = CreateDirectory(dir_path.c_str(), NULL);
    if(!status){
        return -1;
    }
    #endif

    #ifdef sqlr_linux
    bool status = std::filesystem::create_directory(dir_path);
    if(!status){
        return -1;
    }
    #endif

    return 0;
}

// check if file or dir exist
int existCheck(std::string path){
    #ifdef sqlr_windows
    DWORD atrb = GetFileAttributesA(path.c_str());
    if(atrb == INVALID_FILE_ATTRIBUTES){
        return 0;
    }
    #endif
    
    #ifdef sqlr_linux
    bool status = std::filesystem::exists(path);
    if(!status){
        return 0;
    }
    #endif

    return 1;
}

// init files (create tmp directory and so on)
int initFiles(){
    if(!existCheck(tmp_folder_location)){
        int status = createDir(tmp_folder_location);
        if(status){
            return -1;
        }
    }
    return 0;
}

// get line from stdin
std::string getLine(){
    std::string res;
    int c = getchar();
    while(c != '\n' && c != -1){
        res += (char)c;
        c = getchar();
    }
    return res;
}

// get length of multibyte encoding string
long long unsigned int multiByteLength(std::string str){
    long long unsigned int length = 0;
    for(long long unsigned int i=0; i<str.size(); i++){
        unsigned char byte = str[i];
        if((byte & 0b11100000) == 0b11000000){                    // if 1 more byte further
            if((str.size() < i+2) || ((unsigned char)(str[i+1]) & 0b11000000) == 0b10000000){
                i++;
            }
        }else if((byte & 0b11110000) == 0b11100000){              // if 2 more byte further
            bool flag = true;
            for(int j=1; j<3; j++){
                if((str.size() < i+j+1) || ((unsigned char)(str[i+j]) & 0b11000000) != 0b10000000){
                    flag = false; break;
                }
            }
            if(flag){ i+=2;}
            if(try_adjust_char_len){ length++; }
        }else if((byte & 0b11111000) == 0b11110000){              // if 3 more byte further
            bool flag = true;
            for(int j=1; j<4; j++){
                if((str.size() < i+j+1) || ((unsigned char)(str[i+j]) & 0b11000000) != 0b10000000){
                    flag = false; break;
                }
            }
            if(flag){ i+=3; }
            if(try_adjust_char_len){ length++; }
        }
        length++;
    }
    return length;
}

// get the byte index at which the character count reaches the specified point
long long unsigned int multiByteIndex(std::string str, long long unsigned int char_num){
    long long unsigned int index;
    long long unsigned int length = 0;
    for(index=0; index<str.size() && length<char_num; index++){
        unsigned char byte = str[index];
        if((byte & 0b11100000) == 0b11000000){                    // if 1 more byte further
            if((str.size() < index+2) || ((unsigned char)(str[index+1]) & 0b11000000) == 0b10000000){
                index++;
            }
        }else if((byte & 0b11110000) == 0b11100000){              // if 2 more byte further
            bool flag = true;
            for(int j=1; j<3; j++){
                if((str.size() < index+j+1) || ((unsigned char)(str[index+j]) & 0b11000000) != 0b10000000){
                    flag = false; break;
                }
            }
            if(flag){ index+=2; }
            if(try_adjust_char_len){ length++; }
        }else if((byte & 0b11111000) == 0b11110000){              // if 3 more byte further
            bool flag = true;
            for(int j=1; j<4; j++){
                if((str.size() < index+j+1) || ((unsigned char)(str[index+j]) & 0b11000000) != 0b10000000){
                    flag = false; break;
                }
            }
            if(flag){ index+=3; }
            if(try_adjust_char_len){ length++; }
        }
        length++;
    }
    return index;
}


// ----------------------------------------------------------------------------
