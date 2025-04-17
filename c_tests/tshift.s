.text
  .global  main
  .type    main, @function
main:
# shift tests

    clr.l %d0
    clr.l %d1
    clr.l %d2
    lea printf, %a3
    lea thing1, %a4

# asr.b register
    lea asr_reg_buf, %a0
    move.l %a0, -(%a7)
    jsr (%a3)
    adda #4, %a7

    move.w #0, %ccr
    moveq #0x53, %d0
    asr.b #2, %d0
    move %sr, %d1
    move.l %d1, -(%a7)
    move.l %d0, -(%a7)
    lea buffer, %a0
    move.l %a0, -(%a7)
    jsr (%a3)
    adda #12, %a7

    move #0, %ccr
    move #0xa7, %d0
    asr.b #2, %d0
    move %sr, %d1
    move.l %d1, -(%a7)
    move.l %d0, -(%a7)
    lea buffer, %a0
    move.l %a0, -(%a7)
    jsr (%a3)
    adda #12, %a7

    move.w #0x1f, %ccr
    moveq #0x53, %d0
    asr.b #2, %d0
    move %sr, %d1
    move.l %d1, -(%a7)
    move.l %d0, -(%a7)
    lea buffer, %a0
    move.l %a0, -(%a7)
    jsr (%a3)
    adda #12, %a7

    move #0x1f, %ccr
    move #0xa7, %d0
    asr.b #2, %d0
    move %sr, %d1
    move.l %d1, -(%a7)
    move.l %d0, -(%a7)
    lea buffer, %a0
    move.l %a0, -(%a7)
    jsr (%a3)
    adda #12, %a7

# asl.b register
    lea asl_reg_buf, %a0
    move.l %a0, -(%a7)
    jsr (%a3)
    adda #4, %a7

    move.w #0, %ccr
    moveq #0x53, %d0
    asl.b #2, %d0
    move %sr, %d1
    move.l %d1, -(%a7)
    move.l %d0, -(%a7)
    lea buffer, %a0
    move.l %a0, -(%a7)
    jsr (%a3)
    adda #12, %a7

    move #0, %ccr
    move #0xa7, %d0
    asl.b #2, %d0
    move %sr, %d1
    move.l %d1, -(%a7)
    move.l %d0, -(%a7)
    lea buffer, %a0
    move.l %a0, -(%a7)
    jsr (%a3)
    adda #12, %a7

    move.w #0x1f, %ccr
    moveq #0x53, %d0
    asl.b #2, %d0
    move %sr, %d1
    move.l %d1, -(%a7)
    move.l %d0, -(%a7)
    lea buffer, %a0
    move.l %a0, -(%a7)
    jsr (%a3)
    adda #12, %a7

    move #0x1f, %ccr
    move #0xa7, %d0
    asl.b #2, %d0
    move %sr, %d1
    move.l %d1, -(%a7)
    move.l %d0, -(%a7)
    lea buffer, %a0
    move.l %a0, -(%a7)
    jsr (%a3)
    adda #12, %a7

# lsr.b register
    lea lsr_reg_buf, %a0
    move.l %a0, -(%a7)
    jsr (%a3)
    adda #4, %a7

    move.w #0, %ccr
    moveq #0x53, %d0
    lsr.b #2, %d0
    move %sr, %d1
    move.l %d1, -(%a7)
    move.l %d0, -(%a7)
    lea buffer, %a0
    move.l %a0, -(%a7)
    jsr (%a3)
    adda #12, %a7

    move #0, %ccr
    move #0xa7, %d0
    lsr.b #2, %d0
    move %sr, %d1
    move.l %d1, -(%a7)
    move.l %d0, -(%a7)
    lea buffer, %a0
    move.l %a0, -(%a7)
    jsr (%a3)
    adda #12, %a7

    move.w #0x1f, %ccr
    moveq #0x53, %d0
    lsr.b #2, %d0
    move %sr, %d1
    move.l %d1, -(%a7)
    move.l %d0, -(%a7)
    lea buffer, %a0
    move.l %a0, -(%a7)
    jsr (%a3)
    adda #12, %a7

    move #0x1f, %ccr
    move #0xa7, %d0
    lsr.b #2, %d0
    move %sr, %d1
    move.l %d1, -(%a7)
    move.l %d0, -(%a7)
    lea buffer, %a0
    move.l %a0, -(%a7)
    jsr (%a3)
    adda #12, %a7

# lsl.b
    lea lsl_reg_buf, %a0
    move.l %a0, -(%a7)
    jsr (%a3)
    adda #4, %a7

    move.w #0, %ccr
    moveq #0x53, %d0
    lsl.b #2, %d0
    move %sr, %d1
    move.l %d1, -(%a7)
    move.l %d0, -(%a7)
    lea buffer, %a0
    move.l %a0, -(%a7)
    jsr (%a3)
    adda #12, %a7

    move #0, %ccr
    move #0xa7, %d0
    lsl.b #2, %d0
    move %sr, %d1
    move.l %d1, -(%a7)
    move.l %d0, -(%a7)
    lea buffer, %a0
    move.l %a0, -(%a7)
    jsr (%a3)
    adda #12, %a7

    move.w #0x1f, %ccr
    moveq #0x53, %d0
    lsl.b #2, %d0
    move %sr, %d1
    move.l %d1, -(%a7)
    move.l %d0, -(%a7)
    lea buffer, %a0
    move.l %a0, -(%a7)
    jsr (%a3)
    adda #12, %a7

    move #0x1f, %ccr
    move #0xa7, %d0
    lsl.b #2, %d0
    move %sr, %d1
    move.l %d1, -(%a7)
    move.l %d0, -(%a7)
    lea buffer, %a0
    move.l %a0, -(%a7)
    jsr (%a3)
    adda #12, %a7

# roxr.b register
    lea roxr_reg_buf, %a0
    move.l %a0, -(%a7)
    jsr (%a3)
    adda #4, %a7

    move.w #0, %ccr
    moveq #0x53, %d0
    roxr.b #2, %d0
    move %sr, %d1
    move.l %d1, -(%a7)
    move.l %d0, -(%a7)
    lea buffer, %a0
    move.l %a0, -(%a7)
    jsr (%a3)
    adda #12, %a7

    move #0, %ccr
    move #0xa7, %d0
    roxr.b #2, %d0
    move %sr, %d1
    move.l %d1, -(%a7)
    move.l %d0, -(%a7)
    lea buffer, %a0
    move.l %a0, -(%a7)
    jsr (%a3)
    adda #12, %a7

    move.w #0x1f, %ccr
    moveq #0x53, %d0
    roxr.b #2, %d0
    move %sr, %d1
    move.l %d1, -(%a7)
    move.l %d0, -(%a7)
    lea buffer, %a0
    move.l %a0, -(%a7)
    jsr (%a3)
    adda #12, %a7

    move #0x1f, %ccr
    move #0xa7, %d0
    roxr.b #2, %d0
    move %sr, %d1
    move.l %d1, -(%a7)
    move.l %d0, -(%a7)
    lea buffer, %a0
    move.l %a0, -(%a7)
    jsr (%a3)
    adda #12, %a7

# roxl.b
    lea roxl_reg_buf, %a0
    move.l %a0, -(%a7)
    jsr (%a3)
    adda #4, %a7

    move.w #0, %ccr
    moveq #0x53, %d0
    roxl.b #2, %d0
    move %sr, %d1
    move.l %d1, -(%a7)
    move.l %d0, -(%a7)
    lea buffer, %a0
    move.l %a0, -(%a7)
    jsr (%a3)
    adda #12, %a7

    move #0, %ccr
    move #0xa7, %d0
    roxl.b #2, %d0
    move %sr, %d1
    move.l %d1, -(%a7)
    move.l %d0, -(%a7)
    lea buffer, %a0
    move.l %a0, -(%a7)
    jsr (%a3)
    adda #12, %a7

    move.w #0x1f, %ccr
    moveq #0x53, %d0
    roxl.b #2, %d0
    move %sr, %d1
    move.l %d1, -(%a7)
    move.l %d0, -(%a7)
    lea buffer, %a0
    move.l %a0, -(%a7)
    jsr (%a3)
    adda #12, %a7

    move #0x1f, %ccr
    move #0xa7, %d0
    roxl.b #2, %d0
    move %sr, %d1
    move.l %d1, -(%a7)
    move.l %d0, -(%a7)
    lea buffer, %a0
    move.l %a0, -(%a7)
    jsr (%a3)
    adda #12, %a7

# ror.b register
    lea ror_reg_buf, %a0
    move.l %a0, -(%a7)
    jsr (%a3)
    adda #4, %a7

    move.w #0, %ccr
    moveq #0x53, %d0
    ror.b #2, %d0
    move %sr, %d1
    move.l %d1, -(%a7)
    move.l %d0, -(%a7)
    lea buffer, %a0
    move.l %a0, -(%a7)
    jsr (%a3)
    adda #12, %a7

    move #0, %ccr
    move #0xa7, %d0
    ror.b #2, %d0
    move %sr, %d1
    move.l %d1, -(%a7)
    move.l %d0, -(%a7)
    lea buffer, %a0
    move.l %a0, -(%a7)
    jsr (%a3)
    adda #12, %a7

    move.w #0x1f, %ccr
    moveq #0x53, %d0
    ror.b #2, %d0
    move %sr, %d1
    move.l %d1, -(%a7)
    move.l %d0, -(%a7)
    lea buffer, %a0
    move.l %a0, -(%a7)
    jsr (%a3)
    adda #12, %a7

    move #0x1f, %ccr
    move #0xa7, %d0
    ror.b #2, %d0
    move %sr, %d1
    move.l %d1, -(%a7)
    move.l %d0, -(%a7)
    lea buffer, %a0
    move.l %a0, -(%a7)
    jsr (%a3)
    adda #12, %a7

# rol.b
    lea rol_reg_buf, %a0
    move.l %a0, -(%a7)
    jsr (%a3)
    adda #4, %a7

    move.w #0, %ccr
    moveq #0x53, %d0
    rol.b #2, %d0
    move %sr, %d1
    move.l %d1, -(%a7)
    move.l %d0, -(%a7)
    lea buffer, %a0
    move.l %a0, -(%a7)
    jsr (%a3)
    adda #12, %a7

    move #0, %ccr
    move #0xa7, %d0
    rol.b #2, %d0
    move %sr, %d1
    move.l %d1, -(%a7)
    move.l %d0, -(%a7)
    lea buffer, %a0
    move.l %a0, -(%a7)
    jsr (%a3)
    adda #12, %a7

    move.w #0x1f, %ccr
    moveq #0x53, %d0
    rol.b #2, %d0
    move %sr, %d1
    move.l %d1, -(%a7)
    move.l %d0, -(%a7)
    lea buffer, %a0
    move.l %a0, -(%a7)
    jsr (%a3)
    adda #12, %a7

    move #0x1f, %ccr
    move #0xa7, %d0
    rol.b #2, %d0
    move %sr, %d1
    move.l %d1, -(%a7)
    move.l %d0, -(%a7)
    lea buffer, %a0
    move.l %a0, -(%a7)
    jsr (%a3)
    adda #12, %a7

# asr.w <memory>
    lea asr_mem_buf, %a0
    move.l %a0, -(%a7)
    jsr (%a3)
    adda #4, %a7

    move.w #0, %ccr
    move.w #0xc371, (%a4)
    asr.w (%a4)
    move.w (%a4), %d0
    move %sr, %d1
    move.l %d1, -(%a7)
    move.l %d0, -(%a7)
    lea buffer, %a0
    move.l %a0, -(%a7)
    jsr (%a3)
    adda #12, %a7

    move.w #0, %ccr
    move.w #0x7372, (%a4)
    asr.w (%a4)
    move.w (%a4), %d0
    move %sr, %d1
    move.l %d1, -(%a7)
    move.l %d0, -(%a7)
    lea buffer, %a0
    move.l %a0, -(%a7)
    jsr (%a3)
    adda #12, %a7

    move.w #0x1f, %ccr
    move.w #0xc371, (%a4)
    asr.w (%a4)
    move.w (%a4), %d0
    move %sr, %d1
    move.l %d1, -(%a7)
    move.l %d0, -(%a7)
    lea buffer, %a0
    move.l %a0, -(%a7)
    jsr (%a3)
    adda #12, %a7

    move.w #0x1f, %ccr
    move.w #0x7372, (%a4)
    asr.w (%a4)
    move.w (%a4), %d0
    move %sr, %d1
    move.l %d1, -(%a7)
    move.l %d0, -(%a7)
    lea buffer, %a0
    move.l %a0, -(%a7)
    jsr (%a3)
    adda #12, %a7

# asl.w memory
    lea asl_mem_buf, %a0
    move.l %a0, -(%a7)
    jsr (%a3)
    adda #4, %a7

    move.w #0, %ccr
    move.w #0xc371, (%a4)
    asl.w (%a4)
    move.w (%a4), %d0
    move %sr, %d1
    move.l %d1, -(%a7)
    move.l %d0, -(%a7)
    lea buffer, %a0
    move.l %a0, -(%a7)
    jsr (%a3)
    adda #12, %a7

    move.w #0, %ccr
    move.w #0x7372, (%a4)
    asl.w (%a4)
    move.w (%a4), %d0
    move %sr, %d1
    move.l %d1, -(%a7)
    move.l %d0, -(%a7)
    lea buffer, %a0
    move.l %a0, -(%a7)
    jsr (%a3)
    adda #12, %a7

    move.w #0x1f, %ccr
    move.w #0xc371, (%a4)
    asl.w (%a4)
    move.w (%a4), %d0
    move %sr, %d1
    move.l %d1, -(%a7)
    move.l %d0, -(%a7)
    lea buffer, %a0
    move.l %a0, -(%a7)
    jsr (%a3)
    adda #12, %a7

    move.w #0x1f, %ccr
    move.w #0x7372, (%a4)
    asl.w (%a4)
    move.w (%a4), %d0
    move %sr, %d1
    move.l %d1, -(%a7)
    move.l %d0, -(%a7)
    lea buffer, %a0
    move.l %a0, -(%a7)
    jsr (%a3)
    adda #12, %a7

# lsr.w memory
    lea lsr_mem_buf, %a0
    move.l %a0, -(%a7)
    jsr (%a3)
    adda #4, %a7

    move.w #0, %ccr
    move.w #0xc371, (%a4)
    lsr.w (%a4)
    move.w (%a4), %d0
    move %sr, %d1
    move.l %d1, -(%a7)
    move.l %d0, -(%a7)
    lea buffer, %a0
    move.l %a0, -(%a7)
    jsr (%a3)
    adda #12, %a7

    move.w #0, %ccr
    move.w #0x7372, (%a4)
    lsr.w (%a4)
    move.w (%a4), %d0
    move %sr, %d1
    move.l %d1, -(%a7)
    move.l %d0, -(%a7)
    lea buffer, %a0
    move.l %a0, -(%a7)
    jsr (%a3)
    adda #12, %a7

    move.w #0x1f, %ccr
    move.w #0xc371, (%a4)
    lsr.w (%a4)
    move.w (%a4), %d0
    move %sr, %d1
    move.l %d1, -(%a7)
    move.l %d0, -(%a7)
    lea buffer, %a0
    move.l %a0, -(%a7)
    jsr (%a3)
    adda #12, %a7

    move.w #0x1f, %ccr
    move.w #0x7372, (%a4)
    lsr.w (%a4)
    move.w (%a4), %d0
    move %sr, %d1
    move.l %d1, -(%a7)
    move.l %d0, -(%a7)
    lea buffer, %a0
    move.l %a0, -(%a7)
    jsr (%a3)
    adda #12, %a7

# lsl.w memory
    lea lsl_mem_buf, %a0
    move.l %a0, -(%a7)
    jsr (%a3)
    adda #4, %a7

    move.w #0, %ccr
    move.w #0xc371, (%a4)
    lsl.w (%a4)
    move.w (%a4), %d0
    move %sr, %d1
    move.l %d1, -(%a7)
    move.l %d0, -(%a7)
    lea buffer, %a0
    move.l %a0, -(%a7)
    jsr (%a3)
    adda #12, %a7

    move.w #0, %ccr
    move.w #0x7372, (%a4)
    lsl.w (%a4)
    move.w (%a4), %d0
    move %sr, %d1
    move.l %d1, -(%a7)
    move.l %d0, -(%a7)
    lea buffer, %a0
    move.l %a0, -(%a7)
    jsr (%a3)
    adda #12, %a7

    move.w #0x1f, %ccr
    move.w #0xc371, (%a4)
    lsl.w (%a4)
    move.w (%a4), %d0
    move %sr, %d1
    move.l %d1, -(%a7)
    move.l %d0, -(%a7)
    lea buffer, %a0
    move.l %a0, -(%a7)
    jsr (%a3)
    adda #12, %a7

    move.w #0x1f, %ccr
    move.w #0x7372, (%a4)
    lsl.w (%a4)
    move.w (%a4), %d0
    move %sr, %d1
    move.l %d1, -(%a7)
    move.l %d0, -(%a7)
    lea buffer, %a0
    move.l %a0, -(%a7)
    jsr (%a3)
    adda #12, %a7

# roxr.w memory
    lea roxr_mem_buf, %a0
    move.l %a0, -(%a7)
    jsr (%a3)
    adda #4, %a7

    move.w #0, %ccr
    move.w #0xc371, (%a4)
    roxr.w (%a4)
    move.w (%a4), %d0
    move %sr, %d1
    move.l %d1, -(%a7)
    move.l %d0, -(%a7)
    lea buffer, %a0
    move.l %a0, -(%a7)
    jsr (%a3)
    adda #12, %a7

    move.w #0, %ccr
    move.w #0x7372, (%a4)
    roxr.w (%a4)
    move.w (%a4), %d0
    move %sr, %d1
    move.l %d1, -(%a7)
    move.l %d0, -(%a7)
    lea buffer, %a0
    move.l %a0, -(%a7)
    jsr (%a3)
    adda #12, %a7

    move.w #0x1f, %ccr
    move.w #0xc371, (%a4)
    roxr.w (%a4)
    move.w (%a4), %d0
    move %sr, %d1
    move.l %d1, -(%a7)
    move.l %d0, -(%a7)
    lea buffer, %a0
    move.l %a0, -(%a7)
    jsr (%a3)
    adda #12, %a7

    move.w #0x1f, %ccr
    move.w #0x7372, (%a4)
    roxr.w (%a4)
    move.w (%a4), %d0
    move %sr, %d1
    move.l %d1, -(%a7)
    move.l %d0, -(%a7)
    lea buffer, %a0
    move.l %a0, -(%a7)
    jsr (%a3)
    adda #12, %a7

# roxl.w memory
    lea roxl_mem_buf, %a0
    move.l %a0, -(%a7)
    jsr (%a3)
    adda #4, %a7

    move.w #0, %ccr
    move.w #0xc371, (%a4)
    roxl.w (%a4)
    move.w (%a4), %d0
    move %sr, %d1
    move.l %d1, -(%a7)
    move.l %d0, -(%a7)
    lea buffer, %a0
    move.l %a0, -(%a7)
    jsr (%a3)
    adda #12, %a7

    move.w #0, %ccr
    move.w #0x7372, (%a4)
    roxl.w (%a4)
    move.w (%a4), %d0
    move %sr, %d1
    move.l %d1, -(%a7)
    move.l %d0, -(%a7)
    lea buffer, %a0
    move.l %a0, -(%a7)
    jsr (%a3)
    adda #12, %a7

    move.w #0x1f, %ccr
    move.w #0xc371, (%a4)
    roxl.w (%a4)
    move.w (%a4), %d0
    move %sr, %d1
    move.l %d1, -(%a7)
    move.l %d0, -(%a7)
    lea buffer, %a0
    move.l %a0, -(%a7)
    jsr (%a3)
    adda #12, %a7

    move.w #0x1f, %ccr
    move.w #0x7372, (%a4)
    roxl.w (%a4)
    move.w (%a4), %d0
    move %sr, %d1
    move.l %d1, -(%a7)
    move.l %d0, -(%a7)
    lea buffer, %a0
    move.l %a0, -(%a7)
    jsr (%a3)
    adda #12, %a7

# ror.w memory

    lea ror_mem_buf, %a0
    move.l %a0, -(%a7)
    jsr (%a3)
    adda #4, %a7

    move.w #0, %ccr
    move.w #0xc371, (%a4)
    ror.w (%a4)
    move.w (%a4), %d0
    move %sr, %d1
    move.l %d1, -(%a7)
    move.l %d0, -(%a7)
    lea buffer, %a0
    move.l %a0, -(%a7)
    jsr (%a3)
    adda #12, %a7

    move.w #0, %ccr
    move.w #0x7372, (%a4)
    ror.w (%a4)
    move.w (%a4), %d0
    move %sr, %d1
    move.l %d1, -(%a7)
    move.l %d0, -(%a7)
    lea buffer, %a0
    move.l %a0, -(%a7)
    jsr (%a3)
    adda #12, %a7

    move.w #0x1f, %ccr
    move.w #0xc371, (%a4)
    ror.w (%a4)
    move.w (%a4), %d0
    move %sr, %d1
    move.l %d1, -(%a7)
    move.l %d0, -(%a7)
    lea buffer, %a0
    move.l %a0, -(%a7)
    jsr (%a3)
    adda #12, %a7

    move.w #0x1f, %ccr
    move.w #0x7372, (%a4)
    ror.w (%a4)
    move.w (%a4), %d0
    move %sr, %d1
    move.l %d1, -(%a7)
    move.l %d0, -(%a7)
    lea buffer, %a0
    move.l %a0, -(%a7)
    jsr (%a3)
    adda #12, %a7

# rol.w memory
    lea rol_mem_buf, %a0
    move.l %a0, -(%a7)
    jsr (%a3)
    adda #4, %a7

    move.w #0, %ccr
    move.w #0xc371, (%a4)
    rol.w (%a4)
    move.w (%a4), %d0
    move %sr, %d1
    move.l %d1, -(%a7)
    move.l %d0, -(%a7)
    lea buffer, %a0
    move.l %a0, -(%a7)
    jsr (%a3)
    adda #12, %a7

    move.w #0, %ccr
    move.w #0x7372, (%a4)
    rol.w (%a4)
    move.w (%a4), %d0
    move %sr, %d1
    move.l %d1, -(%a7)
    move.l %d0, -(%a7)
    lea buffer, %a0
    move.l %a0, -(%a7)
    jsr (%a3)
    adda #12, %a7

    move.w #0x1f, %ccr
    move.w #0xc371, (%a4)
    rol.w (%a4)
    move.w (%a4), %d0
    move %sr, %d1
    move.l %d1, -(%a7)
    move.l %d0, -(%a7)
    lea buffer, %a0
    move.l %a0, -(%a7)
    jsr (%a3)
    adda #12, %a7

    move.w #0x1f, %ccr
    move.w #0x7372, (%a4)
    rol.w (%a4)
    move.w (%a4), %d0
    move %sr, %d1
    move.l %d1, -(%a7)
    move.l %d0, -(%a7)
    lea buffer, %a0
    move.l %a0, -(%a7)
    jsr (%a3)
    adda #12, %a7

    rts

buffer:
    .string "  result %x flags %x\n"
asr_reg_buf:
    .string "asr.b <register>\n"
asl_reg_buf:
    .string "asl.b <register>\n"
lsr_reg_buf:
    .string "lsr.b <register>\n"
lsl_reg_buf:
    .string "lsl.b <register>\n"
roxr_reg_buf:
    .string "roxr.b <register>\n"
roxl_reg_buf:
    .string "roxl.b <register>\n"
ror_reg_buf:
    .string "ror.b <register>\n"
rol_reg_buf:
    .string "rol.b <register>\n"
asr_mem_buf:
    .string "asr.b <memery>\n"
asl_mem_buf:
    .string "asl.b <memory>\n"
lsr_mem_buf:
    .string "lsr.b <memory>\n"
lsl_mem_buf:
    .string "lsl.b <memory>\n"
roxr_mem_buf:
    .string "roxr.b <memory>\n"
roxl_mem_buf:
    .string "roxl.b <memory>\n"
ror_mem_buf:
    .string "ror.b <memory>\n"
rol_mem_buf:
    .string "rol.b <memory>\n"

thing1:
    .word 0xc371

