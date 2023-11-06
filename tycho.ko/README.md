## tycho.ko

This is a very dumb implementation of fault injection.

Sometimes things need to happen in a very certain order in order reproduce a
race. It can be hard to force this ordering (especially for high-use or very
rare-use paths) in the kernel.

This kernel exposes a /proc/tycho file, which you can write integers to in
order to control things. For example, you might write:

    int proc_tycho;
    EXPORT_SYMBOL_GPL(proc_tycho);
    if (proc_tycho == 1)
        do_rare_thing();

`tycho.ko` looks for an `extern proc_tycho;` exported somewhere from the
kernel, and initializes it to 0 when the module is loaded. Then, it sets the
value to whatever you write to `/proc/tycho`. So you can do things like fail
the next `fork()` or whatever.

I use my name because it's easy for me to remember :)
