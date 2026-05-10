CC      = gcc
CFLAGS  = -Wall -Wextra -std=c99 -Iinclude
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