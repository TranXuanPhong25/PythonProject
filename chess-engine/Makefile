# Compiler and flags
CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O3 -g

# Directories
SRC_DIR = .
BUILD_DIR = build
BIN_DIR = bin

# Source files
SRCS = $(wildcard $(SRC_DIR)/*.cpp)
COMMON_OBJS = $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(filter-out $(SRC_DIR)/bench.cpp $(SRC_DIR)/uci.cpp $(SRC_DIR)/main.cpp $(SRC_DIR)/tune.cpp $(SRC_DIR)/tune_bench.cpp $(SRC_DIR)/bot_match.cpp,$(SRCS)))

# Target executables
TARGET = $(BIN_DIR)/uci.exe
BENCH_TARGET = $(BIN_DIR)/bench.exe
TUNE_TARGET = $(BIN_DIR)/tune.exe
TUNE_BENCH_TARGET = $(BIN_DIR)/tune_bench.exe
BOT_MATCH_TARGET = $(BIN_DIR)/bot_match.exe

# Phony targets
.PHONY: all clean dirs bench tune tune_bench bot_match

# Default target
all: dirs $(TARGET)

# Benchmark target
bench: dirs $(BENCH_TARGET)
	$(BENCH_TARGET)

# Tuning target
tune: dirs $(TUNE_TARGET)
	$(TUNE_TARGET)

# Benchmark-based tuning target
tune_bench: dirs $(TUNE_BENCH_TARGET)
	$(TUNE_BENCH_TARGET)

# Bot match target
bot_match: dirs $(BOT_MATCH_TARGET)
	$(BOT_MATCH_TARGET)

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

# Link object files to create tuning executable
$(TUNE_TARGET): $(COMMON_OBJS) $(BUILD_DIR)/tune.o
	$(CXX) $(CXXFLAGS) $^ -o $@

# Link object files to create benchmark-based tuning executable
$(TUNE_BENCH_TARGET): $(COMMON_OBJS) $(BUILD_DIR)/tune_bench.o
	$(CXX) $(CXXFLAGS) $^ -o $@

# Link object files to create bot match executable
$(BOT_MATCH_TARGET): $(COMMON_OBJS) $(BUILD_DIR)/bot_match.o
	$(CXX) $(CXXFLAGS) $^ -o $@

# Compile source files into object files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Display all object files that will be built
list-objects:
	@echo "Common objects:"
	@for obj in $(COMMON_OBJS); do echo "  $$obj"; done
	@echo "UCI objects: $(COMMON_OBJS) $(BUILD_DIR)/main.o $(BUILD_DIR)/uci.o"
	@echo "Benchmark objects: $(COMMON_OBJS) $(BUILD_DIR)/bench.o"
	@echo "Tune objects: $(COMMON_OBJS) $(BUILD_DIR)/tune.o"
	@echo "Tune-bench objects: $(COMMON_OBJS) $(BUILD_DIR)/tune_bench.o"
	@echo "Bot-match objects: $(COMMON_OBJS) $(BUILD_DIR)/bot_match.o"

# Clean build files
clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR)

# Debug info
debug:
	@echo "Source files: $(SRCS)"
	@echo "UCI objects: $(COMMON_OBJS) $(BUILD_DIR)/main.o $(BUILD_DIR)/uci.o"
	@echo "Benchmark objects: $(COMMON_OBJS) $(BUILD_DIR)/bench.o"
	@echo "Tune objects: $(COMMON_OBJS) $(BUILD_DIR)/tune.o"
	@echo "Tune-bench objects: $(COMMON_OBJS) $(BUILD_DIR)/tune_bench.o"
	@echo "Bot-match objects: $(COMMON_OBJS) $(BUILD_DIR)/bot_match.o"
