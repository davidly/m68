.globl _main
.text
_main:

* shift tests

    clr.l d0
    clr.l d1
    lea _printf, a3
    lea thing1, a4

* asr.b register
    lea asr_reg_buf, a0
    move.l a0, -(a7)
    jsr (a3)
    adda #4, a7

    clr.l d0
    clr.l d1
    move.w #0, ccr
    moveq #$53, d0
    asr.b #2, d0
    move sr, d1
    move.l d1, -(a7)
    move.l d0, -(a7)
    lea buffer, a0
    move.l a0, -(a7)
    jsr (a3)
    adda #12, a7

    clr.l d0
    clr.l d1
    move #0, ccr
    move #$a7, d0
    asr.b #2, d0
    move sr, d1
    move.l d1, -(a7)
    move.l d0, -(a7)
    lea buffer, a0
    move.l a0, -(a7)
    jsr (a3)
    adda #12, a7

    clr.l d0
    clr.l d1
    move.w #$1f, ccr
    moveq #$53, d0
    asr.b #2, d0
    move sr, d1
    move.l d1, -(a7)
    move.l d0, -(a7)
    lea buffer, a0
    move.l a0, -(a7)
    jsr (a3)
    adda #12, a7

    clr.l d0
    clr.l d1
    move #$1f, ccr
    move #$a7, d0
    asr.b #2, d0
    move sr, d1
    move.l d1, -(a7)
    move.l d0, -(a7)
    lea buffer, a0
    move.l a0, -(a7)
    jsr (a3)
    adda #12, a7

* asl.b register
    lea asl_reg_buf, a0
    move.l a0, -(a7)
    jsr (a3)
    adda #4, a7

    clr.l d0
    clr.l d1
    move.w #0, ccr
    moveq #$53, d0
    asl.b #2, d0
    move sr, d1
    move.l d1, -(a7)
    move.l d0, -(a7)
    lea buffer, a0
    move.l a0, -(a7)
    jsr (a3)
    adda #12, a7

    clr.l d0
    clr.l d1
    move #0, ccr
    move #$a7, d0
    asl.b #2, d0
    move sr, d1
    move.l d1, -(a7)
    move.l d0, -(a7)
    lea buffer, a0
    move.l a0, -(a7)
    jsr (a3)
    adda #12, a7

    clr.l d0
    clr.l d1
    move.w #$1f, ccr
    moveq #$53, d0
    asl.b #2, d0
    move sr, d1
    move.l d1, -(a7)
    move.l d0, -(a7)
    lea buffer, a0
    move.l a0, -(a7)
    jsr (a3)
    adda #12, a7

    clr.l d0
    clr.l d1
    move #$1f, ccr
    move #$a7, d0
    asl.b #2, d0
    move sr, d1
    move.l d1, -(a7)
    move.l d0, -(a7)
    lea buffer, a0
    move.l a0, -(a7)
    jsr (a3)
    adda #12, a7

* lsr.b register
    lea lsr_reg_buf, a0
    move.l a0, -(a7)
    jsr (a3)
    adda #4, a7

    clr.l d0
    clr.l d1
    move.w #0, ccr
    moveq #$53, d0
    lsr.b #2, d0
    move sr, d1
    move.l d1, -(a7)
    move.l d0, -(a7)
    lea buffer, a0
    move.l a0, -(a7)
    jsr (a3)
    adda #12, a7

    clr.l d0
    clr.l d1
    move #0, ccr
    move #$a7, d0
    lsr.b #2, d0
    move sr, d1
    move.l d1, -(a7)
    move.l d0, -(a7)
    lea buffer, a0
    move.l a0, -(a7)
    jsr (a3)
    adda #12, a7

    clr.l d0
    clr.l d1
    move.w #$1f, ccr
    moveq #$53, d0
    lsr.b #2, d0
    move sr, d1
    move.l d1, -(a7)
    move.l d0, -(a7)
    lea buffer, a0
    move.l a0, -(a7)
    jsr (a3)
    adda #12, a7

    clr.l d0
    clr.l d1
    move #$1f, ccr
    move #$a7, d0
    lsr.b #2, d0
    move sr, d1
    move.l d1, -(a7)
    move.l d0, -(a7)
    lea buffer, a0
    move.l a0, -(a7)
    jsr (a3)
    adda #12, a7

* lsl.b
    lea lsl_reg_buf, a0
    move.l a0, -(a7)
    jsr (a3)
    adda #4, a7

    clr.l d0
    clr.l d1
    move.w #0, ccr
    moveq #$53, d0
    lsl.b #2, d0
    move sr, d1
    move.l d1, -(a7)
    move.l d0, -(a7)
    lea buffer, a0
    move.l a0, -(a7)
    jsr (a3)
    adda #12, a7

    clr.l d0
    clr.l d1
    move #0, ccr
    move #$a7, d0
    lsl.b #2, d0
    move sr, d1
    move.l d1, -(a7)
    move.l d0, -(a7)
    lea buffer, a0
    move.l a0, -(a7)
    jsr (a3)
    adda #12, a7

    clr.l d0
    clr.l d1
    move.w #$1f, ccr
    moveq #$53, d0
    lsl.b #2, d0
    move sr, d1
    move.l d1, -(a7)
    move.l d0, -(a7)
    lea buffer, a0
    move.l a0, -(a7)
    jsr (a3)
    adda #12, a7

    clr.l d0
    clr.l d1
    move #$1f, ccr
    move #$a7, d0
    lsl.b #2, d0
    move sr, d1
    move.l d1, -(a7)
    move.l d0, -(a7)
    lea buffer, a0
    move.l a0, -(a7)
    jsr (a3)
    adda #12, a7

* roxr.b register
    lea roxr_reg_buf, a0
    move.l a0, -(a7)
    jsr (a3)
    adda #4, a7

    clr.l d0
    clr.l d1
    move.w #0, ccr
    moveq #$53, d0
    roxr.b #2, d0
    move sr, d1
    move.l d1, -(a7)
    move.l d0, -(a7)
    lea buffer, a0
    move.l a0, -(a7)
    jsr (a3)
    adda #12, a7

    clr.l d0
    clr.l d1
    move #0, ccr
    move #$a7, d0
    roxr.b #2, d0
    move sr, d1
    move.l d1, -(a7)
    move.l d0, -(a7)
    lea buffer, a0
    move.l a0, -(a7)
    jsr (a3)
    adda #12, a7

    clr.l d0
    clr.l d1
    move.w #$1f, ccr
    moveq #$53, d0
    roxr.b #2, d0
    move sr, d1
    move.l d1, -(a7)
    move.l d0, -(a7)
    lea buffer, a0
    move.l a0, -(a7)
    jsr (a3)
    adda #12, a7

    clr.l d0
    clr.l d1
    move #$1f, ccr
    move #$a7, d0
    roxr.b #2, d0
    move sr, d1
    move.l d1, -(a7)
    move.l d0, -(a7)
    lea buffer, a0
    move.l a0, -(a7)
    jsr (a3)
    adda #12, a7

* roxl.b
    lea roxl_reg_buf, a0
    move.l a0, -(a7)
    jsr (a3)
    adda #4, a7

    clr.l d0
    clr.l d1
    move.w #0, ccr
    moveq #$53, d0
    roxl.b #2, d0
    move sr, d1
    move.l d1, -(a7)
    move.l d0, -(a7)
    lea buffer, a0
    move.l a0, -(a7)
    jsr (a3)
    adda #12, a7

    clr.l d0
    clr.l d1
    move #0, ccr
    move #$a7, d0
    roxl.b #2, d0
    move sr, d1
    move.l d1, -(a7)
    move.l d0, -(a7)
    lea buffer, a0
    move.l a0, -(a7)
    jsr (a3)
    adda #12, a7

    clr.l d0
    clr.l d1
    move.w #$1f, ccr
    moveq #$53, d0
    roxl.b #2, d0
    move sr, d1
    move.l d1, -(a7)
    move.l d0, -(a7)
    lea buffer, a0
    move.l a0, -(a7)
    jsr (a3)
    adda #12, a7

    clr.l d0
    clr.l d1
    move #$1f, ccr
    move #$a7, d0
    roxl.b #2, d0
    move sr, d1
    move.l d1, -(a7)
    move.l d0, -(a7)
    lea buffer, a0
    move.l a0, -(a7)
    jsr (a3)
    adda #12, a7

* ror.b register
    lea ror_reg_buf, a0
    move.l a0, -(a7)
    jsr (a3)
    adda #4, a7

    clr.l d0
    clr.l d1
    move.w #0, ccr
    moveq #$53, d0
    ror.b #2, d0
    move sr, d1
    move.l d1, -(a7)
    move.l d0, -(a7)
    lea buffer, a0
    move.l a0, -(a7)
    jsr (a3)
    adda #12, a7

    clr.l d0
    clr.l d1
    move #0, ccr
    move #$a7, d0
    ror.b #2, d0
    move sr, d1
    move.l d1, -(a7)
    move.l d0, -(a7)
    lea buffer, a0
    move.l a0, -(a7)
    jsr (a3)
    adda #12, a7

    clr.l d0
    clr.l d1
    move.w #$1f, ccr
    moveq #$53, d0
    ror.b #2, d0
    move sr, d1
    move.l d1, -(a7)
    move.l d0, -(a7)
    lea buffer, a0
    move.l a0, -(a7)
    jsr (a3)
    adda #12, a7

    clr.l d0
    clr.l d1
    move #$1f, ccr
    move #$a7, d0
    ror.b #2, d0
    move sr, d1
    move.l d1, -(a7)
    move.l d0, -(a7)
    lea buffer, a0
    move.l a0, -(a7)
    jsr (a3)
    adda #12, a7

* rol.b
    lea rol_reg_buf, a0
    move.l a0, -(a7)
    jsr (a3)
    adda #4, a7

    clr.l d0
    clr.l d1
    move.w #0, ccr
    moveq #$53, d0
    rol.b #2, d0
    move sr, d1
    move.l d1, -(a7)
    move.l d0, -(a7)
    lea buffer, a0
    move.l a0, -(a7)
    jsr (a3)
    adda #12, a7

    clr.l d0
    clr.l d1
    move #0, ccr
    move #$a7, d0
    rol.b #2, d0
    move sr, d1
    move.l d1, -(a7)
    move.l d0, -(a7)
    lea buffer, a0
    move.l a0, -(a7)
    jsr (a3)
    adda #12, a7

    clr.l d0
    clr.l d1
    move.w #$1f, ccr
    moveq #$53, d0
    rol.b #2, d0
    move sr, d1
    move.l d1, -(a7)
    move.l d0, -(a7)
    lea buffer, a0
    move.l a0, -(a7)
    jsr (a3)
    adda #12, a7

    clr.l d0
    clr.l d1
    move #$1f, ccr
    move #$a7, d0
    rol.b #2, d0
    move sr, d1
    move.l d1, -(a7)
    move.l d0, -(a7)
    lea buffer, a0
    move.l a0, -(a7)
    jsr (a3)
    adda #12, a7

* asr.w <memory>
    lea asr_mem_buf, a0
    move.l a0, -(a7)
    jsr (a3)
    adda #4, a7

    clr.l d0
    clr.l d1
    move.w #0, ccr
    move.w #$c371, (a4)
    asr.w (a4)
    move.w (a4), d0
    move sr, d1
    move.l d1, -(a7)
    move.l d0, -(a7)
    lea buffer, a0
    move.l a0, -(a7)
    jsr (a3)
    adda #12, a7

    clr.l d0
    clr.l d1
    move.w #0, ccr
    move.w #$7372, (a4)
    asr.w (a4)
    move.w (a4), d0
    move sr, d1
    move.l d1, -(a7)
    move.l d0, -(a7)
    lea buffer, a0
    move.l a0, -(a7)
    jsr (a3)
    adda #12, a7

    clr.l d0
    clr.l d1
    move.w #$1f, ccr
    move.w #$c371, (a4)
    asr.w (a4)
    move.w (a4), d0
    move sr, d1
    move.l d1, -(a7)
    move.l d0, -(a7)
    lea buffer, a0
    move.l a0, -(a7)
    jsr (a3)
    adda #12, a7

    clr.l d0
    clr.l d1
    move.w #$1f, ccr
    move.w #$7372, (a4)
    asr.w (a4)
    move.w (a4), d0
    move sr, d1
    move.l d1, -(a7)
    move.l d0, -(a7)
    lea buffer, a0
    move.l a0, -(a7)
    jsr (a3)
    adda #12, a7

* asl.w memory
    lea asl_mem_buf, a0
    move.l a0, -(a7)
    jsr (a3)
    adda #4, a7

    clr.l d0
    clr.l d1
    move.w #0, ccr
    move.w #$c371, (a4)
    asl.w (a4)
    move.w (a4), d0
    move sr, d1
    move.l d1, -(a7)
    move.l d0, -(a7)
    lea buffer, a0
    move.l a0, -(a7)
    jsr (a3)
    adda #12, a7

    clr.l d0
    clr.l d1
    move.w #0, ccr
    move.w #$7372, (a4)
    asl.w (a4)
    move.w (a4), d0
    move sr, d1
    move.l d1, -(a7)
    move.l d0, -(a7)
    lea buffer, a0
    move.l a0, -(a7)
    jsr (a3)
    adda #12, a7

    clr.l d0
    clr.l d1
    move.w #$1f, ccr
    move.w #$c371, (a4)
    asl.w (a4)
    move.w (a4), d0
    move sr, d1
    move.l d1, -(a7)
    move.l d0, -(a7)
    lea buffer, a0
    move.l a0, -(a7)
    jsr (a3)
    adda #12, a7

    clr.l d0
    clr.l d1
    move.w #$1f, ccr
    move.w #$7372, (a4)
    asl.w (a4)
    move.w (a4), d0
    move sr, d1
    move.l d1, -(a7)
    move.l d0, -(a7)
    lea buffer, a0
    move.l a0, -(a7)
    jsr (a3)
    adda #12, a7

* lsr.w memory
    lea lsr_mem_buf, a0
    move.l a0, -(a7)
    jsr (a3)
    adda #4, a7

    clr.l d0
    clr.l d1
    move.w #0, ccr
    move.w #$c371, (a4)
    lsr.w (a4)
    move.w (a4), d0
    move sr, d1
    move.l d1, -(a7)
    move.l d0, -(a7)
    lea buffer, a0
    move.l a0, -(a7)
    jsr (a3)
    adda #12, a7

    clr.l d0
    clr.l d1
    move.w #0, ccr
    move.w #$7372, (a4)
    lsr.w (a4)
    move.w (a4), d0
    move sr, d1
    move.l d1, -(a7)
    move.l d0, -(a7)
    lea buffer, a0
    move.l a0, -(a7)
    jsr (a3)
    adda #12, a7

    clr.l d0
    clr.l d1
    move.w #$1f, ccr
    move.w #$c371, (a4)
    lsr.w (a4)
    move.w (a4), d0
    move sr, d1
    move.l d1, -(a7)
    move.l d0, -(a7)
    lea buffer, a0
    move.l a0, -(a7)
    jsr (a3)
    adda #12, a7

    clr.l d0
    clr.l d1
    move.w #$1f, ccr
    move.w #$7372, (a4)
    lsr.w (a4)
    move.w (a4), d0
    move sr, d1
    move.l d1, -(a7)
    move.l d0, -(a7)
    lea buffer, a0
    move.l a0, -(a7)
    jsr (a3)
    adda #12, a7

* lsl.w memory
    lea lsl_mem_buf, a0
    move.l a0, -(a7)
    jsr (a3)
    adda #4, a7

    clr.l d0
    clr.l d1
    move.w #0, ccr
    move.w #$c371, (a4)
    lsl.w (a4)
    move.w (a4), d0
    move sr, d1
    move.l d1, -(a7)
    move.l d0, -(a7)
    lea buffer, a0
    move.l a0, -(a7)
    jsr (a3)
    adda #12, a7

    clr.l d0
    clr.l d1
    move.w #0, ccr
    move.w #$7372, (a4)
    lsl.w (a4)
    move.w (a4), d0
    move sr, d1
    move.l d1, -(a7)
    move.l d0, -(a7)
    lea buffer, a0
    move.l a0, -(a7)
    jsr (a3)
    adda #12, a7

    clr.l d0
    clr.l d1
    move.w #$1f, ccr
    move.w #$c371, (a4)
    lsl.w (a4)
    move.w (a4), d0
    move sr, d1
    move.l d1, -(a7)
    move.l d0, -(a7)
    lea buffer, a0
    move.l a0, -(a7)
    jsr (a3)
    adda #12, a7

    clr.l d0
    clr.l d1
    move.w #$1f, ccr
    move.w #$7372, (a4)
    lsl.w (a4)
    move.w (a4), d0
    move sr, d1
    move.l d1, -(a7)
    move.l d0, -(a7)
    lea buffer, a0
    move.l a0, -(a7)
    jsr (a3)
    adda #12, a7

* roxr.w memory
    lea roxr_mem_buf, a0
    move.l a0, -(a7)
    jsr (a3)
    adda #4, a7

    clr.l d0
    clr.l d1
    move.w #0, ccr
    move.w #$c371, (a4)
    roxr.w (a4)
    move.w (a4), d0
    move sr, d1
    move.l d1, -(a7)
    move.l d0, -(a7)
    lea buffer, a0
    move.l a0, -(a7)
    jsr (a3)
    adda #12, a7

    clr.l d0
    clr.l d1
    move.w #0, ccr
    move.w #$7372, (a4)
    roxr.w (a4)
    move.w (a4), d0
    move sr, d1
    move.l d1, -(a7)
    move.l d0, -(a7)
    lea buffer, a0
    move.l a0, -(a7)
    jsr (a3)
    adda #12, a7

    clr.l d0
    clr.l d1
    move.w #$1f, ccr
    move.w #$c371, (a4)
    roxr.w (a4)
    move.w (a4), d0
    move sr, d1
    move.l d1, -(a7)
    move.l d0, -(a7)
    lea buffer, a0
    move.l a0, -(a7)
    jsr (a3)
    adda #12, a7

    clr.l d0
    clr.l d1
    move.w #$1f, ccr
    move.w #$7372, (a4)
    roxr.w (a4)
    move.w (a4), d0
    move sr, d1
    move.l d1, -(a7)
    move.l d0, -(a7)
    lea buffer, a0
    move.l a0, -(a7)
    jsr (a3)
    adda #12, a7

* roxl.w memory
    lea roxl_mem_buf, a0
    move.l a0, -(a7)
    jsr (a3)
    adda #4, a7

    clr.l d0
    clr.l d1
    move.w #0, ccr
    move.w #$c371, (a4)
    roxl.w (a4)
    move.w (a4), d0
    move sr, d1
    move.l d1, -(a7)
    move.l d0, -(a7)
    lea buffer, a0
    move.l a0, -(a7)
    jsr (a3)
    adda #12, a7

    clr.l d0
    clr.l d1
    move.w #0, ccr
    move.w #$7372, (a4)
    roxl.w (a4)
    move.w (a4), d0
    move sr, d1
    move.l d1, -(a7)
    move.l d0, -(a7)
    lea buffer, a0
    move.l a0, -(a7)
    jsr (a3)
    adda #12, a7

    clr.l d0
    clr.l d1
    move.w #$1f, ccr
    move.w #$c371, (a4)
    roxl.w (a4)
    move.w (a4), d0
    move sr, d1
    move.l d1, -(a7)
    move.l d0, -(a7)
    lea buffer, a0
    move.l a0, -(a7)
    jsr (a3)
    adda #12, a7

    clr.l d0
    clr.l d1
    move.w #$1f, ccr
    move.w #$7372, (a4)
    roxl.w (a4)
    move.w (a4), d0
    move sr, d1
    move.l d1, -(a7)
    move.l d0, -(a7)
    lea buffer, a0
    move.l a0, -(a7)
    jsr (a3)
    adda #12, a7

* ror.w memory

    lea ror_mem_buf, a0
    move.l a0, -(a7)
    jsr (a3)
    adda #4, a7

    clr.l d0
    clr.l d1
    move.w #0, ccr
    move.w #$c371, (a4)
    ror.w (a4)
    move.w (a4), d0
    move sr, d1
    move.l d1, -(a7)
    move.l d0, -(a7)
    lea buffer, a0
    move.l a0, -(a7)
    jsr (a3)
    adda #12, a7

    clr.l d0
    clr.l d1
    move.w #0, ccr
    move.w #$7372, (a4)
    ror.w (a4)
    move.w (a4), d0
    move sr, d1
    move.l d1, -(a7)
    move.l d0, -(a7)
    lea buffer, a0
    move.l a0, -(a7)
    jsr (a3)
    adda #12, a7

    clr.l d0
    clr.l d1
    move.w #$1f, ccr
    move.w #$c371, (a4)
    ror.w (a4)
    move.w (a4), d0
    move sr, d1
    move.l d1, -(a7)
    move.l d0, -(a7)
    lea buffer, a0
    move.l a0, -(a7)
    jsr (a3)
    adda #12, a7

    clr.l d0
    clr.l d1
    move.w #$1f, ccr
    move.w #$7372, (a4)
    ror.w (a4)
    move.w (a4), d0
    move sr, d1
    move.l d1, -(a7)
    move.l d0, -(a7)
    lea buffer, a0
    move.l a0, -(a7)
    jsr (a3)
    adda #12, a7

* rol.w memory
    lea rol_mem_buf, a0
    move.l a0, -(a7)
    jsr (a3)
    adda #4, a7

    clr.l d0
    clr.l d1
    move.w #0, ccr
    move.w #$c371, (a4)
    rol.w (a4)
    move.w (a4), d0
    move sr, d1
    move.l d1, -(a7)
    move.l d0, -(a7)
    lea buffer, a0
    move.l a0, -(a7)
    jsr (a3)
    adda #12, a7

    clr.l d0
    clr.l d1
    move.w #0, ccr
    move.w #$7372, (a4)
    rol.w (a4)
    move.w (a4), d0
    move sr, d1
    move.l d1, -(a7)
    move.l d0, -(a7)
    lea buffer, a0
    move.l a0, -(a7)
    jsr (a3)
    adda #12, a7

    clr.l d0
    clr.l d1
    move.w #$1f, ccr
    move.w #$c371, (a4)
    rol.w (a4)
    move.w (a4), d0
    move sr, d1
    move.l d1, -(a7)
    move.l d0, -(a7)
    lea buffer, a0
    move.l a0, -(a7)
    jsr (a3)
    adda #12, a7

    clr.l d0
    clr.l d1
    move.w #$1f, ccr
    move.w #$7372, (a4)
    rol.w (a4)
    move.w (a4), d0
    move sr, d1
    move.l d1, -(a7)
    move.l d0, -(a7)
    lea buffer, a0
    move.l a0, -(a7)
    jsr (a3)
    adda #12, a7

    rts

buffer: .dc.b '  result %lx flags %lx',10,0
asr_reg_buf:
    .dc.b 'asr.b <register>',10,0
asl_reg_buf:
    .dc.b 'asl.b <register>',10,0
lsr_reg_buf:
    .dc.b 'lsr.b <register>',10,0
lsl_reg_buf:
    .dc.b 'lsl.b <register>',10,0
roxr_reg_buf:
    .dc.b 'roxr.b <register>',10,0
roxl_reg_buf:
    .dc.b 'roxl.b <register>',10,0
ror_reg_buf:
    .dc.b 'ror.b <register>',10,0
rol_reg_buf:
    .dc.b 'rol.b <register>',10,0
asr_mem_buf:
    .dc.b 'asr.b <memery>',10,0
asl_mem_buf:
    .dc.b 'asl.b <memory>',10,0
lsr_mem_buf:
    .dc.b 'lsr.b <memory>',10,0
lsl_mem_buf:
    .dc.b 'lsl.b <memory>',10,0
roxr_mem_buf:
    .dc.b 'roxr.b <memory>',10,0
roxl_mem_buf:
    .dc.b 'roxl.b <memory>',10,0
ror_mem_buf:
    .dc.b 'ror.b <memory>',10,0
rol_mem_buf:
    .dc.b 'rol.b <memory>',10,0

thing1:
    .dc.w $c371

