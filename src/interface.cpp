#include "interface.h"


// ----------------------------------------------------------------------------

// init parser
Parser::Parser(std::string *_file_name_ptr){
    file_name_ptr = _file_name_ptr; 
}

// add command to commands vector
void Parser::addCommand(std::vector<std::string> _names, std::vector<std::string> *_values_ptr, int _command_id){
    commands.emplace_back(Command{_names, _values_ptr, _command_id});
}

// parse arguments
int Parser::parse(int argc, char *argv[], int &command_id){
    // if not enough arguments
    if(argc < 2){
        printf("lack of arguments\n");
        return 1;
    }
    // if 2 argument passed
    if(argc == 2){
        // check if second arg not file name, but command 
        for(auto &command : commands){
            for(auto &name : command.names){
                if(name == argv[1]){
                    command_id = command.command_id;
                    return 0;
                }
            }
        }
        // if second arg not command
        printf("cant find the \"%s\" command\n", argv[2]);
        return 1;
    }

    // get command ant its options
    *file_name_ptr = argv[1];
    std::string inp_command = argv[2];
    bool finish_flag = false;
    for(auto &command : commands){
        for(auto &name : command.names){
            if(name == inp_command){
                command_id = command.command_id;
                for(int i=3; i<argc; i++){
                    command.values_ptr->push_back(argv[i]);
                }
                finish_flag = true;
                break;
            }
        }
        if(finish_flag){ break; }
    }
    
    return 0;
}


// ----------------------------------------------------------------------------

// default config constructor
Config::Config(){
}

// config constructor
Config::Config(std::string _config_path){
    config_path = _config_path;
}

// default config destructor
Config::~Config(){
}

// load config
int Config::load(){
    // open config file
    std::ifstream config(config_path, std::ios::in);
    if(config.bad() || !config.is_open()){
        config.close();
        return 1;
    }

    // load config data
    std::string line;
    while(std::getline(config, line)){
        int separator = static_cast<int>(line.find("="));
        std::string key = line.substr(0,separator);
        std::string value = line.substr(separator+1);
        data[key] = value;
    }

    // close config file
    config.close();
    return 0;
}

// save current config state
int Config::save(){
    // open config file
    std::ofstream config(config_path, std::ios::out);

    // save current config data
    std::string line;
    for(auto &pair : data){
        line = pair.first + "=" + pair.second;
        config << line << "\n";
    }

    // close config file
    config.close();
    return 0;
}

// get config value
std::string Config::get(std::string key){
    if(data.find(key) == data.end()){ return ""; }
    return data[key];
}

// update config value or add new value
int Config::update(std::string key, std::string value){
    data[key] = value;
    return 0;
}


// ----------------------------------------------------------------------------