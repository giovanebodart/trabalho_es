CC := gcc
CFLAGS := -std=c11 -Wall -Wextra -Werror -pedantic
SAN_ROOT ?= C:/msys64/clang64
SAN_CC := $(SAN_ROOT)/bin/clang.exe
SAN_RUNTIME_DIR := $(SAN_ROOT)/bin
SAN_FLAGS := $(CFLAGS) -g -O1 -fno-omit-frame-pointer \
	-fsanitize=address,undefined

BUILD_DIR := build
SANITIZE_DIR := $(BUILD_DIR)/sanitize
EXEEXT := .exe
MKDIR_BUILD = if not exist "$(BUILD_DIR)" mkdir "$(BUILD_DIR)"
MKDIR_SANITIZE = if not exist "$(SANITIZE_DIR)" mkdir "$(SANITIZE_DIR)"
CLEAN_BUILD = if exist "$(BUILD_DIR)" rmdir /S /Q "$(BUILD_DIR)"

TEST_NAMES := test_assertions test_gc test_interval_tree test_smoke
TEST_BINS := $(addprefix $(BUILD_DIR)/,$(addsuffix $(EXEEXT),$(TEST_NAMES)))
SANITIZE_BINS := $(addprefix $(SANITIZE_DIR)/,$(addsuffix $(EXEEXT),$(TEST_NAMES)))

.PHONY: all test stress sanitize clean

all: $(TEST_BINS)

$(BUILD_DIR):
	$(MKDIR_BUILD)

$(SANITIZE_DIR): | $(BUILD_DIR)
	$(MKDIR_SANITIZE)

$(BUILD_DIR)/%$(EXEEXT): tests/%.c tests/test.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -Itests $< -o $@

$(SANITIZE_DIR)/%$(EXEEXT): tests/%.c tests/test.h | $(SANITIZE_DIR)
	$(SAN_CC) $(SAN_FLAGS) -Itests $< -o $@

$(BUILD_DIR)/test_interval_tree$(EXEEXT): tests/test_interval_tree.c \
		src/interval_tree.c include/interval_tree.h tests/test.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -Iinclude -Itests tests/test_interval_tree.c \
		src/interval_tree.c -o $@

$(SANITIZE_DIR)/test_interval_tree$(EXEEXT): tests/test_interval_tree.c \
		src/interval_tree.c include/interval_tree.h tests/test.h | $(SANITIZE_DIR)
	$(SAN_CC) $(SAN_FLAGS) -Iinclude -Itests tests/test_interval_tree.c \
		src/interval_tree.c -o $@

GC_SOURCES := src/gc.c src/allocator.c src/interval_tree.c
GC_HEADERS := include/gc.h include/gc_config.h include/gc_stats.h \
		include/interval_tree.h src/allocator.h src/gc_internal.h

$(BUILD_DIR)/test_gc$(EXEEXT): tests/test_gc.c $(GC_SOURCES) $(GC_HEADERS) \
		tests/test.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -Iinclude -Isrc -Itests tests/test_gc.c \
		$(GC_SOURCES) -o $@

$(SANITIZE_DIR)/test_gc$(EXEEXT): tests/test_gc.c $(GC_SOURCES) $(GC_HEADERS) \
		tests/test.h | $(SANITIZE_DIR)
	$(SAN_CC) $(SAN_FLAGS) -Iinclude -Isrc -Itests tests/test_gc.c \
		$(GC_SOURCES) -o $@

test: $(TEST_BINS)
	$(BUILD_DIR)/test_assertions$(EXEEXT)
	$(BUILD_DIR)/test_gc$(EXEEXT)
	$(BUILD_DIR)/test_interval_tree$(EXEEXT)
	$(BUILD_DIR)/test_smoke$(EXEEXT)

stress: test

sanitize: $(SANITIZE_BINS)
	set "PATH=$(SAN_RUNTIME_DIR);%PATH%" && $(subst /,\,$(SANITIZE_DIR)/test_assertions$(EXEEXT))
	set "PATH=$(SAN_RUNTIME_DIR);%PATH%" && $(subst /,\,$(SANITIZE_DIR)/test_gc$(EXEEXT))
	set "PATH=$(SAN_RUNTIME_DIR);%PATH%" && $(subst /,\,$(SANITIZE_DIR)/test_interval_tree$(EXEEXT))
	set "PATH=$(SAN_RUNTIME_DIR);%PATH%" && $(subst /,\,$(SANITIZE_DIR)/test_smoke$(EXEEXT))

clean:
	$(CLEAN_BUILD)
