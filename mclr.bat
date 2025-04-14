@echo off
setlocal
path=c:\program files\microsoft visual studio\2022\community\vc\tools\llvm\x64\bin;%path%

rem can't use -Ofast because Nan support won't work
clang++ -DM68 -DNDEBUG -Wno-psabi -I . -x c++ m68.cxx m68000.cxx -o m68cl.exe -O3 -static -fsigned-char -Wno-format -std=c++14 -Wno-deprecated-declarations -luser32.lib
