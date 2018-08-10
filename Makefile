VERSION := 0.0.1

CFLAGS := -D_GNU_SOURCE
LDFLAGS := -ldl

CFLAGS += -O0 -g -gdwarf -ggdb

SRCS := $(wildcard *.[cS])


all: injector sample-target sample-library

injector: $(SRCS)
	gcc -o $@ $^ $(CFLAGS) $(LDFLAGS)

sample-library: sample-library.c
	gcc -o $@.so $^ $(CFLAGS) $(LDFLAGS) -lpthread -shared -fPIC

sample-target: sample-target.cpp
	gcc -o $@ $^ $(CFLAGS) $(LDFLAGS) -lpthread

clean:
	rm injector sample-library.so sample-target $(wildcard *.o)
