TARGETS=matmult matmult_serial matmult_serial_debug
CFLAGS=-g -O3 --std=c99 -Wall -pedantic

all: $(TARGETS)

matmult: matmult.c
	gcc $(CFLAGS) -O3 -fopenmp -o $@ $<

matmult_serial: matmult.c
	gcc $(CFLAGS) -O3 -Wno-unknown-pragmas -o $@ $<

matmult_serial_debug: matmult.c
	gcc $(CFLAGS) -O3 -DDEBUG -Wno-unknown-pragmas -o $@ $<

clean:
	rm -f $(TARGETS)
