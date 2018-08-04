VERSION := 0.0.1

CFLAGS := -D_GNU_SOURCE
LDFLAGS := -ldl


ifeq ($(DEBUG), 1)
	CFLAGS += -O0 -g -gdwarf -ggdb
else
	CFLAGS += -O2
endif


SRCS := $(wildcard *.[cS])

injector: $(SRCS)
	gcc -o $@ $^ $(CFLAGS) $(LDFLAGS)

clean:
	rm injector $(wildcard *.o)
