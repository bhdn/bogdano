
CFLAGS = -Wall

all: rle test

rle: rle.c
	$(CC) $(CFLAGS) -o $@ $<

test: rle
	./test.sh
