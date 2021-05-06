CC = gcc
CFLAGS += -std=gnu99
CFLAGS += -Wall
# CFLAGS += -Werror
CFLAGS += -Wshadow
CFLAGS += -Wextra
CFLAGS += -fstack-protector-all
CFLAGS += -g


client: client/main.o
	$(CC) $(CFLAGS) -o client/main client/main.o

server: server/main.o
	$(CC) $(CFLAGS) -o server/main server/main.o


client/main.o: client/main.c
	$(CC) $(CFLAGS) -c -o $@ $<

server/main.o: server/main.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm client/*.o server/*.o
