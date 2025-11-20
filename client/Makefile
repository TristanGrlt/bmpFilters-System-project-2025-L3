CC = gcc

CFLAGS = -std=c2x -D_XOPEN_SOURCE=500 -Wpedantic -Wall -Wextra -Wconversion -Werror -fstack-protector-all -fpie -pie -O2 -D_FORTIFY_SOURCE=2 -MMD -I../include -MP

LDFLAGS = -lm -lrt

TARGET = build/client_bmp

SRC_LOCAL = $(wildcard src/*.c)
SHARED_SRC = ../shared/full_io.c

OBJ = $(SRC_LOCAL:src/%.c=build/%.o) build/full_io.o

DEP = $(OBJ:.o=.d)

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) -o $@ $(OBJ) $(LDFLAGS)

build/%.o: src/%.c | build
	$(CC) -c $< -o $@ $(CFLAGS)

build/full_io.o: $(SHARED_SRC) | build
	$(CC) -c $< -o $@ $(CFLAGS)

build:
	mkdir -p build

-include $(DEP)

clean:
	rm -f $(TARGET) $(OBJ) $(DEP)

distclean: clean
	rm -f *~

.PHONY: all clean distclean
