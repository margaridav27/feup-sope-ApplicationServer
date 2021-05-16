# Name for our executable
CLIENT_EXECUTABLE := c
SERVER_EXECUTABLE := s

DELAY:=10000

# Output build files
BUILD_DIRECTORY := ./build
CLIENT_BUILD_DIRECTORY := $(BUILD_DIRECTORY)/client

# Source, include and library directories
CLIENT_SOURCE_DIRECTORIES := ./src
INCLUDE_DIRECTORIES := ./src/include

# Compiler
CC := gcc

# Find all C source files (*.c) in our source directories
CLIENT_SOURCE_FILES := $(shell find $(CLIENT_SOURCE_DIRECTORIES) -name *.c)

# Name object files after source files (append .o)
CLIENT_OBJECT_FILES := $(CLIENT_SOURCE_FILES:%=$(CLIENT_BUILD_DIRECTORY)/%.o)

# Name dependencies after object files (append .d)
DEPENDENCIES := $(OBJECT_FILES:.o=.d)

# Include directories
INCLUDE_FLAGS := $(addprefix -I, $(INCLUDE_DIRECTORIES)) -pthread

# Preprocessor flags
CPPFLAGS := $(INCLUDE_FLAGS) -MMD -MP -DDELAY=$(DELAY)

# Compiler flags
CFLAGS := -Wall -Werror -Wpedantic -std=gnu11

# Linker flags
LDFLAGS := -pthread 

.DEFAULT_TARGET: all
all: $(CLIENT_EXECUTABLE) 

$(CLIENT_BUILD_DIRECTORY)/%.c.o: %.c
	@echo "Building: " $<
	@mkdir -p $(dir $@)
	@$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@ -g


$(CLIENT_EXECUTABLE): $(CLIENT_OBJECT_FILES)
	@echo "Linking: " $@
	@$(CC) $(CLIENT_OBJECT_FILES) -o $@ $(LDFLAGS) -g

.PHONY: clean
clean:
	@rm -rf $(BUILD_DIRECTORY) $(CLIENT_EXECUTABLE)
-include $(DEPENDENCIES)
