#include "common.h"
#include "interface.h"
#include "utils.h"
#include "sql.h"


Config *config;
std::string tmp_folder_location;
bool try_adjust_char_len;

// ----------------------------------------------------------------------------

// print help
void helpCommand(){
    printf("\"sqlread\" is simple sqlite reader\n\
\n\
usage: sqlread <file_name>/<command> <command> [options]\n\
examples: \n\
  sqlread -c\n\
  sqlread test_db.db read a='aaa' and b='bbb' lim 1-100\n\
  sqlread test_db.db -r new\n\
\n\
short\\long   |  description                           |  arguments                     |\n\
=============+========================================+================================+\n\
-h   help    |  print this text                       |                                |\n\
-r   read    |  read specific data from database      |  <table> <max> <lims> <filters>|\n\
-c   clean   |  clean tmp data                        |  <table>                       |\n\
-i   info    |  print info about database and tables  |  <table>                       |\n\
-cfg config  |  configurate sqlread                   |                                |\n\
\n\
table   - specify table                : from a\n\
max     - specify max length of column : max 15\n\
lims    - specify start offset and end : lim 20-100\n\
filters - filter by values             : a='a' or b='b'\n\
\n\
read:\n\
  read all data from table or tables also you can use \"new\" option\n\
  that reads data added since last sqlread run\n\
  in filters you can use: OR, or, And, and, = (in one argument, not\n\
  a = 'a', only a='a'), LIKE, like (only a LIKE 'a', LIKE syntax is\n\
  default sqlite LIKE syntax)\n\
clean:\n\
  clean all tmp data (data about previously opened databases)\n\
  also you can specify table\n\
info:\n\
  print info about table or all tables in database, if table not specified\n\
config:\n\
  configurate sqlread: change tmp folder path\n\
");}

// run config process
int configCommand(){
    // update config tmp folder path
    std::string line;
    printf("!!! you can skip value by pressing enter !!!\n");
    printf("------------------------------------------------------------------------------------\n");
    printf("The tmp folder is the folder that will store information about the readed sqlite dbs\n");
    printf("The path to the tmp folder can be absolute or relative to sqlread exe\n");
    printf("- enter new tmp folder path: ");
    line = getLine();
    if(line != ""){
        // path string correction
        for(unsigned long long int i=0; i<line.size(); i++){
            if(line[i] == '\\'){ line[i] = '/'; }
        }
        if(line[line.size()-1] != '/'){
            line += '/';
        }
        config->update("tmp_path", line);
    }
    printf("------------------------------------------------------------------------------------\n");
    printf("The \"try adjust\" flag is a flag, when set, the program tries to adjust the length of\n");
    printf("the string with Unicode or other codepage characters (sometimes these characters can\n");
    printf("be double length)\n");
    printf("- enter new try adjust state: ");
    line = getLine();
    if(line != "" && (line == "0" || line == "1")){
        config->update("try_adjust", line);
    }
    
    // save new config state
    config->save();

    return 0;
}

// clean tmp files
int cleanCommand(std::string file_name, std::vector<std::string> options){
    std::string tmp_list_path = tmp_folder_location + "misc";
    
    // clear all tmp files
    if(file_name == ""){
        // get tmp file name from list and remove it
        std::ifstream list_file(tmp_list_path);
        std::string tmp_file_name;
        while(std::getline(list_file, tmp_file_name)){
            printf("%s\n", tmp_file_name.c_str());
            int status = deleteFile(tmp_file_name);
            if(status){
                printf("cant delete tmp file\n");
                return -1;
            }
        }
        list_file.close();

        // clear list file data
        std::ofstream tmp_list_file(tmp_list_path);
        tmp_list_file.close();
    }

    // clear one tmp file
    else{
        // get tmp file name from sql object
        std::string tmp_file_name;
        Sql *tmp_sql = new Sql(file_name);
        tmp_sql->getTmpFileName(tmp_file_name);
        delete tmp_sql;
        
        // delete tmp file
        int status = deleteFile(tmp_file_name);
        if(status){
            printf("cant delete tmp file\n");
            return -1;
        }

        // delete tmp file name from list file data
        std::string text;
        std::ifstream list_file(tmp_list_path);
        std::string line;
        while(std::getline(list_file, line)){
            if(line != tmp_file_name){
                text += line + "\n"; 
            }
        }
        list_file.close();
        std::ofstream tmp_list_file(tmp_list_path);
        tmp_list_file << text;
        tmp_list_file.close();
    }
   
    return 0;
}

// print info about tables
int infoCommand(std::string file_name, std::vector<std::string> options){
    // check options
    std::string table_name = "";
    if(static_cast<int>(options.size()) == 1){
        table_name = options[0];
    }else if(static_cast<int>(options.size()) > 1){
        printf("too many arguments \"%s\"\n", options[1].c_str());
        return -1;
    }

    Sql sql(file_name);
    sql.openDatabase();
    if(sql.openDatabase() == -1){
        printf("cant find/open/... %s\n", file_name.c_str());
        return 1;
    }

    // get and print tables info 
    printf("%s\n\n", file_name.c_str());

    std::vector<TableInfo> tables_info;
    sql.getTableInfo(table_name, tables_info);

    for(auto &table_info : tables_info){
        printf("- %s\n", table_info.table_name.c_str());
        printf("schema: %s\n", table_info.schema.c_str());
        printf("row number: %lli\n", table_info.row_number);
        printf("bytes used: %lli\n", table_info.used_bytes);
        printf("\n");
    }

    return 0;
}

// read files
int readCommand(std::string file_name, std::vector<std::string> options){
    // prepare variables for argument parsing
    std::string table_name = "";
    int max_column_len = 16;
    std::string filters = "";
    bool new_option = false;
    long long int limits[2] = {-1, -1};
    int options_size = static_cast<int>(options.size());
    std::vector<std::string> used_options;

    // argument parsing
    for(int i=0; i<options_size; i++){
        for(auto &used_option : used_options){
            if(options[i] == used_option){
                printf("\"%s\" already used\n", options[i].c_str());
                return -1;
            }
        }
        
        if(options[i] == "new"){
            new_option = true;
            used_options.push_back("new");
        }
        
        else if(options[i] == "from"){
            table_name = options[++i];
            used_options.push_back("from");
        }
        
        else if(options[i] == "max"){
            max_column_len = std::stoi(options[++i]);
            used_options.push_back("max");
        }
        
        else if(options[i] == "lim"){
            long long int sep = options[++i].find('-');
            std::string from  = options[i].substr(0, sep);
            std::string to    = options[i].substr(sep+1);
            if(static_cast<int>(from.find_first_not_of("0123456789")) != -1){
                printf("\"%s\" is not a number\n", from.c_str());
                return -1;
            }else if(static_cast<int>(to.find_first_not_of("0123456789")) != -1){
                printf("\"%s\" is not a number\n", to.c_str());
                return -1;
            }
            limits[0] = std::stoll(from) - 1;
            limits[1] = std::stoll(to);
            used_options.push_back("lim");
        }
        
        else if(static_cast<int>(options[i].find('=')) != -1 || (options[i+1] == "LIKE" || options[i+1] == "like")){
            while(i < options_size){
                if(options[i] == "and" || options[i] == "AND"){
                    filters += "AND ";
                }else if(options[i] == "or" || options[i] == "OR"){
                    filters += "OR ";
                }else if(static_cast<int>(options[i].find('=')) != -1){
                    filters += options[i] + " ";
                }else if(options[i+1] == "like" || options[i+1] == "LIKE"){
                    filters += options[i] + " LIKE " + options[i+2];
                    i+=2;
                }else{
                    i--;
                    break;
                }i++;
            }
        }
        
        else{
            printf("unexpected argument \"%s\"\n", options[i].c_str());
            return -1;
        }
    }

    // final arguments check
    int tmp_cnt = 0;
    for(auto &option : options){
        if(option == "new" || option == "lim"){ tmp_cnt++; }
    }if(tmp_cnt >= 2){
        printf("\"new\" read cannot have a \"lim\" option\n");
        return -1;
    }

    // prepare sql
    Sql sql(file_name);
    if(sql.openDatabase() == -1){
        printf("cant find/open/... %s\n", file_name.c_str());
        return 1;
    }

    sql.setMaxDisplayData(max_column_len);

    std::vector<std::string> tables_names;
    if(table_name == ""){ sql.getTablesNames(tables_names); }
    else{ tables_names.push_back(table_name); }

    // read and print rows in tables
    for(auto &table : tables_names){
        if(new_option){
            long long int tmp_offset = sql.readNewData(table);
            sql.printReadedData();
            while(tmp_offset != 0){
                if(tmp_offset == -1){
                    printf("something went wrong\n");
                    return -1;
                }
                tmp_offset = sql.readNewData(table, filters, tmp_offset);
                sql.printReadedData(0);
            }
        }else{
            long long int tmp_row_len = (limits[0] != -1) ? limits[1]-limits[0] : -1;
            long long int tmp_offset = sql.readData(table, filters, limits[0], tmp_row_len);
            sql.printReadedData();
            while(tmp_offset != 0){
                if(tmp_offset == -1){
                    printf("something went wrong\n");
                    return -1;
                }
                tmp_row_len = (limits[0] != -1) ? limits[1]-tmp_offset+1 : -1;
                tmp_offset = sql.readData(table, filters, tmp_offset, tmp_row_len);
                sql.printReadedData(0);
            }
        }
        sql.clearReadedData();
        printf("\n");
    }

    return 0;
}

// ----------------------------------------------------------------------------


int main(int argc, char *argv[]){
    // prepare variables for arguments parsing
    int status = 0;
    int command_id = 0;
    std::string file_name;
    std::vector<std::string> options;

    // check if config exist
    std::string config_file_path = exeFolderPath(argv[0]) + "config";
    if(!existCheck(config_file_path)){
        Config tmp_config(config_file_path);
        tmp_config.update("tmp_path", "./tmp/");
        tmp_config.save();
    }

    // load config and get tmp folder path
    config = new Config(config_file_path);
    status = config->load();
    if(status != 0){ return 1; }
    tmp_folder_location = config->get("tmp_path");
    if(static_cast<int>(tmp_folder_location.find("./")) != -1){
        tmp_folder_location = exeFolderPath(argv[0]) + tmp_folder_location.substr(tmp_folder_location.find("./")+2);
    }
    try_adjust_char_len = (config->get("try_adjust") == "1");

    // initialize files
    status = initFiles();
    if(status != 0){ return 1; }

    // parse arguments
    Parser parser(&file_name);
    parser.addCommand({"help",   "-h"},   &options, 0);
    parser.addCommand({"config", "-cfg"}, &options, 1);
    parser.addCommand({"clean",  "-c"},   &options, 2);
    parser.addCommand({"info",   "-i"},   &options, 3);
    parser.addCommand({"read",   "-r"},   &options, 4);
    status = parser.parse(argc, argv, command_id);
    if(status != 0){ return 1; }

    // run needed command
    switch(command_id){
    case 0: helpCommand();                              break;
    case 1: status = configCommand();                   break;
    case 2: status = cleanCommand(file_name, options);  break;
    case 3: status = infoCommand(file_name, options);   break;
    case 4: status = readCommand(file_name, options);   break;
    }
    if(status){ return 1; }

    // free last allocated data
    delete config;
    return 0;
}
