01 REM ********************************************************************
02 REM
03 REM 68k-MBC GPE led blink demo (CP/M-68K CBASIC (C68) Basic Compiler)
04 REM
05 REM Blink a led attached to PIN 8 (GPA5) of the GPIO 
06 REM connector (J7) until USER key is pressed.
08 REM The GPE option must be installed.
09 REM
10 REM ********************************************************************
11 REM
12 REM Demo HW wiring (see 68k-MBC schematic):
13 REM
14 REM    GPIO
15 REM    (J7)
16 REM  +-----+
17 REM  | 1 2 |
18 REM  | 3 4 |   LED         RESISTOR
19 REM  | 5 6 |                 680
20 REM  | 7 8-+--->|-----------/\/\/--+
21 REM  | 9 10|                       |
22 REM  |11 12|                       |
23 REM  |13 14|                       |
24 REM  |15 16|                       |
25 REM  |17 18|                       |
26 REM  |19 20+-----------------------+ GND
27 REM  +-----+  
28 REM    
29 REM *********************************************************************
30 REM
INTEGER EXWRPT, STRPT, EXRDPT, KEYUSER, IODIRA, GPA, J
31 PRINT "Press USER key to exit"
32 REM
33 REM * * * * SET USED OPCODES FOR I/O
34 REM 
35 EXWRPT = 0ffffch : REM Address of the EXECUTE WRITE OPCODE write port
36 STRPT = 0ffffdh  : REM Address of the STORE OPCODE write port
37 EXRDPT = 0ffffch : REM Address of the EXECUTE READ OPCODE read port
38 KEYUSER = 80h   : REM USER KEY read Opcode (0x80)
39 IODIRA = 5      : REM IODIRA write Opcode (0x05)
40 GPA = 3         : REM GPIOA write Opcode (0x03)
48 REM
50 POKE STRPT,IODIRA : POKE EXWRPT,0 : REM Set all GPAx as output (IODIRA=0x00)
60 PRINT "Now blinking GPA5 (GPIO port pin 8)..."
64 REM
65 REM * * * * * BLINK LOOP
66 REM
70 POKE STRPT,GPA : POKE EXWRPT,20h : REM Set GPA5=1, GPAx=0 (GPIOA=B00100000=0X20)
80 GOSUB 505 : REM Delay sub
90 POKE STRPT,GPA : POKE EXWRPT,0 : REM Clear all pins GPAx (MCP23017)
100 GOSUB 505 : REM Delay sub
130 GOTO 70
490 REM
500 REM * * * * * DELAY SUB
501 REM
505 FOR J=0 TO 150
506 POKE STRPT,KEYUSER : REM Write the USER KEY read Opcode
507 IF PEEK(EXRDPT)=1 THEN GOTO 700 : REM Exit if USER key is pressed
510 NEXT J
520 RETURN
690 REM
691 REM * * * * * PROGRAM END
692 REM
700 POKE STRPT,GPA : POKE EXWRPT,0 : REM Clear all pins GPAx (MCP23017)
720 PRINT "Terminated by USER Key"
