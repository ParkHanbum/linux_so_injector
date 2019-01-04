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
	@echo $(COMMON_CFLAGS)
	@$(MAKE) -s build-utils
	@$(MAKE) -s build-arch
	@$(MAKE) -s build-main
	@$(MAKE) -s build
	@$(MAKE) -s sample-library
	@$(MAKE) -s sample-target
	@$(MAKE) -s sample-ptrace
	@$(MAKE) -s signal-checker

sample-ptrace: sample-ptrace.c
	$(CC) -o $@ $^ $(UTILS_OBJS) $(LDFLAGS) -ldl

sample-library: sample-library.c
	$(CC) -o $@.so $^ $(CFLAGS) $(LDFLAGS) -shared -fPIC

signal-checker: signal-checker.c
	$(CC) -o $@.so $^ $(CFLAGS) $(LDFLAGS) -shared -fPIC

sample-target: sample-target.cpp
	$(CC) -o $@ $^ $(TARGET_CFLAGS) $(LDFLAGS) -lpthread -ldl -O0

clean:
	@$(MAKE) -C arch/x86_64 clean
	@$(MAKE) -C utils clean
	$(Q)$(RM) $(objdir)/injector
	$(Q)$(RM) $(objdir)/sample-ptrace $(objdir)/signal-checker
	$(Q)$(RM) $(objdir)/sample-target $(objdir)/sample-library
	$(Q)$(RM) $(objdir)/*.o $(objdir)/*.so

.PHONY: all clean PHONY
.DEFAULT_GOAL := all
