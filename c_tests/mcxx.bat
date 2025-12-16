setlocal
set LD_LIBRARY_PATH=c:\outrun\gcc-8.2.0\m68k-elf\lib
path=c:\outrun\gcc-8.2.0\bin;%path%

set inc1=c:\outrun\gcc-8.2.0\lib\gcc\m68k-elf\8.2.0\include
set inc2=c:\outrun\gcc-8.2.0\m68k-elf\include

\outrun\gcc-8.2.0\bin\m68k-elf-gcc -I%inc1% -I%inc2% -mcpu=68000 %1.c -S -o %1.s

\outrun\gcc-8.2.0\bin\m68k-elf-g++ -I%inc1% -I%inc2% -mcpu=68000 %1.c m68start.s newlib68.c -o %1.elf


