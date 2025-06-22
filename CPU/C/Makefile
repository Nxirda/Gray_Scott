CC=gcc

TARGET=gray_scott

SDL2=`sdl2-config --cflags --libs`

CFlags=-std=c2x

ifdef DEBUG
   CFlags+=-g3 -fno-omit-frame-pointer
else ifdef BENCHMARK
	CFlags+=-g3 -fno-omit-frame-pointer -DNDEBUG
else
   CFlags+=-DNDEBUG
endif

ifdef DOUBLE
   CFlags+=-DDOUBLE_PRECISION
endif

WFlags= -Werror -Wall -Wextra -Wconversion -Wpedantic -Iinclude/ 
OFlags= -Ofast -march=native -funroll-loops -flto 

LFlags= $(SDL2) -fopenmp 

SRC_DIR=src
INC_DIR=include
BUILD_DIR=build

# wildcard ensures that the command is expanded by the shell
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
