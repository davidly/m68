* ===============================================================
* File - CPMGET.S
* ===============================================================
* Created       : 20 Dec 1983 by : Clint H. Woeltjen
* Last Modified : 21 Jan 1984 by : Clint H. Woeltjen
* ===============================================================
*
    .globl    get       * text
    .globl    _lfb      * bss      external
    .globl    _rnb      * text     external
    .text
*
get:
    link        a5,#-2
    move.l      10(a5),a0
    add.l       #48,a0
    clr.w       (a0)            * f.feoln := false;
    move.l      10(a5),_lfb     * _lfb := addr(f);
    move.w      -(a0),ccr       * if f.feof
    bcc         get1
    addq.l      #2,a0
    move.w      #1,(a0)         * f.feoln := true;
    bra         getex
get1:
    jsr         _rnb
    move.l      10(a5),a0
    add.l       #54,a0
    move.w      (a0),ccr        * if f.ftext
    bcc         getex           * then begin
    subq.l      #8,a0           * f.feof :=
    move.w      (a0),d0
    move.l      a0,a1
    adda.l      #14,a1
    move.b      (a1),d1
    cmpi.b      #26,d1
    seq         d2              * (f.fbuffer[0] = chr($1a)) or
    or.w        d2,d0
    andi.w      #1,d0
    move.w      d0,(a0)+        * f.feof
    cmpi.b      #13,d1          * is_eoln := f.fbuffer[0] = chr(13)
    seq         d2
    andi.w      #1,d2
    move.w      d2,-2(a5)
    or.w        d2,d0
    move.w      d0,(a0)         * f.feoln := is_eoln or f.feof
    move.l      10(a5),a2
    add.l       #38,a2
    cmpi.b      #2,(a2)
    seq         d1              * if (f.option = frdwr) and
    andi.w      #1,d1
    and.w       d2,d1           * is_eoln
    move.w      d1,ccr
    bcc         get2
    jsr         _rnb            * then _rnb
get2:
    move.l      10(a5),a0
    add.l       #46,a0
    move.w      (a0)+,d0        * if f.feof
    move.w      (a0),d1
    adda.l      #12,a0
    or.w        d0,d1           * or f.feoln
    move.w      d1,ccr
    bcc         getex
    move.b      #32,(a0)        * then f.fbuffer[0] := ' ';
getex:
    unlk        a5
    move.l      (a7)+,a0
    addq.l      #6,a7
    jmp         (a0)
*
    .end
