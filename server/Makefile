CC = gcc
CFLAGS = -Wall -Werror -g
LDFLAGS = -lpthread

all: aesdsocket

aesdsocket: aesdsocket.c
	$(CC) $(CFLAGS) -o aesdsocket aesdsocket.c $(LDFLAGS)

clean:
	rm -f aesdsocket

