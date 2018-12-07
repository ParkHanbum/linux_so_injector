include $(srcdir)/Makefile.include

sdir := $(srcdir)/arch/x86_64
odir := $(objdir)/arch/x86_64

all: $(odir)/arch.o

$(odir)/arch.o: $(odir)/inject.op $(odir)/inject-contents.op
	$(LD) -r -o $@ $(sdir)/inject.op $(sdir)/inject-contents.op 

$(odir)/inject-contents.op:
	$(CC) $(LIB_CFLAGS) -c $(sdir)/inject-content.S -o $@

$(odir)/inject.op:
	$(CC) $(LIB_CFLAGS) -c $(sdir)/inject.c -o $@

clean:
	$(RM) $(odir)/*.op
	$(RM) $(odir)/*.o
