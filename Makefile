CXX ?= g++
TARGET_BASE ?= requisite-visualization
BUILD_DIR ?= build
BACKEND_DIR ?= backend
SRC_DIR := $(BACKEND_DIR)/src
INCLUDE_DIR := $(BACKEND_DIR)/include

ifeq ($(OS),Windows_NT)
EXEEXT := .exe
RUN_CMD = ./$(TARGET)
POWERSHELL := C:/Windows/System32/WindowsPowerShell/v1.0/powershell.exe
CLEAN_CMD = "$(POWERSHELL)" -NoProfile -Command "Remove-Item -Recurse -Force -ErrorAction SilentlyContinue '$(BUILD_DIR)'; exit 0"
MKDIR_BUILD = if not exist "$(BUILD_DIR)" mkdir "$(BUILD_DIR)"
MSYS2_UCRT_BIN ?= C:/msys64/ucrt64/bin
ifneq ($(wildcard $(MSYS2_UCRT_BIN)/g++.exe),)
export PATH := $(MSYS2_UCRT_BIN);$(PATH)
endif
else
EXEEXT :=
RUN_CMD = ./$(TARGET)
CLEAN_CMD = $(RM) -r $(BUILD_DIR)
MKDIR_BUILD = mkdir -p $(BUILD_DIR)
endif

TARGET ?= $(BUILD_DIR)/$(TARGET_BASE)$(EXEEXT)
TEST_CPP_TARGET ?= $(BUILD_DIR)/test-graph$(EXEEXT)

CORE_SOURCES := $(filter-out $(SRC_DIR)/main.cpp,$(wildcard $(SRC_DIR)/*.cpp))
APP_SOURCES := $(SRC_DIR)/main.cpp $(CORE_SOURCES)
APP_OBJECTS := $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(APP_SOURCES))
CORE_OBJECTS := $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(CORE_SOURCES))
TEST_CPP_SOURCE := tests/cpp/test_graph.cpp
TEST_CPP_OBJECT := $(BUILD_DIR)/test_graph.o
DEPS := $(APP_OBJECTS:.o=.d) $(TEST_CPP_OBJECT:.o=.d)

CPPFLAGS ?= -MMD -MP -I$(INCLUDE_DIR)
CXXFLAGS ?= -std=c++17 -Wall -Wextra -pedantic -g

.PHONY: all clean run test test-cpp

all: $(TARGET)

$(TARGET): $(APP_OBJECTS)
	$(CXX) $(APP_OBJECTS) -o $@

$(TEST_CPP_TARGET): $(TEST_CPP_OBJECT) $(CORE_OBJECTS)
	$(CXX) $(TEST_CPP_OBJECT) $(CORE_OBJECTS) -o $@

$(BUILD_DIR):
	$(MKDIR_BUILD)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp | $(BUILD_DIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

$(TEST_CPP_OBJECT): $(TEST_CPP_SOURCE) | $(BUILD_DIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

run: $(TARGET)
	$(RUN_CMD)

test: test-cpp

test-cpp: $(TEST_CPP_TARGET)
	./$(TEST_CPP_TARGET)

clean:
	$(CLEAN_CMD)

-include $(DEPS)
