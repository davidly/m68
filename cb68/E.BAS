integer digits, a, high, x, n, d
100 DIGITS = 200
110 DIM A( 200 )
120 HIGH = DIGITS
130 X = 0
140 N = HIGH - 1
150 IF N <= 0 then GOTO 200
160 A( N ) = 1
170 N = N - 1
180 GOTO 150
200 A( 1 ) = 2
210 A( 0 ) = 0
220 IF HIGH <= 9 then GOTO 400
230 HIGH = HIGH - 1
235 N = HIGH
240 IF N = 0 then GOTO 300
242 D = X / N
250 A( N ) = X - ( D * N )
260 X = ( 10 * A( N - 1 ) ) + D
270 N = N - 1
280 GOTO 240
300 IF X >= 10 then GOTO 330
310 PRINT USING "#"; X;
320 GOTO 220
330 PRINT USING "##"; X;
340 GOTO 220
400 PRINT ""
410 PRINT "done"
420 END


