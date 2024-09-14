CC = gcc
CFLAGS = -Wall -g
LDFLAGS = -lreadline
SRCS = shell.c disk.c fat.c directory.c file.c
OBJS = $(SRCS:.c=.o)
TARGET = shell

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS) $(LDFLAGS)

clean:
	rm -f $(OBJS) $(TARGET)
