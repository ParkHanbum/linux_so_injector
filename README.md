# Linux so injector

Requirement
1. only support x86 64bit architecture. 
2. this project uses dlopen which is present in libdl to inject shared object, so the target program must be loading libdl.so.


This project aims to inject a Shared object into a process running on Linux.
- Supports multi-threading.
- Sacrificed stability for fastest injection.

Test in Ubuntu 18.04 64bit.

# Example

```
$ make
$ ./sample-target
pid: 72935
addr: 0x601058
pid: 72936
addr: 0x60105c
```

```
# open another terminal to run the injector
$ ./injector sample-library.so 72935
injector: size of code to inject 207
injector: Path of Shared object to be injected : /home/m/git/linux_so_injector/sample-library.so
injector: RIP Register : 400f32
```

then you can see the messsage that shared object has printed.
```
$ ./sample-target
pid: 72935
addr: 0x601058
pid: 72936
addr: 0x60105c
I just got loaded
```

# reference
https://github.com/gaffe23/linux-inject


# etc
run this command when you see the "ptrace attach failed" message.
```
echo 0 | sudo tee /proc/sys/kernel/yama/ptrace_scope
```

## Lincese
GPLv2
