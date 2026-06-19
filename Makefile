CC := gcc
CFLAGS := -std=c11 -Wall -Wextra -Werror -pedantic

BUILD_DIR := build
EXEEXT := .exe
MKDIR_BUILD = if not exist "$(BUILD_DIR)" mkdir "$(BUILD_DIR)"
CLEAN_BUILD = if exist "$(BUILD_DIR)" rmdir /S /Q "$(BUILD_DIR)"

TEST_BIN := $(BUILD_DIR)/test_smoke$(EXEEXT)

.PHONY: all test stress clean

all: $(TEST_BIN)

$(BUILD_DIR):
	$(MKDIR_BUILD)

$(TEST_BIN): tests/test_smoke.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) $< -o $@

test: $(TEST_BIN)
	$(TEST_BIN)

stress: test

clean:
	$(CLEAN_BUILD)
