CC = gcc
CFLAGS = -Wall -Werror -Wextra -O3
TARGET = bin/nintencemu

SRCS = $(wildcard src/*.c)
OBJS = $(SRCS:src/%.c=obj/%.o)

.PHONY: all clean scratch

all: $(TARGET)

$(TARGET): $(OBJS) | bin
	@mkdir -p bin
	@mkdir -p obj
	$(CC) $(CFLAGS) $(OBJS) -o $(TARGET)

obj/%.o: src/%.c | obj
	@mkdir -p bin
	@mkdir -p obj
	$(CC) $(CFLAGS) -c $< -o $@

bin obj:
	@mkdir -p $@

clean:
	rm -rf bin

scratch: clean
	rm -rf obj
