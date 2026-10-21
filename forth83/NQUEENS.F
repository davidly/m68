( Backtracking with a two-dimensional byte board, as in NQUEENS.PAS. )
8 CONSTANT LIMIT
CREATE BOARD LIMIT LIMIT * ALLOT
VARIABLE SIZE
VARIABLE SOLUTIONS
VARIABLE ROW
VARIABLE COL
VARIABLE R
VARIABLE C
: SQUARE ( row col -- addr ) SWAP LIMIT * + BOARD + ;
: SAFE? ( row col -- flag )
    COL ! ROW !
    0 C !
    BEGIN C @ COL @ < WHILE
        ROW @ C @ SQUARE C@ IF 0 EXIT THEN
        1 C +!
    REPEAT
    ROW @ 1- R ! COL @ 1- C !
    BEGIN R @ 0>= C @ 0>= AND WHILE
        R @ C @ SQUARE C@ IF 0 EXIT THEN
        -1 R +! -1 C +!
    REPEAT
    ROW @ 1+ R ! COL @ 1- C !
    BEGIN R @ SIZE @ < C @ 0>= AND WHILE
        R @ C @ SQUARE C@ IF 0 EXIT THEN
        1 R +! -1 C +!
    REPEAT -1
;
: SOLVE ( col -- )
    DUP SIZE @ = IF DROP 1 SOLUTIONS +! EXIT THEN
    SIZE @ 0 DO
        I OVER SAFE? IF
            1 I 2 PICK SQUARE C!
            DUP 1+ RECURSE
            0 I 2 PICK SQUARE C!
        THEN
    LOOP DROP
;
CREATE EXPECTED 1 , 0 , 0 , 2 , 10 , 4 , 40 , 92 ,
: NQUEENS
    BOARD LIMIT LIMIT * 0 FILL
    ." size    solutions" CR
    LIMIT 1+ 1 DO
        I SIZE ! 0 SOLUTIONS ! 0 SOLVE
        I 4 .R SOLUTIONS @ 10 .R CR
        SOLUTIONS @ EXPECTED I 1- 2 * + @ <>
        ABORT" incorrect queen count"
    LOOP
;
NQUEENS
BYE
