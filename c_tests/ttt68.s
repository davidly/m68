# prove you can't win at tic-tac-toe if the opponent is competent in 68000 assembly
# d7: global move count. should end up a multiple of 6493
# d6: current move
# d5: current depth
# d4: alpha
# d3: beta
# d2: value
# a6: the board
# a5: winprocs
# a4: global loop count

.equ piece_x, 1
.equ piece_o, 2
.equ piece_blank, 0

.equ score_win, 6
.equ score_tie, 5
.equ score_lose, 4
.equ score_max, 9
.equ score_min, 2
.equ default_iterations, 1

.section .text
  .global  main
  .type    main, @function
main:
    move.l #default_iterations, %a4
    cmpi.l #1, (%a7, 4)
    beq _arguments_complete
    move.l (%a7, 8), %a0
    move.l (%a0, 4), %d0
    move.l %d0, -(%a7)
    jsr atoi
    adda #4, %a7
    tst.l %d0
    beq _use_default
    move.l %d0, %a4
    bra _arguments_complete

  _use_default:
    move.l #default_iterations, %a4

  _arguments_complete:
    clr.l %d7
    movea.l #board, %a6
    movea.l #winprocs, %a5

  _loop_again:
    moveq #0, %d0
    jsr run_move

    moveq #1, %d0
    jsr run_move

    moveq #4, %d0
    jsr run_move

    subq.l #1, %a4
    cmpa #0, %a4
    bne _loop_again

    move.l %d7, -(%a7)
    pea moves_string
    jsr printf
    adda #8, %a7

    rts

  .type run_move, @function
run_move:
    move.w %d0, -(%a7)
    move.b #piece_x, (%a6, %d0)
    clr.l %d5                   | depth
    move.l %d0, %d6             | current move
    moveq.l #score_max, %d3     | beta
    moveq.l #score_min, %d4     | alpha
    jsr minmax_min

    cmpi #score_tie, %d0
    beq _run_move_ok
    pea failure_string
    jsr printf
    adda #4, %a7

  _run_move_ok:
    move.w (%a7)+, %d0
    clr.b (%a6, %d0)
    rts

  .type minmax_max, @function
minmax_max:
    addq.l #1, %d7

    cmpi.b #4, %d5
    blt _max_no_winner_check

    move.l %d6, %d1
    lsl #2, %d1
    move.l (%a5, %d1), %a0
    jsr (%a0)

    cmpi.b #piece_o, %d0
    bne _max_no_winner_check
    move.q #score_lose, %d0
    rts

  _max_no_winner_check:
    move.w %d6, -(%a7)          | save caller's move
    move.w %d2, -(%a7)          | save caller's value 
    move.w %d4, -(%a7)          | save caller's alpha
    moveq.l #score_min, %d2     | maximizing, so set initial value to minimum
    moveq.l #-1, %d6            | %d6 has the move 0..8
    addq.l #1, %d5              | increment depth

  _max_loop:
    cmpi.b #8, %d6
    beq _max_load_value_return
    addq.l #1, %d6
                                                                                  
    tst.b (%a6, %d6)            | is the board position free?
    bne _max_loop

    move.b #piece_x, (%a6, %d6) | make the move
    jsr minmax_min
    clr.b (%a6, %d6)            | restore the board position to free

    cmpi.b #score_win, %d0      | can't do better than winning
    beq _max_restore_value

    cmp.b %d2, %d0              | compare the score and value
    ble _max_loop               | if not a new high score, try the next move

    cmp.b %d3, %d0              | compare value with beta
    bge _max_restore_value      | beta pruning

    move.l %d0, %d2             | update value with score
    cmp.b %d4, %d2              | compre value with alpha
    ble _max_loop

    move.l %d2, %d4             | update alpha with value
    bra _max_loop

  _max_load_value_return:
    move.l %d2, %d0

  _max_restore_value:
    move.w (%a7)+, %d4          | restore caller's alpha
    move.w (%a7)+, %d2          | restore caller's value
    move.w (%a7)+, %d6          | restore caller's move
    subq.l #1, %d5              | restore depth
    rts

  .type minmax_min, @function
minmax_min:
    addq.l #1, %d7

    cmpi.b #4, %d5
    blt _min_no_winner_check

    move.l %d6, %d1
    lsl #2, %d1
    move.l (%a5, %d1), %a0
    jsr (%a0)

    cmpi.b #piece_x, %d0
    bne _min_not_x
    move.q #score_win, %d0
    rts

  _min_not_x:
    cmpi.b #8, %d5
    bne _min_no_winner_check
    move.q #score_tie, %d0
    rts

  _min_no_winner_check:
    move.w %d6, -(%a7)          | save caller's move
    move.w %d2, -(%a7)          | save caller's value 
    move.w %d3, -(%a7)          | save caller's beta
    moveq.l #score_max, %d2     | minimizing, so set initial value to maximum
    moveq.l #-1, %d6            | %d6 has the move 0..8
    addq.l #1, %d5              | increment depth

  _min_loop:
    cmpi.b #8, %d6
    beq _min_load_value_return
    addq.l #1, %d6
                                                                                  
    tst.b (%a6, %d6)            | is the board position free?
    bne _min_loop

    move.b #piece_o, (%a6, %d6) | make the move
    jsr minmax_max
    clr.b (%a6, %d6)            | restore the board position to free

    cmpi.b #score_lose, %d0     | can't do better than losing
    beq _min_restore_value

    cmp.b %d2, %d0              | compare the score and value
    bge _min_loop               | if not a new low score, try the next move

    cmp.b %d4, %d0              | compare value with alpha
    ble _min_restore_value      | alpha pruning

    move.l %d0, %d2             | update value with score
    cmp.b %d3, %d2              | compre value with beta
    bge _min_loop

    move.l %d2, %d3             | update beta with value
    bra _min_loop

  _min_load_value_return:
    move.l %d2, %d0

  _min_restore_value:
    move.w (%a7)+, %d3          | restore caller's beta
    move.w (%a7)+, %d2          | restore caller's value
    move.w (%a7)+, %d6          | restore caller's move
    subq.l #1, %d5              | restore depth
    rts

  .type proc0, @function
proc0:
    move.b (%a6), %d0
    cmp.b (%a6, 1), %d0
    bne _proc0_next_a
    cmp.b (%a6, 2), %d0
    bne _proc0_next_a
    rts

  _proc0_next_a:
    cmp.b (%a6, 3), %d0
    bne _proc0_next_b
    cmp.b (%a6, 6), %d0
    bne _proc0_next_b
    rts
    
  _proc0_next_b:
    cmp.b (%a6, 4), %d0
    bne _proc0_return_0
    cmp.b (%a6, 8), %d0
    bne _proc0_return_0
    rts

  _proc0_return_0:
    clr.b %d0
    rts

  .type proc1, @function
proc1:
    move.b (%a6, 1), %d0
    cmp.b (%a6, 0), %d0
    bne _proc1_next_a
    cmp.b (%a6, 2), %d0
    bne _proc1_next_a
    rts

  _proc1_next_a:
    cmp.b (%a6, 4), %d0
    bne _proc1_return_0
    cmp.b (%a6, 7), %d0
    bne _proc1_return_0
    rts

  _proc1_return_0:
    clr.b %d0
    rts

  .type proc2, @function
proc2:
    move.b (%a6, 2), %d0
    cmp.b (%a6, 0), %d0
    bne _proc2_next_a
    cmp.b (%a6, 1), %d0
    bne _proc2_next_a
    rts

  _proc2_next_a:
    cmp.b (%a6, 5), %d0
    bne _proc2_next_b
    cmp.b (%a6, 8), %d0
    bne _proc2_next_b
    rts
    
  _proc2_next_b:
    cmp.b (%a6, 4), %d0
    bne _proc2_return_0
    cmp.b (%a6, 6), %d0
    bne _proc2_return_0
    rts

  _proc2_return_0:
    clr.b %d0
    rts

  .type proc3, @function
proc3:
    move.b (%a6, 3), %d0
    cmp.b (%a6, 0), %d0
    bne _proc3_next_a
    cmp.b (%a6, 6), %d0
    bne _proc3_next_a
    rts

  _proc3_next_a:
    cmp.b (%a6, 4), %d0
    bne _proc3_return_0
    cmp.b (%a6, 5), %d0
    bne _proc3_return_0
    rts

  _proc3_return_0:
    clr.b %d0
    rts

  .type proc4, @function
proc4:
    move.b (%a6, 4), %d0
    cmp.b (%a6, 0), %d0
    bne _proc4_next_a
    cmp.b (%a6, 8), %d0
    bne _proc4_next_a
    rts

  _proc4_next_a:
    cmp.b (%a6, 2), %d0
    bne _proc4_next_b
    cmp.b (%a6, 6), %d0
    bne _proc4_next_b
    rts
    
  _proc4_next_b:
    cmp.b (%a6, 1), %d0
    bne _proc4_next_c
    cmp.b (%a6, 7), %d0

    bne _proc4_next_c
    rts
    
  _proc4_next_c:
    cmp.b (%a6, 3), %d0
    bne _proc4_return_0
    cmp.b (%a6, 5), %d0
    bne _proc4_return_0
    rts

  _proc4_return_0:
    clr.b %d0
    rts

  .type proc5, @function
proc5:
    move.b (%a6, 5), %d0
    cmp.b (%a6, 3), %d0
    bne _proc5_next_a
    cmp.b (%a6, 4), %d0
    bne _proc5_next_a
    rts

  _proc5_next_a:
    cmp.b (%a6, 2), %d0
    bne _proc5_return_0
    cmp.b (%a6, 8), %d0
    bne _proc5_return_0
    rts

  _proc5_return_0:
    clr.b %d0
    rts

  .type proc6, @function
proc6:
    move.b (%a6, 6), %d0
    cmp.b (%a6, 2), %d0
    bne _proc6_next_a
    cmp.b (%a6, 4), %d0
    bne _proc6_next_a
    rts

  _proc6_next_a:
    cmp.b (%a6, 0), %d0
    bne _proc6_next_b
    cmp.b (%a6, 3), %d0
    bne _proc6_next_b
    rts
    
  _proc6_next_b:
    cmp.b (%a6, 7), %d0
    bne _proc6_return_0
    cmp.b (%a6, 8), %d0
    bne _proc6_return_0
    rts

  _proc6_return_0:
    clr.b %d0
    rts

  .type proc7, @function
proc7:
    move.b (%a6, 7), %d0
    cmp.b (%a6, 1), %d0
    bne _proc7_next_a
    cmp.b (%a6, 4), %d0
    bne _proc7_next_a
    rts

  _proc7_next_a:
    cmp.b (%a6, 6), %d0
    bne _proc7_return_0
    cmp.b (%a6, 8), %d0
    bne _proc7_return_0
    rts

  _proc7_return_0:
    clr.b %d0
    rts

  .type proc8, @function
proc8:
    move.b (%a6, 8), %d0
    cmp.b (%a6, 0), %d0
    bne _proc8_next_a
    cmp.b (%a6, 4), %d0
    bne _proc8_next_a
    rts

  _proc8_next_a:
    cmp.b (%a6, 2), %d0
    bne _proc8_next_b
    cmp.b (%a6, 5), %d0
    bne _proc8_next_b
    rts
    
  _proc8_next_b:
    cmp.b (%a6, 6), %d0
    bne _proc8_return_0
    cmp.b (%a6, 7), %d0
    bne _proc8_return_0
    rts

  _proc8_return_0:
    clr.b %d0
    rts

.section .data

.align  2
board:
    .zero 9

.section .rodata

moves_string:
    .string "moves: %lu\n"

failure_string:
    .string "result isn't a tie\n"

.align  4
winprocs:
    .long proc0
    .long proc1
    .long proc2
    .long proc3
    .long proc4
    .long proc5
    .long proc6
    .long proc7
    .long proc8

