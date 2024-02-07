# sqlread 

it's simple cli sqlite reader for windows and linux

## usage

general usage: sqlread <file_name>/<command> <command> [options]
examples:
  sqlread -c
  sqlread test_db.db read a='aaa' and b='bbb' lim 1-100
  sqlread test_db.db -r new

## commands

short\long   |  description                           |  arguments                     |
=============+========================================+================================+
-h   help    |  print this text                       |                                |
-r   read    |  read specific data from database      |  <table> <max> <lims> <filters>|
-c   clean   |  clean tmp data                        |  <table>                       |
-i   info    |  print info about database and tables  |  <table>                       |
-cfg config  |  configurate sqlread                   |                                |

### read:
read all data from table or tables, also you can use "new" option
that reads data added since last sqlread run
in filters you can use: OR, or, And, and, = (in one argument, not
a = 'a', only a='a'), LIKE, like (only a LIKE 'a', LIKE syntax is
default sqlite LIKE syntax)

### clean:
clean all tmp data (data about previously opened databases)
also you can specify table

### info:
print info about table or all tables in database, if table not specified

### config:
configurate sqlread internal config
