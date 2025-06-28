@echo off
cl /DM68 /nologo elfto68k.cxx /I.. /I. /EHsc /DDEBUG /O2 /Oi /Fa /Qpar /Zi /link /OPT:REF user32.lib


