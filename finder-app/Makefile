# Define compiler and flags
CC = gcc

# Default target
TARGET = writer

# Object files
OBJS = writer.o

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
		$(CC) -o $@ $^
%.o: %.c
		 $(CC) -c $< -o $@

# Clean target
clean:
		rm -f $(TARGET) $(OBJS)
