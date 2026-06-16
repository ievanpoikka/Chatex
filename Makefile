CC=gcc
CFLAGS=-Wall -Wextra -Ilib -pthread

BUILD=build
UTIL=lib/sock_lib.c

SERVER=$(BUILD)/server
CLIENT=$(BUILD)/client

all: $(SERVER) $(CLIENT)

$(BUILD):
	mkdir -p $(BUILD)

$(CLIENT): client/client.c $(UTIL) | $(BUILD)
	$(CC) $(CFLAGS) -o $@ $^

$(SERVER): server/server.c $(UTIL) | $(BUILD)
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -rf $(BUILD)

.PHONY: all clean
