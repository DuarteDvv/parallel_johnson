CXX := g++
CXXFLAGS := -std=c++17 -O3 -g -Wall -Wextra -Wpedantic -fopenmp -MMD -MP
LDFLAGS := -fopenmp
INCLUDES := -I. -Isrc -Iinclude -Icommon

SRC_DIR := src
COMMON_DIR := common
BUILD_DIR := build/obj
BIN_DIR := bin
TARGET := $(BIN_DIR)/sccs

SRCS := $(wildcard $(SRC_DIR)/*.cpp) $(wildcard $(COMMON_DIR)/*.cpp)
OBJS := $(patsubst %.cpp,$(BUILD_DIR)/%.o,$(SRCS))
DEPS := $(OBJS:.o=.d)

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS) | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)

$(BIN_DIR):
	mkdir -p $@

$(BUILD_DIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR) $(TARGET)

-include $(DEPS)