@echo off
cl /W4 /DM68 /nologo elfto68k.cxx /I.. /I. /EHsc /D_CRT_SECURE_NO_WARNINGS /DNDEBUG /O2 /Oi /Fa /Qpar /Zi /link /OPT:REF


