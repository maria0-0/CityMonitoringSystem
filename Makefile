CC=gcc
CFLAGS=-Wall -Wextra -std=c11
TARGET=city_manager

all: $(TARGET)

$(TARGET): city_manager.c
	$(CC) $(CFLAGS) -o $(TARGET) city_manager.c

clean:
	rm -f $(TARGET)
