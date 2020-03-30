SOURCES = $(wildcard *.c)
BINS = $(SOURCES:.c=.out)
FLAGS = -pedantic -Wall -std=c99 -pthread -lrt

all: $(BINS)

%.out: %.c
	gcc $(FLAGS) $^ -o $@

clean:
	rm -f $(BINS)

.PHONY: all clean