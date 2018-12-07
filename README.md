

You can test the stability of feature to inject into a shared object.

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
