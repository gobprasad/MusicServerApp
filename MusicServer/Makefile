# Declaration of variables

CROSS_COMPILE=arm-linux-gnueabihf-
CC = $(CROSS_COMPILE)gcc
CC_FLAGS = -w

# File names
EXEC = common
EXEC_LIB = lib$(EXEC).a
SOURCES = $(wildcard src/*.c)
OBJECTS = $(SOURCES:.c=.o)
INCLUDE = -I inc -I ../Common/inc -I../Lib/libao/general/include -I../Lib/mpg123/general/include
LIB_DIR = -L../Common -L../Lib/libao/arm/lib -L../Lib/mpg123/arm/lib



# File names
EXEC = MusicApp

# Main target
$(EXEC): $(OBJECTS)
	$(CC) $(OBJECTS) -o $(EXEC) ../Common/libcommon.a $(LIB_DIR) -lpthread -lao -lmpg123 -lm

# To obtain object files
%.o: %.c
	$(CC) $(INCLUDE) -c $(CC_FLAGS) -g $< -o $@

# To remove generated files
clean:
	rm -f $(EXEC) $(OBJECTS)

