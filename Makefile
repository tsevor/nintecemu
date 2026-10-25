CC = gcc
CFLAGS += -Wall -Werror -Wextra -O3 -MMD -MP -flto -Iinclude $(shell pkg-config --cflags gtk4 libadwaita-1)
LDFLAGS += $(shell pkg-config --libs gtk4 libadwaita-1)

ARGS = "roms/AccuracyCoin.nes"

TARGET = bin/nintecemu

SRCS = $(shell find src -name '*.c')
OBJS = $(SRCS:src/%.c=obj/%.o)
DEPS = $(OBJS:.o=.d)

.PHONY: all run clean

all: $(TARGET)

$(TARGET): $(OBJS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(OBJS) -o $(TARGET) $(LDFLAGS)

obj/%.o: src/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

run: $(TARGET)
	./$(TARGET) $(ARGS)

clean:
	rm -rf bin
	rm -rf obj

-include $(DEPS)
