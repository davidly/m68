REM A PROGRAM ID MULTIPLY MATRICES. 465880 EXPECTED RESULT
rem runs 30x faster with integer than the default floating point
rem integer M, N, A, I, J, C, B, K
25 M = 20 : N = 20
50 SUM = 0
60 GOSUB 150
65 REM PRINT "DIMENSIONED"
70 GOSUB 200
75 REM PRINT "A FILLED"
80 GOSUB 280
85 REM PRINT "B FILLED"
90 GOSUB 360
95 REM PRINT "C FILLED"
100 GOSUB 440
105 REM PRINT 1 "MULTIPLIED"
110 GOSUB 540
120 PRINT "SUM = ";SUM
130 PRINT CHR$(7)
140 GOTO 9999
150 REM DIMENSION
160 DIM A(20,20)
170 DIM B(20,20)
180 DIM C(20,20)
190 RETURN
200 REM FILL A
210 FOR I = 1 TO 20
220 FOR J = 1 TO 20
230 A(I,J) = I + J
240 NEXT
250 NEXT
260 RETURN
270 REM *************
280 REM FILL B
290 FOR I = 1 TO 20
300 FOR J = 1 TO 20
310 B(I,J) = INT((I+J)/J)
320 NEXT
330 NEXT
340 RETURN
350 REM **********
360 REM FILL C
370 FOR I = 1 TO 20
380 FOR J = 1 TO 20
390 C(I,J) = 0
400 NEXT
410 NEXT
420 RETURN
430 REM **************
440 REM ************ MULTIPLY
450 FOR I = 1 TO 20
460 FOR J = 1 TO 20
470 FOR K = 1 TO 20
480 C(I,J) = C(I,J) + A(I,K)*B(K,J)
490 NEXT
500 NEXT
510 NEXT
520 RETURN
530 REM **************
540 REM *************** SUMMIT
550 FOR I = 1 TO 20
560 FOR J = 1 TO 20
570 SUM = SUM + C(I,J)
580 NEXT
590 NEXT
600 RETURN
610 REM ************************(
9999 end
