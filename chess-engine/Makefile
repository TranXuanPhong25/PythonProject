# Compiler and flags
CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O3 -g

# Directories
SRC_DIR = chess-engine
BUILD_DIR = build
BIN_DIR = bin

# Source files
SRCS = $(wildcard *.cpp)
OBJS = $(patsubst %.cpp,$(BUILD_DIR)/%.o,$(filter-out bench.cpp,$(SRCS)))


# Target executable
TARGET = $(BIN_DIR)/uci.exe

# Phony targets
.PHONY: all clean dirs

# Default target
all: dirs $(TARGET)

bench: evaluate.cpp search.cpp tt.cpp chess.cpp score_move.cpp see.cpp evaluate_features.cpp

# Create directories
dirs:
	@mkdir -p $(BUILD_DIR)
	@mkdir -p $(BIN_DIR)

# Link object files to create executable
$(TARGET): $(OBJS)
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
	@echo "Object files: $(OBJS)"
#make bench
