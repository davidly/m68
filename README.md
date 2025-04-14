# m68
m68 runs 68000 elf and Motorola hex binaries on Windows and Linux.

m68 loads binaries into memory then emulates the Motorola 68000 to execute them. 

C and C++ programs in the c_tests folder can be built with a 68000 cross compiler on your platform 
of choice. I've run buildgcc-8.2.0.sh on both Windows 11 running on AMD64 and Ubuntu running 
on Arm64. See [Building GCC for 68000](http://www.aaldert.com/outrun/gcc-auto.html#:~:text=I've%20made%20the%2068000%20cross%20compiler%20build,have%20MinGW/MSYS%20installed%2C%20and%20have%20an%20internet) for details.

The build scripts m.bat and m.sh in the c_tests folder can be used to build samples on Windows and Linux.
mm68.bat and mm68.sh build the m68 emulator targeting the 68000 so the emulator can run itself recursively.
mall.bat and mall.sh build all of the sample apps.

The sample apps in the c_tests folder build with newlib and have the required stubs for newlib to call into
Linux-like system calls emulated by m68. m68start.s has the assembly code that initializes newlib
then calls main(). newlib68.c has the small wrappers that are called by newlib then make Linux-style
system calls into the m68 emulator via the Trap insruction. It's required to link these two object
files with yor C apps for them to run.


notes/bugs:
    - C++ global destructors aren't invoked at app exit
    - C++ exceptions don't work yet due to _start not initializing them for newlib
    
