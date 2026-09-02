CXX = g++
CXXFLAGS = -Wall -Wextra -std=c++17 -Iinclude
ASM = nasm
ASMFLAGS = -f elf64

SRC_DIR = src
OBJ_DIR = obj
BIN_DIR = bin
TEST_DIR = tests

CPP_SRCS = $(wildcard $(SRC_DIR)/*.cpp)
CPP_OBJS = $(patsubst $(SRC_DIR)/%.cpp, $(OBJ_DIR)/%.o, $(CPP_SRCS))

ASM_SRCS = $(wildcard $(SRC_DIR)/*.asm)
ASM_OBJS = $(patsubst $(SRC_DIR)/%.asm, $(OBJ_DIR)/%.o, $(ASM_SRCS))

TEST_CPP_OBJS = $(filter-out $(OBJ_DIR)/main.o, $(CPP_OBJS))

TARGET = $(BIN_DIR)/Obsfuria
TEST_TARGET0 = $(BIN_DIR)/phase0_test
TEST_TARGET1 = $(BIN_DIR)/phase1_test
TEST_TARGET2 = $(BIN_DIR)/phase2_test
TEST_TARGET3 = $(BIN_DIR)/phase3_test
TEST_TARGET4 = $(BIN_DIR)/phase4_test
TEST_TARGET5 = $(BIN_DIR)/phase5_test
TEST_TARGET6 = $(BIN_DIR)/phase6_test

ARSENAL_HPP = include/arsenal.hpp

all: dirs $(ARSENAL_HPP) $(TARGET)

install: all
	rm -f /usr/local/bin/obsfuria
	cp $(TARGET) /usr/local/bin/obsfuria
	chmod +x /usr/local/bin/obsfuria
	@echo "Installed to /usr/local/bin/obsfuria"

dirs:
	mkdir -p $(OBJ_DIR) $(BIN_DIR)

$(ARSENAL_HPP):
	bash ./scripts/add_commands.sh

$(TARGET): $(CPP_OBJS) $(ASM_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.asm
	$(ASM) $(ASMFLAGS) $< -o $@

$(TEST_TARGET0): $(TEST_DIR)/phase0_test.cpp $(OBJ_DIR)/terminal.o
	$(CXX) $(CXXFLAGS) -o $@ $^

$(TEST_TARGET1): $(TEST_DIR)/phase1_test.cpp $(ARSENAL_HPP)
	$(CXX) $(CXXFLAGS) -o $@ $<

$(TEST_TARGET2): $(TEST_DIR)/phase2_test.cpp $(OBJ_DIR)/spatial_manager.o $(ASM_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

$(TEST_TARGET3): $(TEST_DIR)/phase3_test.cpp $(OBJ_DIR)/sprite_gen.o $(OBJ_DIR)/ui_renderer.o $(OBJ_DIR)/sanitizer.o
	$(CXX) $(CXXFLAGS) -o $@ $^

$(TEST_TARGET4): $(TEST_DIR)/phase4_test.cpp $(OBJ_DIR)/stat_calc.o $(OBJ_DIR)/reward_system.o $(OBJ_DIR)/terminal.o $(OBJ_DIR)/ui_renderer.o
	$(CXX) $(CXXFLAGS) -o $@ $^

$(TEST_TARGET5): $(TEST_DIR)/phase5_test.cpp $(OBJ_DIR)/exec_mock.o $(OBJ_DIR)/err_simulate.o $(OBJ_DIR)/terminal.o $(OBJ_DIR)/save_engine.o $(OBJ_DIR)/spatial_manager.o $(ASM_OBJS) $(ARSENAL_HPP)
	$(CXX) $(CXXFLAGS) -o $@ $^

$(TEST_TARGET6): $(TEST_DIR)/phase6_test.cpp $(OBJ_DIR)/save_engine.o $(OBJ_DIR)/spatial_manager.o $(ASM_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

test: $(TEST_TARGET0) $(TEST_TARGET1) $(TEST_TARGET2) $(TEST_TARGET3) $(TEST_TARGET4) $(TEST_TARGET5) $(TEST_TARGET6)

uninstall:
	rm -f /usr/local/bin/obsfuria
	@echo "Uninstalled from /usr/local/bin/obsfuria"

clean:
	rm -f $(OBJ_DIR)/*.o $(BIN_DIR)/* Obsfuria

.PHONY: all dirs clean test install uninstall

