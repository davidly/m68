# m68
m68 runs 68000 elf and Motorola hex binaries on Windows, macOS, and Linux.

m68 loads binaries into memory then emulates the Motorola 68000 to execute them. 

C and C++ programs in the c_tests folder can be built with a 68000 cross compiler on most platforms. I've run buildgcc-8.2.0.sh on both Windows 11 running on AMD64 and Ubuntu running 
on Arm64 to build the GNU gcc 8.2.0 cross compiler. See [Building GCC for 68000](http://www.aaldert.com/outrun/gcc-auto.html#:~:text=I've%20made%20the%2068000%20cross%20compiler%20build,have%20MinGW/MSYS%20installed%2C%20and%20have%20an%20internet) for details. 

I've been unable to build the gcc 68000 cross compiler on macOS and Raspberry PI 5.

The build scripts m.bat and m.sh in the c_tests folder can be used to build samples on Windows and Linux.
mm68.bat and mm68.sh build the m68 emulator targeting the 68000 so the emulator can run itself recursively.
mall.bat and mall.sh build all of the sample apps.

The sample apps in the c_tests folder build with newlib and have the required stubs for newlib to call into
Linux-like system calls emulated by m68. m68start.s has the assembly code that initializes newlib
then calls main(). newlib68.c has the small wrappers that are called by newlib then make Linux-style
system calls into the m68 emulator via the Trap insruction. It's required to link these two object
files with yor C apps for them to link and run.

usage: M68 <M68 arguments> <elf_executable> <app arguments>
   
       arguments:    -e     just show information about the elf executable; don't actually run it
                     -h:X   # of meg for the heap (brk space). 0..1024 are valid. default is 40
                     -i     if -t is set, also enables instruction tracing
                     -m:X   # of meg for mmap space. 0..1024 are valid. default is 40
                     -p     shows performance information at app exit
                     -t     enable debug tracing to m68.log
                     -v     used with -e shows verbose information (e.g. symbols)

files:

    * m68.cxx:      Loads programs and services Linux-style syscall() requests from apps
    * linuxem.h:    Defines syscall IDs. Same values as for RISC-V 64 and Arm64.
    * m68000.cxx:   Emulates a 68000
    * m68000.hxx:   Header for the 68000
    * djl_con.hxx:  Console / terminal related functionality
    * djl_mmap.hxx: Support for mmap syscalls. Newlib doesn't use it so currently unused.
    * djl_os.hxx:   Abstracts various OS, CPU, and compiler-specific concepts
    * djltrace.hxx: Used for tracing status and instructions
    * m.bat/mmac.sh/m.sh: Builds debug versions of m68 on Windows/macOS/Linux
    * mr.bat/mrmac.sh/mr.sh: Builds release versions of m6 on Windows/macOS/Linux
    * runall.bat/runall.sh: Runs test apps in c_tests to validate m68
    * baseline_test_m68.txt: Expected results of runall.bat/runall.sh test scripts
    * mgr.bat/mclr.bat: Builds m68 on Windows using gcc and clang. clang and gcc versions of m68 are about 14% faster than msft C++.
    * words.txt:    Dictionary file for the AN test app

Test files in the hexapps folder are Motorolla hex files generated by Peter J. Fondse's 68000 IDE + emulator. More information is here: [m68k](https://grvc.us.es/FC_grado/docs/practicas/P5/IntroSimulador.pdf)

c_tests folder (test apps + newlib stubs with Linux syscall support)

    * m68start.s:  _start and syscall() for C/C++ apps built with newlib
    * newlib68.c:  stubs called by newlib() for primitives including exit, open, close, etc. Also, printf/sprintf/fprintf overrides of newlib that support floating point (%f, %lf), 8-byte integers (%lld), and size_t integers (%zd).
    * m.bat/m.sh:  builds C++ apps on Windows and Linux
    * mm68.bat/mm68.sh: builds the m68 emulator for the 68000 to run nested as another test case.
    * mall.bat/mall.sh: builds all test apps on Windows/Linux
    * ma.bat/ma.sh: builds .s 68000 assembly file apps
    * *.c:         various test apps. 
    * tbcd.s:      validates the 3 68000 BCD instructions

notes/bugs:

    * C++ exceptions don't work yet due to _start not initializing them for newlib
    * These insructions are unimplemented: movep, tas, trapv, illegal, chk
    
