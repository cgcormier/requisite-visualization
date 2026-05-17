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
else
EXEEXT :=
RUN_CMD = ./$(TARGET)
CLEAN_CMD = $(RM) -r $(BUILD_DIR)
endif

TARGET ?= $(BUILD_DIR)/$(TARGET_BASE)$(EXEEXT)

SOURCES := $(SRC_DIR)/main.cpp $(SRC_DIR)/Graph.cpp $(SRC_DIR)/Course.cpp $(SRC_DIR)/DatabaseConfig.cpp
OBJECTS := $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(SOURCES))
DEPS := $(OBJECTS:.o=.d)

CPPFLAGS ?= -MMD -MP -I$(INCLUDE_DIR)
CXXFLAGS ?= -std=c++17 -Wall -Wextra -pedantic -g

.PHONY: all clean run

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) $(OBJECTS) -o $@

$(BUILD_DIR):
	mkdir $(BUILD_DIR)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp | $(BUILD_DIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

run: $(TARGET)
	$(RUN_CMD)

clean:
	$(CLEAN_CMD)

-include $(DEPS)
