CXX = g++
CXXFLAGS = -Wall -Wextra -std=c++17 -Iinclude -MMD -MP
CFLAGS = -Wall -Wextra -std=c11 -Iinclude -MMD -MP
STRIP ?= strip
ASM = nasm
ASMFLAGS = -f elf64

SRC_DIR = src
OBJ_DIR = obj
BIN_DIR = bin
TEST_DIR = tests
PREFIX ?= /usr/local
DESTDIR ?=

CPP_SRCS = $(wildcard $(SRC_DIR)/*.cpp)
CPP_OBJS = $(patsubst $(SRC_DIR)/%.cpp, $(OBJ_DIR)/%.o, $(CPP_SRCS))

C_OBJS = $(OBJ_DIR)/spatial_hash.o

ASM_SRCS = $(wildcard $(SRC_DIR)/*.asm)
ASM_OBJS = $(patsubst $(SRC_DIR)/%.asm, $(OBJ_DIR)/%.o, $(ASM_SRCS))

TARGET = $(BIN_DIR)/Obsfuria
RELEASE = $(BIN_DIR)/Obsfuria.release
TEST_TARGET0 = $(BIN_DIR)/phase0_test
TEST_TARGET1 = $(BIN_DIR)/phase1_test
TEST_TARGET2 = $(BIN_DIR)/phase2_test
TEST_TARGET3 = $(BIN_DIR)/phase3_test
TEST_TARGET4 = $(BIN_DIR)/phase4_test
TEST_TARGET5 = $(BIN_DIR)/phase5_test
TEST_TARGET6 = $(BIN_DIR)/phase6_test
TEST_COMMAND_CONFIG = $(BIN_DIR)/command_config_test

TEST_TARGETS = $(TEST_TARGET0) $(TEST_TARGET1) $(TEST_TARGET2) $(TEST_TARGET3) $(TEST_TARGET4) $(TEST_TARGET5) $(TEST_TARGET6) $(TEST_COMMAND_CONFIG)

ARSENAL_HPP = include/arsenal.hpp

all: $(TARGET)

release: $(RELEASE)

$(RELEASE): $(TARGET)
	@set -eu; tmp=$$(mktemp "$@.XXXXXX"); trap 'rm -f -- "$$tmp"' EXIT; \
		$(STRIP) --strip-all -o "$$tmp" "$<"; chmod 755 "$$tmp"; mv -T -- "$$tmp" "$@"

export RECIPIENT
encrypted-release: $(RELEASE)
	bash scripts/encrypt_release.sh "$(RELEASE)"

check-stage:
	@if [ -n "$(DESTDIR)" ]; then \
		stage=$$(realpath -m -- "$(DESTDIR)"); \
		for path in "$(DESTDIR)$(PREFIX)/bin/obsfuria" \
			"$(DESTDIR)$(PREFIX)/share/obsfuria/assets/default_commands.txt" \
			"$(DESTDIR)$(PREFIX)/share/obsfuria/assets/crazy_commands.txt"; do \
			resolved=$$(realpath -m -- "$$path"); \
			case "$$resolved" in "$$stage"/*) ;; *) \
				echo "Refusing path outside DESTDIR: $$path" >&2; exit 1;; \
			esac; \
		done; \
	fi

install: check-stage
	@if [ ! -x "$(TARGET)" ]; then \
		echo "$(TARGET) is missing; run 'make' as your regular user first." >&2; exit 1; \
	fi
	@if ! $(MAKE) --no-print-directory -q $(TARGET); then \
		echo "$(TARGET) is out of date; run 'make' as your regular user first." >&2; exit 1; \
	fi
	install -Dm755 "$(TARGET)" "$(DESTDIR)$(PREFIX)/bin/obsfuria"
	install -Dm644 assets/default_commands.txt "$(DESTDIR)$(PREFIX)/share/obsfuria/assets/default_commands.txt"
	install -Dm644 assets/crazy_commands.txt "$(DESTDIR)$(PREFIX)/share/obsfuria/assets/crazy_commands.txt"
	@echo "Installed to $(DESTDIR)$(PREFIX)/bin/obsfuria"

$(OBJ_DIR) $(BIN_DIR):
	mkdir -p "$@"

$(ARSENAL_HPP): scripts/add_commands.sh assets/default_commands.txt
	bash ./scripts/add_commands.sh

# These translation units include the generated header; make the dependency
# explicit so parallel builds cannot compile them while the generator writes it.
$(OBJ_DIR)/exec_mock.o $(OBJ_DIR)/game_loop.o: $(ARSENAL_HPP)

$(TARGET): $(CPP_OBJS) $(C_OBJS) $(ASM_OBJS) | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $^

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.asm | $(OBJ_DIR)
	$(ASM) $(ASMFLAGS) $< -o $@

$(TEST_TARGET0): $(TEST_DIR)/phase0_test.cpp $(OBJ_DIR)/terminal.o | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $^

$(TEST_TARGET1): $(TEST_DIR)/phase1_test.cpp $(ARSENAL_HPP) | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $<

$(TEST_TARGET2): $(TEST_DIR)/phase2_test.cpp $(OBJ_DIR)/spatial_manager.o $(C_OBJS) $(ASM_OBJS) | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $^

$(TEST_TARGET3): $(TEST_DIR)/phase3_test.cpp $(OBJ_DIR)/sprite_gen.o $(OBJ_DIR)/ui_renderer.o $(OBJ_DIR)/sanitizer.o | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $^

$(TEST_TARGET4): $(TEST_DIR)/phase4_test.cpp $(OBJ_DIR)/stat_calc.o $(OBJ_DIR)/reward_system.o $(OBJ_DIR)/terminal.o $(OBJ_DIR)/ui_renderer.o | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $^

$(TEST_TARGET5): $(TEST_DIR)/phase5_test.cpp $(OBJ_DIR)/exec_mock.o $(OBJ_DIR)/err_simulate.o $(OBJ_DIR)/terminal.o $(OBJ_DIR)/save_engine.o $(OBJ_DIR)/spatial_manager.o $(C_OBJS) $(ASM_OBJS) | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $^

$(TEST_TARGET6): $(TEST_DIR)/phase6_test.cpp $(OBJ_DIR)/save_engine.o $(OBJ_DIR)/spatial_manager.o $(C_OBJS) $(ASM_OBJS) | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $^

$(TEST_COMMAND_CONFIG): $(TEST_DIR)/command_config_test.cpp $(OBJ_DIR)/command_config.o | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $^

ifneq ($(wildcard $(TEST_DIR)/phase0_test.cpp),)
test: $(TEST_TARGETS)
	@set -e; for test in $(TEST_TARGETS); do \
		echo "Running $$test"; \
		"$$test"; \
	done

else
test:
	@echo "The phase test sources are local. See CONTRIBUTING.md for verification instructions." >&2
	@exit 1
endif

uninstall: check-stage
	rm -f "$(DESTDIR)$(PREFIX)/bin/obsfuria" \
		"$(DESTDIR)$(PREFIX)/share/obsfuria/assets/default_commands.txt" \
		"$(DESTDIR)$(PREFIX)/share/obsfuria/assets/crazy_commands.txt"
	@echo "Uninstalled from $(DESTDIR)$(PREFIX)"

clean:
	rm -f $(CPP_OBJS) $(CPP_OBJS:.o=.d) $(C_OBJS) $(C_OBJS:.o=.d) $(ASM_OBJS) $(RELEASE) $(TARGET) $(TEST_TARGETS) $(TEST_TARGETS:%=%.d)

.PHONY: all release encrypted-release clean test install uninstall check-stage

-include $(CPP_OBJS:.o=.d) $(C_OBJS:.o=.d)
