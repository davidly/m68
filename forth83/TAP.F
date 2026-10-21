( Apery's constant using fixed point with eight decimal places. )
( Each reciprocal cube is truncated; total error is less than 0.0001. )
2VARIABLE SOFAR
VARIABLE DIVISOR
VARIABLE SEED
VARIABLE TOTAL
VARIABLE PREV
: D/U ( unsigned-double divisor -- unsigned-double )
    DIVISOR ! 0 DIVISOR @ UM/MOD >R
    DIVISOR @ UM/MOD SWAP DROP R>
;
: .FIXED ( unsigned-double -- )
    <# # # # # # # # # 46 HOLD #S #> TYPE
;
: GCD ( u u -- u )
    BEGIN DUP WHILE
        TUCK 0 SWAP UM/MOD DROP
    REPEAT DROP
;
: RANDOM ( -- u )
    SEED @ 0 4 UM/MOD SWAP DROP 3079 * 73 + DUP SEED ! ABS
;
: .RATIO ( numerator denominator -- )
    DIVISOR ! DIVISOR @ /MOD
    48 + EMIT 46 EMIT
    8 0 DO
        10 UM* DIVISOR @ UM/MOD 48 + EMIT
    LOOP DROP
;
: REPORT? ( iteration -- flag ) PREV @ 10 * = ;
: TAP
    ." tap starting, should tend towards 1.2020569031595942854..." CR
    ." fixed point, eight decimal places" CR
    ." first implementation..." CR
    0. SOFAR 2! 1 PREV !
    10001 1 DO
        100000000. I D/U I D/U I D/U
        SOFAR 2@ D+ SOFAR 2!
        I REPORT? IF
            I PREV ! ."   at " I . ." iterations: "
            SOFAR 2@ .FIXED CR
        THEN
    LOOP
    SOFAR 2@ 120205242. D= NOT
    ABORT" incorrect reciprocal cube sum"
    ." second implementation..." CR
    13 SEED ! 0 TOTAL ! 1 PREV !
    10001 1 DO
        RANDOM RANDOM RANDOM GCD GCD 1 = IF 1 TOTAL +! THEN
        I REPORT? IF
            I PREV ! ."   at " I . ." iterations: "
            I TOTAL @ .RATIO CR
        THEN
    LOOP
    TOTAL @ 8372 <> ABORT" incorrect coprime count"
    ." tap completed with great success" CR
;
TAP
BYE
