@echo off
setlocal

rem can't use -Ofast because NaN support won't work
g++ -O3 -ggdb -D M68 -D _MSC_VER m68.cxx m68000.cxx -I ../djl -D NDEBUG -o m68g.exe -static


