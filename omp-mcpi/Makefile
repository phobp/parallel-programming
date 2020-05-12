CFLAGS=-g -O2 -Wall --std=c99
TARGETS=mcpi mcpi_ser

all: $(TARGETS)

mcpi: mcpi.c
	gcc $(CFLAGS) -fopenmp -o $@ $<

mcpi_ser: mcpi.c
	gcc $(CFLAGS) -Wno-unknown-pragmas -o $@ $<

clean:
	rm -f $(TARGETS)

