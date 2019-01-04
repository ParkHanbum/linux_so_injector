include $(srcdir)/Makefile.include

sdir := $(srcdir)/arch/x86_64
odir := $(objdir)/arch/x86_64

ifeq ($(USE_LIBC_DLOPEN), 1)
  COMMON_CFLAGS += -DUSE_LIBC_DLOPEN
else
  COMMON_CFLAGS += -DUSE_LIBDL_DLOPEN
endif

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
