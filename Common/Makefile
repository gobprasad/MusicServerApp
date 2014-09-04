# Declaration of variables

CROSS_COMPILE=arm-linux-gnueabihf-
CC = $(CROSS_COMPILE)gcc
CC_FLAGS = -w

# File names
EXEC = common
EXEC_LIB = lib$(EXEC).a
SOURCES = $(wildcard src/*.c)
OBJECTS = $(SOURCES:.c=.o)
INCLUDE = -I inc

# Main target
all: $(OBJECTS)
	ar rcs $(EXEC_LIB) $(OBJECTS) 

test: all
	$(CC) $(INCLUDE) test/listTest.c -o testbin libcommon.a -lpthread -g

# To obtain object files
%.o: %.c
	$(CC) $(INCLUDE) -c $(CC_FLAGS) -g $< -o $@

# To remove generated files
clean:
	rm -f $(EXEC_LIB) $(OBJECTS) test/listTest.o testbin
