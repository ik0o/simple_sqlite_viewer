#ifndef INTERFACE_H
#define INTERFACE_H


#include "common.h"


class Parser{
    public:
    Parser(std::string *_file_name_ptr);
    void addCommand(std::vector<std::string> _names, std::vector<std::string> *_values_ptr, int _command_id);
    int parse(int argc, char *argv[], int &command_id);

    private:
    struct Command{
        std::vector<std::string> names;
        std::vector<std::string> *values_ptr;
        int command_id;
    };
    std::vector<Command> commands;
    std::string *file_name_ptr;
};

class Config{
    private:
    std::map<std::string, std::string> data = {};
    std::string config_path;

    public:
    Config();
    Config(std::string _config_path);
    ~Config();
    int load();
    int save();
    std::string get(std::string key);
    int update(std::string key, std::string value);
};


#endif