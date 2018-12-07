include $(srcdir)/Makefile.include

sdir := $(srcdir)/utils
odir := $(objdir)/utils

UTILS_SRCS := $(wildcard *.c)

all: $(odir)/%.o

$(odir)/%.o: $(UTILS_SRCS)
	$(CC) $(COMMON_CFLAGS) -c $(UTILS_SRCS) 

clean:
	$(RM) $(odir)/*.o
