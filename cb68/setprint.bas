01 REM ***********************************************************************
02 REM
03 REM 68k-MBC - CP/M-68K CBASIC (C68) Basic Compiler
04 REM
05 REM Set a QR203 thermal printer on Serial 2 to 42 columns mode
06 REM
07 REM ***********************************************************************
08 REM
INTEGER SER2TX, EXWRPT, STRPT, CH

10 ESC = 1bh
11 EXWRPT = 0ffffch : REM Address of the EXECUTE WRITE OPCODE write port
12 STRPT = 0ffffdh  : REM Address of the STORE OPCODE write port
15 SER2TX = 10h     : REM SERIAL 2 TX write Opcode (0x10)

16 PRINT "Set the QR203 thermal printer @ 42 columns"
17 PRINT "Note: Serial port 2 must be set @ 9600 bps"

19 REM ESC ! 0x05 - Set Small font (42 columns) + Emphasized mode
20 CH = ESC : GOSUB 500
30 CH = 21h : GOSUB 500
40 CH = 05h : GOSUB 500

44 REM ESC E 0x01 - Set bold on
45 CH = ESC : GOSUB 500
50 CH = 45h : GOSUB 500
60 CH = 01h : GOSUB 500
70 GOTO 600

490 REM
500 REM * * * * * SERIAL 2 TX
501 REM
505 POKE STRPT,SER2TX : REM Store the SERIAL 2 TX write Opcode
507 POKE EXWRPT,CH : REM Send CH to the Serial 2
520 RETURN

600 END

