# Define compiler and flags
CC ?= gcc

TARGET = aesdsocket
OBJS = aesdsocket.o

CFLAGS ?= -Wall -Werror
LDFLAGS ?=
LDLIBS ?=

# Cross-compilation support
ifeq ($(CROSS_COMPILE),)
    PREFIX =
else
    PREFIX = $(CROSS_COMPILE)
    CC := $(PREFIX)gcc
endif

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) $(LDLIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(TARGET) $(OBJS)

