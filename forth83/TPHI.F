( Fibonacci ratios in fixed point, four digits after the decimal. )
VARIABLE PREV1
VARIABLE PREV2
: .RATIO ( numerator denominator -- )
    10000 SWAP */
    0 <# # # # # 46 HOLD #S #> TYPE
;
: TPHI
    ." should tend towards 1.618033988749..." CR
    ." fixed point, truncated to four decimal places" CR
    1 PREV1 ! 1 PREV2 !
    22 1 DO
        PREV1 @ PREV2 @ + PREV1 @ PREV2 ! PREV1 !
        ."   at " I . ." iterations: "
        PREV1 @ PREV2 @ .RATIO CR
    LOOP
    PREV1 @ 28657 <> PREV2 @ 17711 <> OR
    ABORT" incorrect Fibonacci sequence"
;
TPHI
BYE
