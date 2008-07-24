CFLAGS=-Wall -ansi -g -O2 -lm
SOURCES=main.c moves.c board.c tables.c search.c time.c log.c
OBJECTS=main.o moves.o board.o tables.o search.o time.o log.o

all: player

player: $(OBJECTS)
	$(CC) $(CFLAGS) -o player $(OBJECTS)

submission.c: $(SOURCES)
	./compile.pl $(SOURCES) > submission.c

test1: submission.c
	$(CC) $(CFLAGS) -DTEST1 -o test1 submission.c

clean:
	rm -f random player $(OBJECTS) submission.c

