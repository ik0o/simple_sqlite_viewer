#ifndef SQL_H
#define SQL_H


#include "common.h"
#include "sqlite3.h"
#include "utils.h"

#define MAX_DISPLAY_DATA      15
#define MAX_CHUCK_SIZE        8192


struct TableInfo{
    std::string table_name;
    long long int row_number;
    long long int used_bytes;
    std::string schema;
};

class Sql{
    private:
    struct Row{
        long long int row_id;
        std::vector<std::string> values;
    };
    struct Table{
        long long int row_number;
        long long int used_bytes;
        long long int max_row_number;
        std::string schema;
        std::vector<std::string> columns;
        std::vector<Row> *readded_rows = new std::vector<Row>;
        std::vector<int> max_column_lens;
        bool readed = false;
    };
    // tables info
    std::map<std::string, Table> prev_tables;
    std::map<std::string, Table> tables;
    // files info
    std::string name;
    std::string file_path;
    std::string tmp_file_path;
    // limits and other data
    sqlite3* db = nullptr;
    int max_display_data = MAX_DISPLAY_DATA;
    long long int max_read_data = 0;

    int loadTmpInfo();
    int saveTmpInfo();
    int updateTablesInfo();
    int getInfo();

    public:
    Sql();
    Sql(std::string _file_path);
    ~Sql();
    int openDatabase();
    long long int readData(std::string table_name = "", std::string filters = "", long long int row_offset = -1, long long int row_len = -1);
    long long int readNewData(std::string table_name = "", std::string filters = "", long long int row_offset = -1);
    int clearReadedData(std::string table_name = "");
    int printReadedData(bool header = 1);
    
    // other
    void getTablesNames(std::vector<std::string> &names);
    void getTmpFileName(std::string &out);
    void setMaxDisplayData(int _max_display_data);
    void getTableInfo(std::string table_name, std::vector<TableInfo> &table_info);
};


#endif