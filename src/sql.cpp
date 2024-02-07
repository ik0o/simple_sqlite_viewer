#include "sql.h"


// ----------------------------------------------------------------------------

// default sql constructor
Sql::Sql(){
    file_path = ".";
    name = "a";
    tmp_file_path = tmp_folder_location + name;
}

// sql constructor
Sql::Sql(std::string _file_path){
    // get file path
    file_path = _file_path;
    
    // get file name
    int tmp_start = _file_path.find_last_of('/');
    if(tmp_start == -1){ tmp_start = _file_path.find_last_of('\\'); }
    if(tmp_start == -1){ tmp_start = 0; }
    else{ tmp_start += 1; }
    int tmp_end = _file_path.find_last_of('.');
    if(tmp_end == -1){ tmp_end = _file_path.size(); }
    name = _file_path.substr(tmp_start, tmp_end-tmp_start);
    
    // get tmp_file_path
    std::string full_file_path = absolutePath(file_path);
    for(int i=0; i<static_cast<int>(full_file_path.size()); i++){
        char c = full_file_path[i];
        if(c == '/'){ tmp_file_path += "#-#"; continue; }
        if(c == '.'){ tmp_file_path += "#_#"; continue; }
        if(c == ':'){ tmp_file_path += "#=#"; continue; }
        tmp_file_path += c;
    }
    tmp_file_path = tmp_folder_location + tmp_file_path;
}

// default sql destructor
Sql::~Sql(){
    // delete all created tables
    for(auto &table : tables){
        delete table.second.readded_rows;
    }
    // delete all created previously opened tables
    for(auto &table : prev_tables){
        delete table.second.readded_rows;
    }
    // close sqlite db if it is not closed 
    if(db != nullptr){ sqlite3_close(db); }
}
 
// set max suze of text and blob display data 
void Sql::setMaxDisplayData(int _max_display_data){
    max_display_data = _max_display_data;
}

// get names of tables
void Sql::getTablesNames(std::vector<std::string> &names){
    for(auto &table : tables){
        names.push_back(table.first);
    }
}

// get tmp file name
void Sql::getTmpFileName(std::string &out){
    out = tmp_file_path;
}

// get tables info to TableInfo vector
void Sql::getTableInfo(std::string table_name, std::vector<TableInfo> &table_info){
    if(table_name == ""){
        for(auto &table : tables){
            table_info.push_back( TableInfo{table.first, table.second.row_number, table.second.used_bytes, table.second.schema} );
        }
    }else{
        if(tables.find(table_name) == tables.end()){ return; }
        Table table = tables[table_name];
        table_info.push_back( TableInfo{table_name, table.row_number, table.used_bytes, table.schema} );
    }
}

// open database
int Sql::openDatabase(){
    // check file existense
    if(!existCheck(file_path)){ return -1; }
    // open databse
    int status = sqlite3_open(file_path.c_str(), &db);
    if(status){ return -1; }
    // get info about database
    status = getInfo();
    if(status){ return -1; }
    // get previous info about database and put current info
    status = loadTmpInfo();
    if(status){ return -1; }
    status = saveTmpInfo();
    if(status){ return -1; }

    return 0;
}

// read data from database table
long long int Sql::readData(std::string table_name, std::string filters, long long int row_offset, long long int row_len){
    // check if table exist
    if(table_name == ""){ table_name = tables.begin()->first; }
    auto table = tables.find(table_name);
    if(table == tables.end()){ return -1; }

    // if it is empty table
    if(table->second.row_number == 0){
        table->second.readed = true;
        return 0;
    }

    // prepare sqlite and create query
    std::string sql_query = "SELECT *,rowid FROM " + table->first; 
    if(row_offset != -1){
        if(row_len == -1){ row_len = table->second.max_row_number; }
        sql_query += " LIMIT " + std::to_string(row_offset) + "," + std::to_string(row_len);
    }
    if(filters != ""){
        sql_query += " WHERE " + filters;
    }
    sqlite3_stmt *statement = NULL;
    int status = sqlite3_prepare_v2(db, sql_query.c_str(), sql_query.size()+2, &statement, 0);
    if(status != 0 || statement == NULL){
        return -1;
    }
    
    // get data to readed_rows and update max val size in column
    table->second.readded_rows->clear();
    table->second.columns.clear();
    table->second.max_column_lens.clear();
    int col_num = sqlite3_column_count(statement);
    for(int i=0; i<col_num-1; i++){
        table->second.columns.push_back(std::string(sqlite3_column_name(statement, i)));
        table->second.max_column_lens.push_back(static_cast<int>(table->second.columns[i].size()));
    }
    table->second.max_column_lens.push_back(0);

    bool flag = false;
    while(!flag){
        if(static_cast<int>(table->second.readded_rows->size()) >= table->second.max_row_number){
            table->second.readed = true;
            return row_offset;
        }
        status = sqlite3_step(statement);
        if(status == SQLITE_ROW){
            Row tmp_row;
            // get values in row except last column with row_id
            for(int col_i=0; col_i<col_num-1; col_i++){
                int value_len = 0;
                int type = sqlite3_column_type(statement, col_i);
                std::string value;

                if(type == SQLITE_INTEGER){
                    value = std::to_string(sqlite3_column_int(statement, col_i));
                    value_len = static_cast<int>(value.size());
                }
                
                else if(type == SQLITE_FLOAT){
                    value = std::to_string(sqlite3_column_double(statement, col_i));
                    value_len = static_cast<int>(value.size());
                }
                
                else if(type == SQLITE3_TEXT){
                    int encoding = sqlite3_value_encoding(sqlite3_column_value(statement, col_i));
                    int byte_num = sqlite3_column_bytes(statement, col_i);
                    const unsigned char *text = sqlite3_column_text(statement, col_i);
                    value = reinterpret_cast<const char*>(text);

                    // multiByte funcs used because of text encoding used in sqlite
                    if(encoding == SQLITE_UTF8){
                        value = value.substr(0, multiByteIndex(value, max_display_data));
                        if(byte_num > max_display_data){ value += "..."; }
                        value_len = static_cast<int>(multiByteLength(value));
                    }else if(encoding == SQLITE_UTF16){
                        value = value.substr(0, multiByteIndex(value, max_display_data)*2);
                        if(byte_num > max_display_data*2){ value += "\0.\0.\0."; }
                        value_len = static_cast<int>(multiByteLength(value));
                    }
                }
                
                else if(type == SQLITE_BLOB){
                    int byte_num = sqlite3_column_bytes(statement, col_i);
                    const unsigned char *data = reinterpret_cast<const unsigned char*>(sqlite3_column_blob(statement, col_i));
                    
                    // converting bytes to hex
                    value = "0x";
                    for(int i=0; i<byte_num && i<max_display_data; i++){
                        value += toHex<char>(data[i]);                    
                    }
                    
                    // add ascii representation
                    if(byte_num > max_display_data){ value += "..."; }
                    value += " (";
                    for(int i=0; i<byte_num && i<max_display_data; i++){
                        value += static_cast<char>(data[i]);                 
                    }
                    if(byte_num > max_display_data){ value += "..."; }
                    value += ")";

                    value_len = static_cast<int>(value.size());
                }
                
                else if(type == SQLITE_NULL){
                    value = "NULL";
                    value_len = static_cast<int>(value.size());
                }

                tmp_row.values.push_back(value);
                if(table->second.max_column_lens[col_i] < value_len){
                    table->second.max_column_lens[col_i] = value_len;
                }
            }
        
            // set row_id and update max row id
            tmp_row.row_id = sqlite3_column_int(statement, col_num-1);
            if(table->second.max_column_lens[col_num-1] < tmp_row.row_id){
                table->second.max_column_lens[col_num-1] = tmp_row.row_id;
            } 
            table->second.readded_rows->push_back(tmp_row);
            row_offset = tmp_row.row_id;
        }else if(status == SQLITE_DONE){
            flag = true;
        }else{
            return -1;
        }
    }

    // finalize
    sqlite3_finalize(statement);
    table->second.readed = true;

    return 0;
}

// read the data that has been added since the last opening
long long int Sql::readNewData(std::string table_name, std::string filters, long long int row_offset){
    if(table_name == ""){ table_name = tables.begin()->first; }

    // if whole table is new
    if(prev_tables.find(table_name) == tables.end()){ 
        long long int result = readData(table_name, filters, row_offset);
        if(result){ return result; }
    }

    // if table readed previously
    else{
        long long int from = prev_tables[table_name].row_number;
        if(row_offset != -1){
            if(row_offset < from){ return -1; }
            from = row_offset;
        }
        if(tables[table_name].row_number - from <= 0){ return 0; }
        long long int result = readData(table_name, filters, from);
        if(result){ return result; }
    }

    return 0;
}

// clear readed data
int Sql::clearReadedData(std::string table_name){
    if(table_name == ""){
        for(auto &table : tables){
            table.second.readded_rows->clear();
            table.second.readed = false;
        }
    }else if(tables.find(table_name) == tables.end()){
        return -1;
    }else{
        tables[table_name].readded_rows->clear();
        tables[table_name].readed = false;
    }

    return 0;
}

// print previously readed rows
int Sql::printReadedData(bool header){
    for(auto &table : tables){
        if(table.second.readed && table.second.readded_rows->size() > 0){
            if(header){
                int table_len = static_cast<int>(std::to_string(table.second.max_column_lens.back()).size()) + 3;
                for(int col_i=0; col_i<static_cast<int>(table.second.columns.size()); col_i++){
                    table_len += table.second.max_column_lens[col_i] + 1;
                }

                // draw table tittle
                int tittle_len = static_cast<int>(table.first.size());
                int gap_len = (table_len - tittle_len) / 2;
                for(int i=0; i<table_len; i++){
                    if(i < gap_len-1){
                        printf("=");
                    }else if(i < gap_len + tittle_len + 1){
                        printf(" %s ", table.first.c_str());
                        i += tittle_len + 1;
                    }else{
                        printf("=");
                    }
                }
                printf("\n");

                // draw table header
                printf("%*c|", static_cast<int>(std::to_string(table.second.max_column_lens.back()).size()) + 3, ' ');            
                for(int col_i=0; col_i<static_cast<int>(table.second.columns.size()); col_i++){
                    std::string name = table.second.columns[col_i];
                    int name_len = static_cast<int>(name.size());
                    int space_len = table.second.max_column_lens[col_i] - name_len + 1;
                    printf("%s%*c", name.c_str(), space_len, ' ');
                }
                printf("\n");

                // draw table header separater
                for(int i=0; i<table_len; i++){
                    printf("-");
                }
                printf("\n");
            }

            // draw table values
            for(long long int row_i=0; row_i<static_cast<int>(table.second.readded_rows->size()); row_i++){
                Row tmp_row = table.second.readded_rows->at(row_i);
                int value_len = static_cast<int>(std::to_string(tmp_row.row_id).size());
                int space_len = static_cast<int>(std::to_string(table.second.max_column_lens.back()).size()) - value_len + 1;
                printf("[%lli]%*c|", tmp_row.row_id, space_len, ' ');
                for(int i=0; i<static_cast<int>(tmp_row.values.size()); i++){
                    std::string value = tmp_row.values[i];
                    value_len = static_cast<int>(multiByteLength(value));
                    space_len = table.second.max_column_lens[i] - value_len + 1;
                    printf("%s%*c", value.c_str(), space_len, ' ');
                }
                printf("\n");
            }
        }else if(table.second.readed){
            printf("==== %s ====\nempty\n\n", table.first.c_str());
        }
    }

    return 0;
}

// tmp is file that contain prev info about database
// get info from tmp files
int Sql::loadTmpInfo(){    
    std::ifstream tmp_file(tmp_file_path.c_str());

    // if the tmp file does not yet exist
    if(tmp_file.bad() || !tmp_file.is_open()){
        std::ofstream list_file(tmp_folder_location + "misc", std::ios::app);
        list_file << tmp_file_path + "\n";
        list_file.close();
        return 0;
    }

    // get tmp info
    std::string line;
    while(std::getline(tmp_file, line)){
        std::string var = line.substr(0, line.find('='));
        std::string val = line.substr(line.find('=')+1);
        prev_tables[var].row_number = std::stoi(val);
    }

    tmp_file.close();
    return 0;
}

// put info to tmp file
int Sql::saveTmpInfo(){
    std::string data;
    for(auto &table : tables){
        data += table.first + '=' + std::to_string(table.second.row_number) + "\n";
    }

    std::ofstream tmp_file(tmp_file_path.c_str());
    tmp_file << data;
    tmp_file.close();

    return 0;
}

// update the list of table names
int Sql::updateTablesInfo(){
    // prepare sqlite
    std::string query = "SELECT name, sql FROM sqlite_master WHERE type=\"table\" AND name NOT LIKE \"sqlite_%\"";
    sqlite3_stmt *statement = NULL;
    int status = sqlite3_prepare_v2(db, query.c_str(), query.size()+2, &statement, 0);
    if(status != 0 || statement == NULL){
        return -1;
    }

    // get table name
    const unsigned char* name;
    const unsigned char* schema;
    bool flag = false;
    while(!flag){
        status = sqlite3_step(statement);
        switch(status){
        case SQLITE_ROW:
            // bytes = sqlite3_column_bytes(statement, 0);
            name = sqlite3_column_text(statement, 0);
            schema = sqlite3_column_text(statement, 1);
            tables[std::string(reinterpret_cast<const char*>(name))].schema = std::string(reinterpret_cast<const char*>(schema));
            break;
        case SQLITE_DONE:
            flag = true;
            break;
        default:
            return -1;
        }
    }
    sqlite3_finalize(statement);

    // get number of rows per table
    for(auto &table : tables){
        // prepare sqlite
        statement = NULL;
        query = "SELECT Count(*) FROM " + table.first;
        int status = sqlite3_prepare_v2(db, query.c_str(), query.size()+2, &statement, 0);
        if(status != 0 || statement == NULL){
            return -1;
        }

        // get number of rows per table
        long long int size;
        status = sqlite3_step(statement);
        switch(status){
        case SQLITE_ROW:
            size = sqlite3_column_int64(statement, 0);
            table.second.row_number = size;
            break;
        default:
            return -1;
        }
        sqlite3_finalize(statement);
    }

    // get number of used byte per table
    for(auto &table : tables){
        // prepare sqlite
        statement = NULL;
        query = "SELECT (SUM(pgsize)-SUM(unused)) FROM \"dbstat\" WHERE name = \"" + table.first + "\"";
        int status = sqlite3_prepare_v2(db, query.c_str(), query.size()+2, &statement, 0);
        if(status != 0 || statement == NULL){
            return -1;
        }

        // get number of used bytes per table
        long long int size;
        status = sqlite3_step(statement);
        switch(status){
        case SQLITE_ROW:
            size = sqlite3_column_int64(statement, 0);
            table.second.used_bytes = size;
            break;
        default:
            return -1;
        }
        // calculate max number of row avaliable per read
        if(table.second.row_number != 0 && table.second.used_bytes != 0){
            table.second.max_row_number = MAX_CHUCK_SIZE / (table.second.used_bytes / table.second.row_number);
        }

        sqlite3_finalize(statement);
    }

    return 0;
}

// get info about database
int Sql::getInfo(){
    int status = updateTablesInfo();
    if(status){ return -1; }
    
    // future get info
    
    return 0;
}


// ----------------------------------------------------------------------------
