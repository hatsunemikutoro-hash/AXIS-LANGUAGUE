CC      = gcc
CFLAGS  = -Wall -Wextra -std=c11 -O2
TARGET  = axis
SRCS    = main.c axis.c

$(TARGET): $(SRCS) axis.h
	$(CC) $(CFLAGS) -o $(TARGET) $(SRCS)

clean:
	rm -f $(TARGET)

.PHONY: clean
