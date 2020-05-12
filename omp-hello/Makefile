TARGETS=hello hello_serial
CFLAGS=-g -O3 --std=c99 -Wall -pedantic

all: $(TARGETS)

hello: hello.c
	gcc $(CFLAGS) -O3 -fopenmp -o $@ $<

hello_serial: hello.c
	gcc $(CFLAGS) -O3 -Wno-unknown-pragmas -o $@ $<

clean:
	rm -f $(TARGETS)
