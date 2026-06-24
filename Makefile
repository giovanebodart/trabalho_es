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

TEST_NAMES := test_assertions test_gc test_interval_tree test_marker \
	test_register_roots test_smoke test_stack_roots test_sweeper
TEST_BINS := $(addprefix $(BUILD_DIR)/,$(addsuffix $(EXEEXT),$(TEST_NAMES)))
SANITIZE_BINS := $(addprefix $(SANITIZE_DIR)/,$(addsuffix $(EXEEXT),$(TEST_NAMES)))
EXAMPLE_NAMES := list tree cyclic_graph
EXAMPLE_BINS := $(addprefix $(BUILD_DIR)/example_,$(addsuffix $(EXEEXT),$(EXAMPLE_NAMES)))
SANITIZE_EXAMPLE_BINS := $(addprefix $(SANITIZE_DIR)/example_,$(addsuffix $(EXEEXT),$(EXAMPLE_NAMES)))
BENCHMARK_NAMES := scale_allocations
BENCHMARK_BINS := $(addprefix $(BUILD_DIR)/bench_,$(addsuffix $(EXEEXT),$(BENCHMARK_NAMES)))
SCALE_STRESS_MAX ?= 100000
SCALE_BENCHMARK_MAX ?= 1000000

.PHONY: all test stress sanitize benchmark clean

all: $(TEST_BINS) $(EXAMPLE_BINS) $(BENCHMARK_BINS)

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

$(BUILD_DIR)/test_marker$(EXEEXT): tests/test_marker.c src/marker.c \
		src/interval_tree.c src/marker.h src/allocator.h \
		include/interval_tree.h tests/test.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -Iinclude -Isrc -Itests tests/test_marker.c \
		src/marker.c src/interval_tree.c -o $@

$(SANITIZE_DIR)/test_marker$(EXEEXT): tests/test_marker.c src/marker.c \
		src/interval_tree.c src/marker.h src/allocator.h \
		include/interval_tree.h tests/test.h | $(SANITIZE_DIR)
	$(SAN_CC) $(SAN_FLAGS) -Iinclude -Isrc -Itests tests/test_marker.c \
		src/marker.c src/interval_tree.c -o $@

$(BUILD_DIR)/test_stack_roots$(EXEEXT): tests/test_stack_roots.c \
		src/stack_roots.c src/marker.c src/interval_tree.c \
		src/stack_roots.h src/marker.h src/allocator.h \
		include/interval_tree.h tests/test.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -Iinclude -Isrc -Itests tests/test_stack_roots.c \
		src/stack_roots.c src/marker.c src/interval_tree.c -o $@

$(SANITIZE_DIR)/test_stack_roots$(EXEEXT): tests/test_stack_roots.c \
		src/stack_roots.c src/marker.c src/interval_tree.c \
		src/stack_roots.h src/marker.h src/allocator.h \
		include/interval_tree.h tests/test.h | $(SANITIZE_DIR)
	$(SAN_CC) $(SAN_FLAGS) -Iinclude -Isrc -Itests \
		tests/test_stack_roots.c src/stack_roots.c src/marker.c \
		src/interval_tree.c -o $@

$(BUILD_DIR)/test_register_roots$(EXEEXT): tests/test_register_roots.c \
		src/register_roots.c src/marker.c src/interval_tree.c \
		src/register_roots.h src/marker.h src/allocator.h \
		include/interval_tree.h tests/test.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -Iinclude -Isrc -Itests tests/test_register_roots.c \
		src/register_roots.c src/marker.c src/interval_tree.c -o $@

$(SANITIZE_DIR)/test_register_roots$(EXEEXT): tests/test_register_roots.c \
		src/register_roots.c src/marker.c src/interval_tree.c \
		src/register_roots.h src/marker.h src/allocator.h \
		include/interval_tree.h tests/test.h | $(SANITIZE_DIR)
	$(SAN_CC) $(SAN_FLAGS) -Iinclude -Isrc -Itests \
		tests/test_register_roots.c src/register_roots.c src/marker.c \
		src/interval_tree.c -o $@

$(BUILD_DIR)/test_sweeper$(EXEEXT): tests/test_sweeper.c src/sweeper.c \
		src/allocator.c src/interval_tree.c src/sweeper.h src/allocator.h \
		include/gc_stats.h include/interval_tree.h tests/test.h \
		| $(BUILD_DIR)
	$(CC) $(CFLAGS) -Iinclude -Isrc -Itests tests/test_sweeper.c \
		src/sweeper.c src/allocator.c src/interval_tree.c -o $@

$(SANITIZE_DIR)/test_sweeper$(EXEEXT): tests/test_sweeper.c src/sweeper.c \
		src/allocator.c src/interval_tree.c src/sweeper.h src/allocator.h \
		include/gc_stats.h include/interval_tree.h tests/test.h \
		| $(SANITIZE_DIR)
	$(SAN_CC) $(SAN_FLAGS) -Iinclude -Isrc -Itests tests/test_sweeper.c \
		src/sweeper.c src/allocator.c src/interval_tree.c -o $@

GC_SOURCES := src/gc.c src/allocator.c src/interval_tree.c src/marker.c \
		src/register_roots.c src/roots.c src/stack_roots.c src/sweeper.c
GC_HEADERS := include/gc.h include/gc_config.h include/gc_stats.h \
		include/interval_tree.h src/allocator.h src/gc_internal.h \
		src/marker.h src/register_roots.h src/roots.h src/stack_roots.h \
		src/sweeper.h

$(BUILD_DIR)/test_gc$(EXEEXT): tests/test_gc.c $(GC_SOURCES) $(GC_HEADERS) \
		tests/test.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -Iinclude -Isrc -Itests tests/test_gc.c \
		$(GC_SOURCES) -o $@

$(SANITIZE_DIR)/test_gc$(EXEEXT): tests/test_gc.c $(GC_SOURCES) $(GC_HEADERS) \
		tests/test.h | $(SANITIZE_DIR)
	$(SAN_CC) $(SAN_FLAGS) -Iinclude -Isrc -Itests tests/test_gc.c \
		$(GC_SOURCES) -o $@

$(BUILD_DIR)/example_%$(EXEEXT): examples/%.c $(GC_SOURCES) $(GC_HEADERS) \
		| $(BUILD_DIR)
	$(CC) $(CFLAGS) -Iinclude -Isrc $< $(GC_SOURCES) -o $@

$(BUILD_DIR)/bench_%$(EXEEXT): benchmarks/%.c $(GC_SOURCES) $(GC_HEADERS) \
		| $(BUILD_DIR)
	$(CC) $(CFLAGS) -Iinclude -Isrc $< $(GC_SOURCES) -o $@

$(SANITIZE_DIR)/example_%$(EXEEXT): examples/%.c $(GC_SOURCES) $(GC_HEADERS) \
		| $(SANITIZE_DIR)
	$(SAN_CC) $(SAN_FLAGS) -Iinclude -Isrc $< $(GC_SOURCES) -o $@

test: $(TEST_BINS) $(EXAMPLE_BINS)
	$(BUILD_DIR)/test_assertions$(EXEEXT)
	$(BUILD_DIR)/test_gc$(EXEEXT)
	$(BUILD_DIR)/test_interval_tree$(EXEEXT)
	$(BUILD_DIR)/test_marker$(EXEEXT)
	$(BUILD_DIR)/test_register_roots$(EXEEXT)
	$(BUILD_DIR)/test_smoke$(EXEEXT)
	$(BUILD_DIR)/test_stack_roots$(EXEEXT)
	$(BUILD_DIR)/test_sweeper$(EXEEXT)
	$(BUILD_DIR)/example_list$(EXEEXT)
	$(BUILD_DIR)/example_tree$(EXEEXT)
	$(BUILD_DIR)/example_cyclic_graph$(EXEEXT)

stress: test $(BUILD_DIR)/bench_scale_allocations$(EXEEXT)
	$(BUILD_DIR)/bench_scale_allocations$(EXEEXT) $(SCALE_STRESS_MAX)

benchmark: $(BUILD_DIR)/bench_scale_allocations$(EXEEXT)
	$(BUILD_DIR)/bench_scale_allocations$(EXEEXT) $(SCALE_BENCHMARK_MAX)

sanitize: $(SANITIZE_BINS) $(SANITIZE_EXAMPLE_BINS)
	set "PATH=$(SAN_RUNTIME_DIR);%PATH%" && $(subst /,\,$(SANITIZE_DIR)/test_assertions$(EXEEXT))
	set "PATH=$(SAN_RUNTIME_DIR);%PATH%" && $(subst /,\,$(SANITIZE_DIR)/test_gc$(EXEEXT))
	set "PATH=$(SAN_RUNTIME_DIR);%PATH%" && $(subst /,\,$(SANITIZE_DIR)/test_interval_tree$(EXEEXT))
	set "PATH=$(SAN_RUNTIME_DIR);%PATH%" && $(subst /,\,$(SANITIZE_DIR)/test_marker$(EXEEXT))
	set "PATH=$(SAN_RUNTIME_DIR);%PATH%" && $(subst /,\,$(SANITIZE_DIR)/test_register_roots$(EXEEXT))
	set "PATH=$(SAN_RUNTIME_DIR);%PATH%" && $(subst /,\,$(SANITIZE_DIR)/test_smoke$(EXEEXT))
	set "PATH=$(SAN_RUNTIME_DIR);%PATH%" && $(subst /,\,$(SANITIZE_DIR)/test_stack_roots$(EXEEXT))
	set "PATH=$(SAN_RUNTIME_DIR);%PATH%" && $(subst /,\,$(SANITIZE_DIR)/test_sweeper$(EXEEXT))
	set "PATH=$(SAN_RUNTIME_DIR);%PATH%" && $(subst /,\,$(SANITIZE_DIR)/example_list$(EXEEXT))
	set "PATH=$(SAN_RUNTIME_DIR);%PATH%" && $(subst /,\,$(SANITIZE_DIR)/example_tree$(EXEEXT))
	set "PATH=$(SAN_RUNTIME_DIR);%PATH%" && $(subst /,\,$(SANITIZE_DIR)/example_cyclic_graph$(EXEEXT))

clean:
	$(CLEAN_BUILD)
