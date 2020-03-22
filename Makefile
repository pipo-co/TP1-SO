SOURCES = $(wildcard *.c)
BINS = $(SOURCES:.c=.out)
FLAGS = -pedantic -Wall -std=c99

all: $(BINS)

%.out: %.c
	gcc $(FLAGS) $^ -o $@

clean:
	rm -f $(BINS)

.PHONY: all clean