# Compiler and flags
CXX = g++
CXXFLAGS = -std=c++20 -Wall -g -O2 -fno-omit-frame-pointer -fstack-protector-all

# Directories
SRC_DIR = .
BUILD_DIR = build
BIN_DIR = bin

# Source files
SRCS = $(wildcard $(SRC_DIR)/*.cpp)
COMMON_OBJS = $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(filter-out $(SRC_DIR)/bench.cpp $(SRC_DIR)/uci.cpp $(SRC_DIR)/main.cpp $(SRC_DIR)/tune.cpp $(SRC_DIR)/tune_bench.cpp $(SRC_DIR)/bot_match.cpp stockfish.cpp,$(SRCS)))

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

# Link object files to create UCI executable - explicitly list all required object files
$(TARGET): $(BUILD_DIR)/main.o $(BUILD_DIR)/uci.o $(BUILD_DIR)/chess.o $(BUILD_DIR)/evaluate.o $(BUILD_DIR)/evaluate_pieces.o $(BUILD_DIR)/evaluate_features.o $(BUILD_DIR)/evaluate_attacks.o $(BUILD_DIR)/evaluate_mobility.o $(BUILD_DIR)/search.o $(BUILD_DIR)/tunable_params.o $(BUILD_DIR)/tt.o $(BUILD_DIR)/score_move.o $(BUILD_DIR)/see.o
	$(CXX) $(CXXFLAGS) $^ -o $@ -static-libgcc -static-libstdc++

# Compile source files into object files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean build files
clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR)

# Touch files to force recompilation
rebuild: clean all

# Create parameter files if they don't exist
params:
	@echo "Creating default parameter files if they don't exist"
	@if [ ! -f "benchmark_best_params.txt" ]; then \
		echo "RFP_MARGIN 75\nRFP_DEPTH 5\nRFP_IMPROVING_BONUS 62\nLMR_BASE 75\nLMR_DIVISION 225\nNMP_BASE 3\nNMP_DIVISION 3\nNMP_MARGIN 180\nLMP_DEPTH_THRESHOLD 5\nFUTILITY_MARGIN 100\nFUTILITY_DEPTH 8\nFUTILITY_IMPROVING 50\nQS_FUTILITY_MARGIN 150\nSEE_QUIET_MARGIN_BASE -10\nSEE_NOISY_MARGIN_BASE -50\nASPIRATION_DELTA 15\nHISTORY_PRUNING_THRESHOLD 5000" > benchmark_best_params.txt; \
	fi