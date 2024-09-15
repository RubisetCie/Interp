# Compilers
CC = gcc

# Compiler flags
override CFLAGS += -std=c11

# Linker flags
override LDFLAGS += -flto

# Source files
SRC_FILES = \
	interp.c \
	int24.c

# Installation prefix
PREFIX = /usr/local

TARGET = interp

all: $(TARGET)

$(TARGET): $(SRC_FILES)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

install:
	mkdir -p $(PREFIX)/bin
	cp $(TARGET) $(PREFIX)/bin/

uninstall:
	rm -f $(PREFIX)/bin/$(TARGET)

clean:
	rm -f $(TARGET)
