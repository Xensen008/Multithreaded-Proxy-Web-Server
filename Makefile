CC=gcc
CFLAGS=-g -Wall
LDFLAGS=-lws2_32 -lpthread

all: proxy

proxy: proxy_server_with_cache.c proxy_parse.c proxy_parse.h
	$(CC) $(CFLAGS) -c proxy_parse.c
	$(CC) $(CFLAGS) -c proxy_server_with_cache.c
	$(CC) $(CFLAGS) -o proxy proxy_parse.o proxy_server_with_cache.o $(LDFLAGS)

clean:
	rm -f proxy *.o

tar:
	tar -cvzf ass1.tgz proxy_server_with_cache.c README Makefile proxy_parse.c proxy_parse.h 