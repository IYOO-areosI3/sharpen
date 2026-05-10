CC      = gcc
CFLAGS  = -std=c99 -Iinclude \
          -Wall -Wextra -Werror \
          -Wpedantic \
          -Wconversion \
          -Wsign-conversion \
          -Wshadow \
          -Wformat=2 \
          -Wundef \
          -Wuninitialized \
          -Winit-self \
          -Wmissing-prototypes \
          -Wstrict-prototypes \
          -Wold-style-definition \
          -Wimplicit-fallthrough \
          -Wno-unused-parameter \
          -O2

TARGET  = SharpenOS
SRCS    = src/main.c src/commands.c src/utils.c
OBJS    = $(SRCS:.c=.o)

.PHONY: all clean run

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

run: $(TARGET)
	./$(TARGET)