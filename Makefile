CC = gcc
CFLAGS = -Wall -O2 $(shell pkg-config --cflags libimobiledevice-1.0 libusbmuxd-2.0)
LDFLAGS = $(shell pkg-config --libs libimobiledevice-1.0 libusbmuxd-2.0)

TARGET = iproxy2vts
SRC = main.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC) $(LDFLAGS)

clean clear:
	rm -f $(TARGET)

.PHONY: all clean
