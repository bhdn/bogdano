
CFLAGS = -Wall -g

rle_deps = rle.c rle_compress.o rle_decompress.o

all: rle

rle: $(rle_deps)
	$(CC) $(CFLAGS) $(rle_deps) -o $@
	

%.o: %.c %.h
	$(CC) $(CFLAGS) -c $< -o $@

test: rle
	./test.sh
