01 REM ********************************************************************
02 REM
03 REM 68k-MBC SPP activation utility (CP/M-68K CBASIC (C68) Basic Compiler)
04 REM
05 REM Enable the SPP (Standard Parallel Port) Adapter board linked to
06 REM the LPT: logical device.
08 REM The GPE option and the SPP Adapter board must be installed.
09 REM
10 REM ********************************************************************

15 INTEGER EXWROPT, STROPT, EXRDOPT, GETSPP, SETSPP, J
30 PRINT "SPP activation utility: ";

34 REM IOS definitions
35 EXWROPT = 0ffffch : REM Address of the EXECUTE WRITE OPCODE write port
36 STROPT = 0ffffdh  : REM Address of the STORE OPCODE write port
37 EXRDOPT = 0ffffch : REM Address of the EXECUTE READ OPCODE read port
38 GETSPP = 83h      : REM GETSPP read Opcode (0x83)
39 SETSPP = 11h      : REM SETSPP write Opcode (0x11)

50 POKE STROPT,SETSPP : POKE EXWROPT,0    : REM Enable the SPP
55 POKE STROPT,GETSPP : J = PEEK(EXRDOPT) : REM Read the SPP status byte
60 IF J = 0 THEN GOTO 300 : REM SPP not enabled

70 PRINT "SPP enabled on LPT: using the GPIO port."
71 PRINT : PRINT "NOTES:"
73 PRINT "* GPIO port is now reserved exclusively for SPP emulation"
76 PRINT "* The SPP emulation is active until next system reset/reboot"
77 PRINT "* When in use a permanent not ready printer can hang CP/M"
79 PRINT "* To use it set LST: = LPT: with the CP/M command: STAT LST:=LPT:"
80 GOTO 600

300 PRINT "SPP activation failed!"

600 END

