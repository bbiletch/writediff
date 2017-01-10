CC=gcc

CPPFLAGS=-I.
CFLAGS=-O3 -Wall -g -std=c11

sources=$(wildcard *.c)
objects=$(sources:.c=.o)
deps=$(sources:%.c=.%.d)

all: writediff

clean:
	rm -f writediff *.o .*.d

cleantemp:
	rm -f *~

-include $(deps)

writediff: $(objects)
	$(CC) $(CPPFLAGS) $(CFLAGS) $^ -o $@

%.o .%.d: %.c
	$(CC) -MMD -MF .$*.d -MT '$*.o .$*.d' $(CPPFLAGS) $(CFLAGS) -c $*.c -o $*.o
