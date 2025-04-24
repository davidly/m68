@echo off

..\m68 cp68.68k -i 0: %1.c %1.i
..\m68 c068.68k %1.i %1.1 %1.2 %1.3 -f
del  %1.i
..\m68 c168.68k %1.1 %1.2 %1.s
del %1.1
del %1.2
..\m68 as68.68k -l -u -s 0: %1.s
rem del %1.s

