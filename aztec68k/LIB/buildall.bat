@echo off
if "%CROOT%"=="" goto END
echo Makes Everything... (press ctrl-C to abort)
pause

call make -f makemac clean
call make -f makemac c68mac.lib
call make -f makemac c68macll.lib

call make -f makefile c68.lib
call make -f makefile m68.lib
call make -f makefile m881.lib


call make -f makefile c68ll.lib
call make -f makefile m68ll.lib
call make -f makefile m881ll.lib

lb68 -t c68mac.lib > ..\doc\c68mac.lst
lb68 -t c68.lib > ..\doc\c68.lst
lb68 -t m68.lib > ..\doc\m68.lst
lb68 -t m881.lib > ..\doc\m881.lst

del lb.out
:END