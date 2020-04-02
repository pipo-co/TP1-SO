CC = gcc
CFLAGS = -pedantic -Wall -std=c99 -pthread -lrt
RM = rm -f

SOURCES = $(wildcard *.c)
BINS = $(SOURCES:.c=.out)

CPPANS = $(SOURCES:.c=.cppout)	

#Test Files 
TF = prueba/*

all: $(BINS)

%.out: %.c
	$(CC) $(CFLAGS) $^ -o $@

clean: clean_test
	$(RM) $(BINS)

clean_test:
	$(RM) $(CPPANS) *.valout report.tasks

test: clean $(CPPANS) 
	./pvs.sh $(TOP)
	valgrind ./master.out $(TF) 2> master.valout | valgrind ./view.out 2> view.valout

%.cppout: %.c
	cppcheck --quiet --enable=all --force --inconclusive  $^ 2> $@


.PHONY: all clean test clean_test