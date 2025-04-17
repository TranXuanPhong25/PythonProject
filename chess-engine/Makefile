# Compiler and flags
CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O3 -g

# Directories
SRC_DIR = .
BUILD_DIR = build
BIN_DIR = bin

# Source files
SRCS = $(wildcard *.cpp)
COMMON_OBJS = $(patsubst %.cpp,$(BUILD_DIR)/%.o,$(filter-out bench.cpp uci.cpp main.cpp eval_symmetry_test.cpp,$(SRCS)))

# Target executables
TARGET = $(BIN_DIR)/uci.exe
BENCH_TARGET = $(BIN_DIR)/bench.exe
EVAL_TEST_TARGET = $(BIN_DIR)/eval_test.exe

# Phony targets
.PHONY: all clean dirs bench eval_test

# Default target
all: dirs $(TARGET)

# Benchmark target
bench: dirs $(BENCH_TARGET)
	$(BENCH_TARGET)

# Evaluation symmetry test target
eval_test: dirs $(EVAL_TEST_TARGET)
	$(EVAL_TEST_TARGET)

# Create directories
dirs:
	@mkdir -p $(BUILD_DIR)
	@mkdir -p $(BIN_DIR)

# Link object files to create UCI executable
$(TARGET): $(COMMON_OBJS) $(BUILD_DIR)/main.o $(BUILD_DIR)/uci.o
	$(CXX) $(CXXFLAGS) $^ -o $@

# Link object files to create benchmark executable
$(BENCH_TARGET): $(COMMON_OBJS) $(BUILD_DIR)/bench.o
	$(CXX) $(CXXFLAGS) $^ -o $@

# Link object files to create evaluation test executable
$(EVAL_TEST_TARGET): $(COMMON_OBJS) $(BUILD_DIR)/eval_symmetry_test.o
	$(CXX) $(CXXFLAGS) $^ -o $@

# Compile source files into object files
$(BUILD_DIR)/%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean build files
clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR)

# Debug info
debug:
	@echo "Source files: $(SRCS)"
	@echo "UCI objects: $(COMMON_OBJS) $(BUILD_DIR)/main.o $(BUILD_DIR)/uci.o"
	@echo "Benchmark objects: $(COMMON_OBJS) $(BUILD_DIR)/bench.o"
	@echo "Common objects: $(COMMON_OBJS)"
