@echo off
cl /DM68 /nologo m68.cxx m68000.cxx /I. /EHsc /DDEBUG /O2 /Oi /Fa /Qpar /Zi /link /OPT:REF user32.lib


