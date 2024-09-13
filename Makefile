CC = gcc
CFLAGS = -Wall -g
SRCS = shell.c disk.c fat.c directory.c file.c
OBJS = $(SRCS:.c=.o)
TARGET = shell

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

clean:
	rm -f $(OBJS) $(TARGET)
