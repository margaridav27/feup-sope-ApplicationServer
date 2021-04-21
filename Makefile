# Name for our executable
TARGET_EXECUTABLE := c

# Output build files
BUILD_DIRECTORY := ./build

# Source, include and library directories
SOURCE_DIRECTORIES := ./src
INCLUDE_DIRECTORIES := ./src/include
LIBRARY_DIRECTORIES := ./lib

# Compiler
CC := gcc

# Find all C source files (*.c) in our source directories
SOURCE_FILES := $(shell find $(SOURCE_DIRECTORIES) -name *.c)

# Name object files after source files (append .o)
OBJECT_FILES := $(SOURCE_FILES:%=$(BUILD_DIRECTORY)/%.o)

# Include object files from libraries
OBJECT_FILES := $(OBJECT_FILES) $(shell find $(LIBRARY_DIRECTORIES) -name *.o)

# Name dependencies after object files (append .d)
DEPENDENCIES := $(OBJECT_FILES:.o=.d)

# Include directories
INCLUDE_FLAGS := $(addprefix -I, $(INCLUDE_DIRECTORIES)) -pthread

# Preprocessor flags
CPPFLAGS := $(INCLUDE_FLAGS) -MMD -MP -DDELAY=0

# Compiler flags
CFLAGS := -Wall -Werror -Wpedantic -Wextra -std=gnu11

# Linker flags
LDFLAGS := -pthread 

.DEFAULT_TARGET: all
all: $(TARGET_EXECUTABLE)

$(BUILD_DIRECTORY)/%.c.o: %.c
	@echo "Building: " $< 
	@mkdir -p $(dir $@)
	@$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@ -g

$(TARGET_EXECUTABLE): $(OBJECT_FILES)
	@echo "Linking: " $@
	@$(CC) $(OBJECT_FILES) -o $@ $(LDFLAGS) -g

.PHONY: clean
clean:
	@rm -rf $(BUILD_DIRECTORY) $(TARGET_EXECUTABLE)
-include $(DEPENDENCIES)
