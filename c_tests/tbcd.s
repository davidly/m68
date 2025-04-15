.text
  .global  main
  .type    main, @function
main:
# abcd tests

    move #0, %ccr
    moveq #3, %d0
    moveq #4, %d1
    abcd %d1, %d0
    move %sr, %d1
    andi #0x1f, %d1
    move.l %d1, -(%a7)
    move.l %d0, -(%a7)
    pea buffer
    jsr printf
    adda #12, %a7

    move #0x1f, %ccr
    moveq #3, %d0
    moveq #4, %d1
    abcd %d1, %d0
    move %sr, %d1
    andi #0x1f, %d1
    move.l %d1, -(%a7)
    move.l %d0, -(%a7)
    pea buffer
    jsr printf
    adda #12, %a7

    move #0, %ccr
    moveq #0x68, %d0
    moveq #0x34, %d1
    abcd %d1, %d0
    move %sr, %d1
    andi #0x1f, %d1
    move.l %d1, -(%a7)
    move.l %d0, -(%a7)
    pea buffer
    jsr printf
    adda #12, %a7

    move #0x1f, %ccr
    moveq #0x68, %d0
    moveq #0x31, %d1
    abcd %d1, %d0
    move %sr, %d1
    andi #0x1f, %d1
    move.l %d1, -(%a7)
    move.l %d0, -(%a7)
    pea buffer
    jsr printf
    adda #12, %a7

    move #0x1f, %ccr
    moveq #0x68, %d0
    moveq #0x21, %d1
    abcd %d1, %d0
    move %sr, %d1
    andi #0x1f, %d1
    move.l %d1, -(%a7)
    move.l %d0, -(%a7)
    pea buffer
    jsr printf
    adda #12, %a7

    move #0x1f, %ccr
    moveq #0x68, %d0
    moveq #0x71, %d1
    abcd %d1, %d0
    move %sr, %d1
    andi #0x1f, %d1
    move.l %d1, -(%a7)
    move.l %d0, -(%a7)
    pea buffer
    jsr printf
    adda #12, %a7

    move #0x1f, %ccr
    moveq #0, %d0
    moveq #0, %d1
    abcd %d1, %d0
    move %sr, %d1
    andi #0x1f, %d1
    move.l %d1, -(%a7)
    move.l %d0, -(%a7)
    pea buffer
    jsr printf
    adda #12, %a7

    move #0, %ccr
    moveq #0, %d0
    moveq #0, %d1
    abcd %d1, %d0
    move %sr, %d1
    andi #0x1f, %d1
    move.l %d1, -(%a7)
    move.l %d0, -(%a7)
    pea buffer
    jsr printf
    adda #12, %a7

    move #0, %ccr
    lea aftervaluea, %a0
    lea aftervalueb, %a1
    abcd -(%a0), -(%a1)
    move %sr, %d1
    andi #0x1f, %d1
    move.l %d1, -(%a7)
    move.b (%a1), %d0
    move.l %d0, -(%a7)
    pea buffer
    jsr printf
    adda #12, %a7

# sbcd tests

    move #0, %ccr
    moveq #3, %d0
    moveq #4, %d1
    sbcd %d1, %d0
    move %sr, %d1
    andi #0x1f, %d1
    move.l %d1, -(%a7)
    move.l %d0, -(%a7)
    pea buffer
    jsr printf
    adda #12, %a7

    move #0x1f, %ccr
    moveq #3, %d0
    moveq #4, %d1
    sbcd %d1, %d0
    move %sr, %d1
    andi #0x1f, %d1
    move.l %d1, -(%a7)
    move.l %d0, -(%a7)
    pea buffer
    jsr printf
    adda #12, %a7

    move #0, %ccr
    moveq #0x68, %d0
    moveq #0x34, %d1
    sbcd %d1, %d0
    move %sr, %d1
    andi #0x1f, %d1
    move.l %d1, -(%a7)
    move.l %d0, -(%a7)
    pea buffer
    jsr printf
    adda #12, %a7

    move #0x1f, %ccr
    moveq #0x68, %d0
    moveq #0x31, %d1
    sbcd %d1, %d0
    move %sr, %d1
    andi #0x1f, %d1
    move.l %d1, -(%a7)
    move.l %d0, -(%a7)
    pea buffer
    jsr printf
    adda #12, %a7

    move #0x1f, %ccr
    moveq #0x68, %d0
    moveq #0x21, %d1
    sbcd %d1, %d0
    move %sr, %d1
    andi #0x1f, %d1
    move.l %d1, -(%a7)
    move.l %d0, -(%a7)
    pea buffer
    jsr printf
    adda #12, %a7

    move #0x1f, %ccr
    moveq #0x68, %d0
    moveq #0x71, %d1
    sbcd %d1, %d0
    move %sr, %d1
    andi #0x1f, %d1
    move.l %d1, -(%a7)
    move.l %d0, -(%a7)
    pea buffer
    jsr printf
    adda #12, %a7

    move #0x1f, %ccr
    moveq #0, %d0
    moveq #0, %d1
    sbcd %d1, %d0
    move %sr, %d1
    andi #0x1f, %d1
    move.l %d1, -(%a7)
    move.l %d0, -(%a7)
    pea buffer
    jsr printf
    adda #12, %a7

    move #0, %ccr
    moveq #0, %d0
    moveq #0, %d1
    sbcd %d1, %d0
    move %sr, %d1
    andi #0x1f, %d1
    move.l %d1, -(%a7)
    move.l %d0, -(%a7)
    pea buffer
    jsr printf
    adda #12, %a7

    move #0, %ccr
    lea aftervaluea, %a0
    lea aftervalueb, %a1
    sbcd -(%a0), -(%a1)
    move %sr, %d1
    andi #0x1f, %d1
    move.l %d1, -(%a7)
    move.b (%a1), %d0
    move.l %d0, -(%a7)
    pea buffer
    jsr printf
    adda #12, %a7

# nbcd
    move #0, %ccr
    moveq #3, %d0
    nbcd %d0
    move %sr, %d1
    andi #0x1f, %d1
    move.l %d1, -(%a7)
    move.l %d0, -(%a7)
    pea buffer
    jsr printf
    adda #12, %a7

    move #0x1f, %ccr
    moveq #3, %d0
    nbcd %d0
    move %sr, %d1
    andi #0x1f, %d1
    move.l %d1, -(%a7)
    move.l %d0, -(%a7)
    pea buffer
    jsr printf
    adda #12, %a7

    move #0, %ccr
    moveq #0x68, %d0
    nbcd %d0
    move %sr, %d1
    andi #0x1f, %d1
    move.l %d1, -(%a7)
    move.l %d0, -(%a7)
    pea buffer
    jsr printf
    adda #12, %a7

    move #0x1f, %ccr
    moveq #0x68, %d0
    nbcd %d0
    move %sr, %d1
    andi #0x1f, %d1
    move.l %d1, -(%a7)
    move.l %d0, -(%a7)
    pea buffer
    jsr printf
    adda #12, %a7

    move #0x1f, %ccr
    moveq #0x68, %d0
    nbcd %d0
    move %sr, %d1
    andi #0x1f, %d1
    move.l %d1, -(%a7)
    move.l %d0, -(%a7)
    pea buffer
    jsr printf
    adda #12, %a7

    move #0x1f, %ccr
    moveq #0x68, %d0
    nbcd %d0
    move %sr, %d1
    andi #0x1f, %d1
    move.l %d1, -(%a7)
    move.l %d0, -(%a7)
    pea buffer
    jsr printf
    adda #12, %a7

    move #0x1f, %ccr
    moveq #0, %d0
    nbcd %d0
    move %sr, %d1
    andi #0x1f, %d1
    move.l %d1, -(%a7)
    move.l %d0, -(%a7)
    pea buffer
    jsr printf
    adda #12, %a7

    move #0, %ccr
    moveq #0, %d0
    nbcd %d0
    move %sr, %d1
    andi #0x1f, %d1
    move.l %d1, -(%a7)
    move.l %d0, -(%a7)
    pea buffer
    jsr printf
    adda #12, %a7

    move #0, %ccr
    lea aftervaluea, %a0
    nbcd (%a0)
    move %sr, %d1
    andi #0x1f, %d1
    move.l %d1, -(%a7)
    move.b (%a0), %d0
    move.l %d0, -(%a7)
    pea buffer
    jsr printf
    adda #12, %a7

    rts

buffer:
    .string "result is %#x flags %#x\n"

valuea:
    .byte 0x10
    .byte 0x34
aftervaluea:
valueb:
    .byte 0x20
    .byte 0x27
aftervalueb:
    .byte 0x10
