CC = gcc

CFLAGS = -Wall -Werror -Wextra -O3 -MMD -MP
TARGET = bin/nintecemu

ARGS = "roms/nestest.nes"

SRCS = $(wildcard src/*.c)
OBJS = $(SRCS:src/%.c=obj/%.o)
DEPS = $(OBJS:.o=.d)

.PHONY: all run clean scratch

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

run: $(TARGET)
	./$(TARGET) $(ARGS)

clean:
	rm -rf bin
	rm -rf obj

-include $(DEPS)
