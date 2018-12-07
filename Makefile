VERSION := 0.0.1

srcdir = $(CURDIR)
ifeq ($(objdir),)
    ifneq ($(O),)
        objdir = $(O)
    else
        objdir = $(CURDIR)
    endif
endif

include $(srcdir)/Makefile.include
export srcdir objdir

ARCH_OBJ := $(srcdir)/arch/$(ARCH)/arch.o

UTILS_SRCS := $(wildcard $(srcdir)/utils/*.c)
UTILS_OBJS := $(patsubst $(srcdir)/utils/%.c,$(objdir)/utils/%.o,$(UTILS_SRCS))

build-utils:
	@$(MAKE) -C $(srcdir)/utils

build-arch: 
	@$(MAKE) -C $(srcdir)/arch/x86_64

build-main: $(objdir)/main.o
$(objdir)/main.o: $(srcdir)/main.c
	$(CC) -o $@ $(COMMON_CFLAGS) -c $^ 

build: $(objdir)/injector
$(objdir)/injector: $(objdir)/main.o 
	$(CC) $(COMMON_CFLAGS) $(COMMON_LDFLAGS) -o $@ $(ARCH_OBJ) $(UTILS_OBJS) $^

all:
	$(MAKE) build-utils
	$(MAKE) build-arch
	$(MAKE) build-main
	$(MAKE) build
	$(MAKE) sample-library
	$(MAKE) sample-target

sample-library: sample-library.c
	gcc -o $@.so $^ $(CFLAGS) $(LDFLAGS) -lpthread -shared -fPIC

sample-target: sample-target.cpp
	gcc -o $@ $^ $(TARGET_CFLAGS) $(LDFLAGS) -lpthread -ldl -O0

clean:
	@$(MAKE) -C arch/x86_64 clean
	@$(MAKE) -C utils clean
	$(Q)$(RM) $(objdir)/injector
	$(Q)$(RM) $(objdir)/sample-target $(objdir)/sample-library
	$(Q)$(RM) $(objdir)/*.o $(objdir)/*.so

.PHONY: all clean PHONY
.DEFAULT_GOAL := all
