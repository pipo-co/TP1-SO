SOURCES = $(wildcard *.c)
BINS = $(SOURCES:.c=.out)
CPPANS = $(SOURCES:.c=.cppout)
FLAGS = -pedantic -Wall -std=c99 -pthread -lrt

all: $(BINS)

%.out: %.c
	gcc $(FLAGS) $^ -o $@

clean:
	rm -f $(BINS) $(CPPANS) report.tasks

test: $(CPPANS)
	./pvs.sh

%.cppout: %.c
	cppcheck --quiet --enable=all --force --inconclusive  $^ 2> $@


.PHONY: all clean test