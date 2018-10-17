
You can test the stability of feature to inject into a shared object.

$ make
$ ./sample-target
```
pid: 1
addr: 0x101010
pid: 2
addr: 0x202020
```
$ ./injector sample-library.so 1 2 0x101010 1



# etc
run this command when you see the "ptrace attach failed" message.

echo 0 | sudo tee /proc/sys/kernel/yama/ptrace_scope


