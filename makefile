CC = gcc
CFLAGS = -Wall
LDFLAGS = -lrt

SRCS = main.c msgq.c
OBJS = $(SRCS:.c=.o)

all: msgq

msgq: $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

main.o: main.c msgq.h
	$(CC) $(CFLAGS) -c $<

msgq.o: msgq.c msgq.h
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f *.o msgq