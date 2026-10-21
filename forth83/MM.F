( Integer matrix multiplication; Pascal MM.PAS dimensions and values. )
20 CONSTANT ROWS
20 CONSTANT INNER
20 CONSTANT COLS
CREATE A ROWS INNER * 2 * ALLOT
CREATE B INNER COLS * 2 * ALLOT
CREATE C ROWS COLS * 2 * ALLOT
2VARIABLE SUMM
VARIABLE ROW
VARIABLE COL
: A-CELL ( row col -- addr ) SWAP INNER * + 2 * A + ;
: B-CELL ( row col -- addr ) SWAP COLS * + 2 * B + ;
: C-CELL ( row col -- addr ) SWAP COLS * + 2 * C + ;
: MM
    ROWS 0 DO INNER 0 DO
        J I + 2 + J I A-CELL !
    LOOP LOOP
    INNER 0 DO COLS 0 DO
        J I + 2 + I 1+ / J I B-CELL !
    LOOP LOOP
    C ROWS COLS * 2 * 0 FILL
    ROWS 0 DO
        I ROW !
        COLS 0 DO
            I COL !
            INNER 0 DO
                ROW @ I A-CELL @ I COL @ B-CELL @ *
                ROW @ COL @ C-CELL +!
            LOOP
        LOOP
    LOOP
    0. SUMM 2!
    ROWS 0 DO COLS 0 DO
        SUMM 2@ J I C-CELL @ S>D D+ SUMM 2!
    LOOP LOOP
    ." summ is: " SUMM 2@ D. CR
    SUMM 2@ 465880. D= NOT ABORT" incorrect matrix sum"
;
MM
BYE
