CC = gcc
CXX = g++

EXE = sqlread
SRCS_DIR = ./src/
MAIN = $(SRCS_DIR)main.cpp
SOURCES = $(SRCS_DIR)utils.cpp $(SRCS_DIR)sql.cpp $(SRCS_DIR)interface.cpp
HEADERS = $(addsuffix .h, $(basename $(SOURCES)))
OBJECTS = $(addsuffix .o, $(basename $(notdir $(SOURCES))))
OBJECTS += sqlite3.o
CXXFLAGS = -Wall -Wformat
UNAME_S = $(shell uname -s)
DEL_S = rm -rf

ifeq ($(UNAME_S), Linux)
	CXXFLAGS += -std=c++17
endif
ifeq ($(UNAME_S), Windows_NT)
	CXXFLAGS += -std=c++14
	DEL_S = del
endif

all:$(EXE)
	@echo Build complete $(ECHO_MESSAGE)

$(EXE):$(OBJECTS) $(MAIN) $(HEADERS)
	$(CXX) -o $@ $(MAIN) $(OBJECTS) $(CXXFLAGS) $(LIBS)

%.o:$(SRCS_DIR)%.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

%.o:$(SRCS_DIR)sqlite3.c
	$(CC) -c -o sqlite3.o $(SRCS_DIR)sqlite3.c -DSQLITE_ENABLE_DBSTAT_VTAB


clean:
	$(DEL_S) $(OBJECTS)
