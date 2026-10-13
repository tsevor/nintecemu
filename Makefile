CC = gcc
CFLAGS += -Wall -Werror -Wextra -O3 -MMD -MP -flto $(shell pkg-config --cflags gtk4 libadwaita-1)
LDFLAGS += $(shell pkg-config --libs gtk4 libadwaita-1)

ARGS = "roms/nestest.nes"

TARGET = bin/nintecemu

SRCS = $(wildcard src/*.c)
OBJS = $(SRCS:src/%.c=obj/%.o)
DEPS = $(OBJS:.o=.d)

.PHONY: all run clean

all: $(TARGET)

$(TARGET): $(OBJS) | bin
	@mkdir -p bin
	@mkdir -p obj
	$(CC) $(CFLAGS) $(OBJS) -o $(TARGET) $(LDFLAGS)

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
