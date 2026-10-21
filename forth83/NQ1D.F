( Backtracking with a one-dimensional row board, as in NQ1D.PAS. )
8 CONSTANT LIMIT
CREATE BOARD LIMIT ALLOT
VARIABLE SIZE
VARIABLE SOLUTIONS
VARIABLE ROW
VARIABLE COL
VARIABLE R
VARIABLE C
: SAFE? ( row col -- flag )
    COL ! ROW !
    ROW @ BOARD + C@ 255 <> IF 0 EXIT THEN
    ROW @ 1- R ! COL @ 1- C !
    BEGIN R @ 0>= C @ 0>= AND WHILE
        R @ BOARD + C@ C @ = IF 0 EXIT THEN
        -1 R +! -1 C +!
    REPEAT
    ROW @ 1+ R ! COL @ 1- C !
    BEGIN R @ SIZE @ < C @ 0>= AND WHILE
        R @ BOARD + C@ C @ = IF 0 EXIT THEN
        1 R +! -1 C +!
    REPEAT -1
;
: SOLVE ( col -- )
    DUP SIZE @ = IF DROP 1 SOLUTIONS +! EXIT THEN
    SIZE @ 0 DO
        I OVER SAFE? IF
            DUP I BOARD + C!
            DUP 1+ RECURSE
            255 I BOARD + C!
        THEN
    LOOP DROP
;
CREATE EXPECTED 1 , 0 , 0 , 2 , 10 , 4 , 40 , 92 ,
: NQ1D
    BOARD LIMIT 255 FILL
    ." size    solutions" CR
    LIMIT 1+ 1 DO
        I SIZE ! 0 SOLUTIONS ! 0 SOLVE
        I 4 .R SOLUTIONS @ 10 .R CR
        SOLUTIONS @ EXPECTED I 1- 2 * + @ <>
        ABORT" incorrect queen count"
    LOOP
;
NQ1D
BYE
