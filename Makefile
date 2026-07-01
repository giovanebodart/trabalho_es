CC := gcc
CFLAGS := -std=c11 -Wall -Wextra -Werror -pedantic
PYTHON ?= python
SAN_ROOT ?= C:/msys64/clang64
SAN_CC := $(SAN_ROOT)/bin/clang.exe
SAN_RUNTIME_DIR := $(SAN_ROOT)/bin
SAN_FLAGS := $(CFLAGS) -g -O1 -fno-omit-frame-pointer \
	-fsanitize=address,undefined

BUILD_DIR := build
SANITIZE_DIR := $(BUILD_DIR)/sanitize
DATA_DIR := data
PLOTS_DIR := plots
EXEEXT := .exe
MKDIR_BUILD = if not exist "$(BUILD_DIR)" mkdir "$(BUILD_DIR)"
MKDIR_SANITIZE = if not exist "$(SANITIZE_DIR)" mkdir "$(SANITIZE_DIR)"
MKDIR_DATA = if not exist "$(DATA_DIR)" mkdir "$(DATA_DIR)"
MKDIR_PLOTS = if not exist "$(PLOTS_DIR)" mkdir "$(PLOTS_DIR)"
CLEAN_BUILD = if exist "$(BUILD_DIR)" rmdir /S /Q "$(BUILD_DIR)"

TEST_NAMES := test_assertions test_gc test_interval_tree test_marker \
	test_old_pages test_register_roots test_smoke test_stack_roots \
	test_sweeper
TEST_BINS := $(addprefix $(BUILD_DIR)/,$(addsuffix $(EXEEXT),$(TEST_NAMES)))
SANITIZE_BINS := $(addprefix $(SANITIZE_DIR)/,$(addsuffix $(EXEEXT),$(TEST_NAMES)))
TEST_HELPER := include/test.h
EXAMPLE_NAMES := list tree cyclic_graph fire_test_visualizer
EXAMPLE_BINS := $(addprefix $(BUILD_DIR)/example_,$(addsuffix $(EXEEXT),$(EXAMPLE_NAMES)))
SANITIZE_EXAMPLE_BINS := $(addprefix $(SANITIZE_DIR)/example_,$(addsuffix $(EXEEXT),$(EXAMPLE_NAMES)))
BENCHMARK_NAMES := scale_allocations fire_test tree compare_collectors
BENCHMARK_BINS := $(addprefix $(BUILD_DIR)/bench_,$(addsuffix $(EXEEXT),$(BENCHMARK_NAMES)))
SCALE_STRESS_MAX ?= 100000
SCALE_BENCHMARK_MAX ?= 1000000
FIRE_STRESS_MAX ?= 50000
FIRE_BENCHMARK_MAX ?= 1000000
TREE_BENCHMARK_MAX ?= 100000
COMPARE_BENCHMARK_OBJECTS ?= 50000

.PHONY: all test stress sanitize benchmark plots clean

all: $(TEST_BINS) $(EXAMPLE_BINS) $(BENCHMARK_BINS)

$(BUILD_DIR):
	$(MKDIR_BUILD)

$(SANITIZE_DIR): | $(BUILD_DIR)
	$(MKDIR_SANITIZE)

$(BUILD_DIR)/%$(EXEEXT): tests/%.c $(TEST_HELPER) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -Iinclude $< -o $@

$(SANITIZE_DIR)/%$(EXEEXT): tests/%.c $(TEST_HELPER) | $(SANITIZE_DIR)
	$(SAN_CC) $(SAN_FLAGS) -Iinclude $< -o $@

$(BUILD_DIR)/test_interval_tree$(EXEEXT): tests/test_interval_tree.c \
		src/interval_tree.c include/interval_tree.h $(TEST_HELPER) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -Iinclude tests/test_interval_tree.c \
		src/interval_tree.c -o $@

$(SANITIZE_DIR)/test_interval_tree$(EXEEXT): tests/test_interval_tree.c \
		src/interval_tree.c include/interval_tree.h $(TEST_HELPER) | $(SANITIZE_DIR)
	$(SAN_CC) $(SAN_FLAGS) -Iinclude tests/test_interval_tree.c \
		src/interval_tree.c -o $@

$(BUILD_DIR)/test_marker$(EXEEXT): tests/test_marker.c src/marker.c \
		src/interval_tree.c include/marker.h include/allocator.h \
		include/interval_tree.h $(TEST_HELPER) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -Iinclude tests/test_marker.c \
		src/marker.c src/interval_tree.c -o $@

$(SANITIZE_DIR)/test_marker$(EXEEXT): tests/test_marker.c src/marker.c \
		src/interval_tree.c include/marker.h include/allocator.h \
		include/interval_tree.h $(TEST_HELPER) | $(SANITIZE_DIR)
	$(SAN_CC) $(SAN_FLAGS) -Iinclude tests/test_marker.c \
		src/marker.c src/interval_tree.c -o $@

$(BUILD_DIR)/test_stack_roots$(EXEEXT): tests/test_stack_roots.c \
		src/stack_roots.c src/marker.c src/interval_tree.c \
		include/stack_roots.h include/marker.h include/allocator.h \
		include/interval_tree.h $(TEST_HELPER) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -Iinclude tests/test_stack_roots.c \
		src/stack_roots.c src/marker.c src/interval_tree.c -o $@

$(SANITIZE_DIR)/test_stack_roots$(EXEEXT): tests/test_stack_roots.c \
		src/stack_roots.c src/marker.c src/interval_tree.c \
		include/stack_roots.h include/marker.h include/allocator.h \
		include/interval_tree.h $(TEST_HELPER) | $(SANITIZE_DIR)
	$(SAN_CC) $(SAN_FLAGS) -Iinclude \
		tests/test_stack_roots.c src/stack_roots.c src/marker.c \
		src/interval_tree.c -o $@

$(BUILD_DIR)/test_register_roots$(EXEEXT): tests/test_register_roots.c \
		src/register_roots.c src/marker.c src/interval_tree.c \
		include/register_roots.h include/marker.h include/allocator.h \
		include/interval_tree.h $(TEST_HELPER) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -Iinclude tests/test_register_roots.c \
		src/register_roots.c src/marker.c src/interval_tree.c -o $@

$(SANITIZE_DIR)/test_register_roots$(EXEEXT): tests/test_register_roots.c \
		src/register_roots.c src/marker.c src/interval_tree.c \
		include/register_roots.h include/marker.h include/allocator.h \
		include/interval_tree.h $(TEST_HELPER) | $(SANITIZE_DIR)
	$(SAN_CC) $(SAN_FLAGS) -Iinclude \
		tests/test_register_roots.c src/register_roots.c src/marker.c \
		src/interval_tree.c -o $@

$(BUILD_DIR)/test_old_pages$(EXEEXT): tests/test_old_pages.c \
		src/old_pages.c src/allocator.c src/interval_tree.c \
		include/old_pages.h include/allocator.h include/interval_tree.h \
		$(TEST_HELPER) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -Iinclude tests/test_old_pages.c \
		src/old_pages.c src/allocator.c src/interval_tree.c -o $@

$(SANITIZE_DIR)/test_old_pages$(EXEEXT): tests/test_old_pages.c \
		src/old_pages.c src/allocator.c src/interval_tree.c \
		include/old_pages.h include/allocator.h include/interval_tree.h \
		$(TEST_HELPER) | $(SANITIZE_DIR)
	$(SAN_CC) $(SAN_FLAGS) -Iinclude \
		tests/test_old_pages.c src/old_pages.c src/allocator.c \
		src/interval_tree.c -o $@

$(BUILD_DIR)/test_sweeper$(EXEEXT): tests/test_sweeper.c src/sweeper.c \
		src/allocator.c src/interval_tree.c include/sweeper.h include/allocator.h \
		include/gc_stats.h include/interval_tree.h $(TEST_HELPER) \
		| $(BUILD_DIR)
	$(CC) $(CFLAGS) -Iinclude tests/test_sweeper.c \
		src/sweeper.c src/allocator.c src/interval_tree.c -o $@

$(SANITIZE_DIR)/test_sweeper$(EXEEXT): tests/test_sweeper.c src/sweeper.c \
		src/allocator.c src/interval_tree.c include/sweeper.h include/allocator.h \
		include/gc_stats.h include/interval_tree.h $(TEST_HELPER) \
		| $(SANITIZE_DIR)
	$(SAN_CC) $(SAN_FLAGS) -Iinclude tests/test_sweeper.c \
		src/sweeper.c src/allocator.c src/interval_tree.c -o $@

GC_SOURCES := src/gc.c src/allocator.c src/interval_tree.c src/marker.c \
		src/old_pages.c src/register_roots.c src/roots.c \
		src/stack_roots.c src/sweeper.c
GC_HEADERS := include/gc.h include/gc_config.h include/gc_stats.h \
		include/interval_tree.h include/allocator.h include/gc_internal.h \
		include/marker.h include/old_pages.h include/register_roots.h \
		include/roots.h include/stack_roots.h include/sweeper.h
GC_LIBS := -lpsapi

$(BUILD_DIR)/test_gc$(EXEEXT): tests/test_gc.c $(GC_SOURCES) $(GC_HEADERS) \
		$(TEST_HELPER) \
		| $(BUILD_DIR)
	$(CC) $(CFLAGS) -Iinclude tests/test_gc.c \
		$(GC_SOURCES) -o $@ $(GC_LIBS)

$(SANITIZE_DIR)/test_gc$(EXEEXT): tests/test_gc.c $(GC_SOURCES) $(GC_HEADERS) \
		$(TEST_HELPER) \
		| $(SANITIZE_DIR)
	$(SAN_CC) $(SAN_FLAGS) -Iinclude tests/test_gc.c \
		$(GC_SOURCES) -o $@ $(GC_LIBS)

$(BUILD_DIR)/example_%$(EXEEXT): examples/%.c $(GC_SOURCES) $(GC_HEADERS) \
		| $(BUILD_DIR)
	$(CC) $(CFLAGS) -Iinclude $< $(GC_SOURCES) -o $@ $(GC_LIBS)

$(BUILD_DIR)/bench_%$(EXEEXT): benchmarks/%.c $(GC_SOURCES) $(GC_HEADERS) \
		| $(BUILD_DIR)
	$(CC) $(CFLAGS) -Iinclude $< $(GC_SOURCES) -o $@ $(GC_LIBS)

$(SANITIZE_DIR)/example_%$(EXEEXT): examples/%.c $(GC_SOURCES) $(GC_HEADERS) \
		| $(SANITIZE_DIR)
	$(SAN_CC) $(SAN_FLAGS) -Iinclude $< $(GC_SOURCES) -o $@ $(GC_LIBS)

test: $(TEST_BINS) $(EXAMPLE_BINS)
	$(BUILD_DIR)/test_assertions$(EXEEXT)
	$(BUILD_DIR)/test_gc$(EXEEXT)
	$(BUILD_DIR)/test_interval_tree$(EXEEXT)
	$(BUILD_DIR)/test_marker$(EXEEXT)
	$(BUILD_DIR)/test_old_pages$(EXEEXT)
	$(BUILD_DIR)/test_register_roots$(EXEEXT)
	$(BUILD_DIR)/test_smoke$(EXEEXT)
	$(BUILD_DIR)/test_stack_roots$(EXEEXT)
	$(BUILD_DIR)/test_sweeper$(EXEEXT)
	$(BUILD_DIR)/example_list$(EXEEXT)
	$(BUILD_DIR)/example_tree$(EXEEXT)
	$(BUILD_DIR)/example_cyclic_graph$(EXEEXT)

stress: test $(BUILD_DIR)/bench_scale_allocations$(EXEEXT) \
		$(BUILD_DIR)/bench_fire_test$(EXEEXT)
	$(BUILD_DIR)/bench_scale_allocations$(EXEEXT) $(SCALE_STRESS_MAX)
	$(BUILD_DIR)/bench_fire_test$(EXEEXT) $(FIRE_STRESS_MAX)

benchmark: $(BENCHMARK_BINS)
	$(BUILD_DIR)/bench_scale_allocations$(EXEEXT) $(SCALE_BENCHMARK_MAX)
	$(BUILD_DIR)/bench_fire_test$(EXEEXT) $(FIRE_BENCHMARK_MAX)
	$(BUILD_DIR)/bench_tree$(EXEEXT) $(TREE_BENCHMARK_MAX)
	$(BUILD_DIR)/bench_compare_collectors$(EXEEXT) $(COMPARE_BENCHMARK_OBJECTS)

plots: $(BENCHMARK_BINS)
	$(MKDIR_DATA)
	$(MKDIR_PLOTS)
	$(subst /,\,$(BUILD_DIR)/bench_scale_allocations$(EXEEXT)) $(SCALE_BENCHMARK_MAX) --csv > $(DATA_DIR)\scale.csv
	$(subst /,\,$(BUILD_DIR)/bench_fire_test$(EXEEXT)) $(FIRE_BENCHMARK_MAX) --csv > $(DATA_DIR)\fire.csv
	$(subst /,\,$(BUILD_DIR)/bench_tree$(EXEEXT)) $(TREE_BENCHMARK_MAX) --csv > $(DATA_DIR)\tree.csv
	$(subst /,\,$(BUILD_DIR)/bench_compare_collectors$(EXEEXT)) $(COMPARE_BENCHMARK_OBJECTS) --csv --stages > $(DATA_DIR)\collectors.csv
	$(PYTHON) scripts\generate_plots.py

sanitize: $(SANITIZE_BINS) $(SANITIZE_EXAMPLE_BINS)
	set "PATH=$(SAN_RUNTIME_DIR);%PATH%" && $(subst /,\,$(SANITIZE_DIR)/test_assertions$(EXEEXT))
	set "PATH=$(SAN_RUNTIME_DIR);%PATH%" && $(subst /,\,$(SANITIZE_DIR)/test_gc$(EXEEXT))
	set "PATH=$(SAN_RUNTIME_DIR);%PATH%" && $(subst /,\,$(SANITIZE_DIR)/test_interval_tree$(EXEEXT))
	set "PATH=$(SAN_RUNTIME_DIR);%PATH%" && $(subst /,\,$(SANITIZE_DIR)/test_marker$(EXEEXT))
	set "PATH=$(SAN_RUNTIME_DIR);%PATH%" && $(subst /,\,$(SANITIZE_DIR)/test_old_pages$(EXEEXT))
	set "PATH=$(SAN_RUNTIME_DIR);%PATH%" && $(subst /,\,$(SANITIZE_DIR)/test_register_roots$(EXEEXT))
	set "PATH=$(SAN_RUNTIME_DIR);%PATH%" && $(subst /,\,$(SANITIZE_DIR)/test_smoke$(EXEEXT))
	set "PATH=$(SAN_RUNTIME_DIR);%PATH%" && $(subst /,\,$(SANITIZE_DIR)/test_stack_roots$(EXEEXT))
	set "PATH=$(SAN_RUNTIME_DIR);%PATH%" && $(subst /,\,$(SANITIZE_DIR)/test_sweeper$(EXEEXT))
	set "PATH=$(SAN_RUNTIME_DIR);%PATH%" && $(subst /,\,$(SANITIZE_DIR)/example_list$(EXEEXT))
	set "PATH=$(SAN_RUNTIME_DIR);%PATH%" && $(subst /,\,$(SANITIZE_DIR)/example_tree$(EXEEXT))
	set "PATH=$(SAN_RUNTIME_DIR);%PATH%" && $(subst /,\,$(SANITIZE_DIR)/example_cyclic_graph$(EXEEXT))
	set "PATH=$(SAN_RUNTIME_DIR);%PATH%" && $(subst /,\,$(SANITIZE_DIR)/example_fire_test_visualizer$(EXEEXT)) --demo

clean:
	$(CLEAN_BUILD)
