CFLAGS := -fPIC -g -Wall
CXXFLAGS := -fPIC -O3 -g -Wall
INCLUDE := -Iinclude/
LIBRARY := -lm -lpthread -lboost_system -lboost_filesystem

PMFUZZ_INCLUDE := -I$(shell pwd)/../../../include/
PMFUZZ_LIB := -Wl,-R$(shell pwd)/../../../build/ -lpmfuzz -DPMFUZZ

BUILD := ./build
OBJ_DIR := $(BUILD)/obj
APP_DIR := $(BUILD)/app
LIB_DIR := $(BUILD)/lib
SRC_DIR := ./src

DIRS    := $(OBJ_DIR) $(APP_DIR) $(LIB_DIR)

DEPENDS := include/common.hh include/trace.hh include/xfdetector.hh

PINTOOL_DIR := ./pintool

all: dirs $(APP_DIR)/xfdetector $(LIB_DIR)/xfdetector_interface.a $(LIB_DIR)/libxfdetector_interface.so
	make -C pintool/

dirs: $(OBJ_DIR) $(APP_DIR) $(LIB_DIR)

$(OBJ_DIR) $(APP_DIR) $(LIB_DIR):
	@mkdir -p $@

$(LIB_DIR)/libxfdetector_interface.so: $(OBJ_DIR)/xfdetector_interface.o
	$(CC) -shared -o $@ $<

$(LIB_DIR)/xfdetector_interface.a: $(OBJ_DIR)/xfdetector_interface.o
	ar -cvq $@ $<

$(OBJ_DIR)/xfdetector_interface.o: $(SRC_DIR)/xfdetector_interface.c
	$(CC) -c $(CFLAGS) -o $@ $< $(INCLUDE) $(PMFUZZ_INCLUDE) $(PMFUZZ_LIB)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cc $(DEPENDS)
	$(CXX) $(CXXFLAGS) -c -o $@ $< $(INCLUDE)

$(APP_DIR)/xfdetector: $(OBJ_DIR)/xfdetector.o $(OBJ_DIR)/shadow_pm.o $(OBJ_DIR)/exec_ctrl.o
	$(CXX) $(CXX_FLAGS) -o $@  $^ $(LIBRARY)


clean:
	make -C pintool/ clean
	rm -rf $(BUILD)
	rm -rf $(PINTOOL_DIR)/obj-intel64/
