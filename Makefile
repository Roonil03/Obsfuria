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

all: dirs $(TARGET)

install: $(TARGET)
	bash ./scripts/add_commands.sh
	sudo cp $(TARGET) /usr/local/bin/obsfuria
	sudo chmod +x /usr/local/bin/obsfuria
	@echo "Installed to ~/.local/bin/obsfuria"

dirs:
	mkdir -p $(OBJ_DIR) $(BIN_DIR)

$(TARGET): $(CPP_OBJS) $(ASM_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.asm
	$(ASM) $(ASMFLAGS) $< -o $@

clean:
	rm -rf $(OBJ_DIR)/*.o $(BIN_DIR)/* Obsfuria

.PHONY: all dirs clean

