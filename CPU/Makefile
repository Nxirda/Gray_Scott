CC=gcc

TARGET=gray_scott

CFlags=
DebugFlags=-g3 -fno-omit-frame-pointer
WFlags= -Werror -Wall -Wextra -Wconversion $(DebugFlags) -Iinclude/ 
OFlags= -O3 -march=native

SDL2=`sdl2-config --cflags --libs`
LFlags= $(SDL2) #-fopenmp 

SRC_DIR=src
INC_DIR=include
BUILD_DIR=build

# wildcard ensures that the command id expanded by the shell
SOURCES= $(wildcard $(SRC_DIR)/*.c) 
OBJECTS= $(patsubst %.c, $(BUILD_DIR)/%.o, $(notdir $(SOURCES)))

BIN= $(BUILD_DIR)/$(TARGET)

all: $(BIN)

$(BIN): $(OBJECTS)
	$(CC) $(CFlags) $(WFlags) $(OFlags) $(OBJECTS) main.c -o $@ $(LFlags)

# Pipe is used to ensure build dir is created before anything
$(BUILD_DIR)/%.o : $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFlags) $(WFlags) $(OFlags) -c $< -o $@ $(LFlags)

$(BUILD_DIR) : 
	mkdir -p $(BUILD_DIR)

clean: 
	rm -f $(OBJECTS) $(BIN) 

.PHONY: all clean
