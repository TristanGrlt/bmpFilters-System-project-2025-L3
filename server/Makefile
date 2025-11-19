CC = gcc

CFLAGS = -std=c2x -D_XOPEN_SOURCE=501 -Wpedantic -Wall -Wextra -Wconversion -Werror -fstack-protector-all -fpie -pie -O2 -D_FORTIFY_SOURCE=2 -MMD -I../include -MP

LDFLAGS = -lm

TARGET = build/run

SRC = $(wildcard src/*.c)

OBJ = $(SRC:src/%.c=build/%.o)

DEP = $(OBJ:.o=.d)

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) -o $@ $(OBJ) $(LDFLAGS)

build/%.o: src/%.c | build
	$(CC) -c $< -o $@ $(CFLAGS)

build:
	mkdir -p build

-include $(DEP)

clean:
	rm -f $(TARGET) $(OBJ) $(DEP)

distclean: clean
	rm -f *~

.PHONY: all clean distclean
