# Define compiler and flags
CC = gcc

# Default target
TARGET = aesdsocket 

# Object files
OBJS = aesdsocket.o

# Compiler flags
CFLAGS = -Wall -Werror

# Cross-compilation support
ifeq ($(CROSS_COMPILE),)
        # Native build
		PREFIX =
else
        # Cross-compilation
		PREFIX = $(CROSS_COMPILE)
		CC = $(PREFIX)gcc
endif

# Default target
all: $(TARGET)

$(TARGET): $(OBJS)
		$(CC) $(CFLAGS) $(INCLUDES) -o $@ $^
%.o: %.c
		$(CC) -c $< -o $@

# Clean target
clean:
		rm -f $(TARGET) $(OBJS)
