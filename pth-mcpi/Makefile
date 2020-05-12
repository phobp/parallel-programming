CFLAGS=-g -O2 -Wall --std=c99
TARGETS=mcpi

all: $(TARGETS)

mcpi: mcpi.c
	gcc $(CFLAGS) -o $@ $< -lpthread

clean:
	rm -f $(TARGETS)

