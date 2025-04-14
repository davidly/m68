@echo off
cl /DM68 /W4 /wd4996 /nologo m68.cxx m68000.cxx /I. /EHsc /DNDEBUG /GS- /GL /Ot /Ox /Ob3 /Oi /Qpar /Zi /Fa /FAs /link /OPT:REF user32.lib

