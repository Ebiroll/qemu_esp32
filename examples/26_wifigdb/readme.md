Work in progress, allow gdb to connect over wifi on a live system

wifistub.c is the important file, the other files comes from here,
https://github.com/resetnow/esp-gdbstub

The purpose of this project is to understand the gdb-stub and maybe improve it with 
	    "swbreak+;"
		"hwbreak+;"
		"qXfer:threads:read+;"
capabilities.

https://www-zeuthen.desy.de/dv/documentation/unixguide/infohtml/gdb/Thread-List-Format.html

As all other code here, its for learning.