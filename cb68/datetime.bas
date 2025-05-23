01 REM **************************************************************
02 REM
03 REM 68k-MBC DATETIME demo (CP/M-68K CBASIC (C68) Basic Compiler)
04 REM
05 REM **************************************************************
06 REM
INTEGER EXWRPT, STRPT, EXRDPT, DTTM, T, S, M, H, D, N, Y
08 EXWRPT = 0ffffch : REM Address of the EXECUTE WRITE OPCODE write port
09 STRPT = 0ffffdh  : REM Address of the STORE OPCODE write port
10 EXRDPT = 0ffffch : REM Address of the EXECUTE READ OPCODE read port
11 DTTM = 084h      : REM DATETIME read Opcode
15 POKE STRPT,DTTM  : REM Store the DATETIME read Opcode
20 S = PEEK(EXRDPT) : REM Read a RTC parameter
30 M = PEEK(EXRDPT) : REM Read a RTC parameter
40 H = PEEK(EXRDPT) : REM Read a RTC parameter
50 D = PEEK(EXRDPT) : REM Read a RTC parameter
60 N = PEEK(EXRDPT) : REM Read a RTC parameter
70 Y = PEEK(EXRDPT) : REM Read a RTC parameter
80 T = PEEK(EXRDPT) : REM Read a RTC parameter
83 IF T < 128 THEN 90 : REM Two complement correction
85 T = T - 256
90 PRINT
100 PRINT "THE TIME IS: "; H; ":"; M; ":"; S
120 PRINT "THE DATE IS: "; D; "/"; N; "/"; Y + 2000
135 PRINT "THE TEMPERATURE IS: "; T; "C"
145 PRINT

                                                                                                 