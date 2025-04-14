# m68
m68 runs 68000 elf and Motorola hex binaries on Windows and Linux.

m68 loads binaries into memory then emulates the Motorola 68000 to execute them. 

C programs in the c_tests folder can be built with a 68000 cross compiler on your platform 
of choice. I've run buildgcc-8.2.0.sh on both Windows 11 running on AMD64 and Ubuntu running 
on Arm64. See [Building GCC for 68000](http://www.aaldert.com/outrun/gcc-auto.html#:~:text=I've%20made%20the%2068000%20cross%20compiler%20build,have%20MinGW/MSYS%20installed%2C%20and%20have%20an%20internet) for details.

The sample apps
in the c_tests folder build with newlib and have the required stubs for newlib to call into
Linux-like system calls emulated by m68. 
