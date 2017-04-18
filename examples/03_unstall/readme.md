Example trying to unstall the app cpu without compiling for multicore.

Currently this example does not run on hardware or in qemu.

To improve this example further analysis is required.

One thing to note is that heap_alloc_caps_init() should be called after the cpu1 has been booted by the rom.. This is not currently done.

