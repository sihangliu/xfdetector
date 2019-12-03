CC := gcc
CXX := g++
CFLAGS := -fPIC -g
CXXFLAGS := -fPIC -O3
INCLUDE := -Iinclude/
LIBRARY := -lm -lpthread -lboost_system -lboost_filesystem

BUILD := ./build
OBJ_DIR := $(BUILD)/obj
APP_DIR := $(BUILD)/app
LIB_DIR := $(BUILD)/lib
SRC_DIR := ./src

PINTOOL_DIR := ./pintool

all:
	mkdir -p $(OBJ_DIR) $(APP_DIR) $(LIB_DIR)
	$(CXX) -c $(CXXFLAGS) -o $(OBJ_DIR)/pmrace.o $(SRC_DIR)/pmrace.cc $(INCLUDE)
	$(CXX) $(CXX_FLAGS) -o $(APP_DIR)/pmrace $(OBJ_DIR)/pmrace.o $(LIBRARY)
	$(CC) -c $(CFLAGS) -o $(OBJ_DIR)/pmrace_interface.o $(SRC_DIR)/pmrace_interface.c $(INCLUDE)
	ar -cvq $(LIB_DIR)/libpmrace_interface.a $(OBJ_DIR)/pmrace_interface.o 
	$(CC) -shared -o $(LIB_DIR)/libpmrace_interface.so $(OBJ_DIR)/pmrace_interface.o
	make -C pintool/ clean
	make -C pintool/

clean:
	rm -rf $(BUILD)
	rm -rf $(PINTOOL_DIR)/obj-intel64/
