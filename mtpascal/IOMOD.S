* ===============================================================
* File IOMOD.S
* ===============================================================
* Created       : 17 Jan 1984 by : Clint H. Woeltjen
* Last Modified : 21 Jan 1984 by : Clint H. Woeltjen
* ===============================================================
*
    .globl    _chn      * external
    .globl    _nok      * external
    .globl    _spn      * external
    .globl    _crl      * external
    .globl    _sfb      * external
    .globl    _wrs      * external
    .globl    _hlt      * external
    .globl    _bdos     * external
    .globl    output    * external
    .globl    fillchar  * external
    .globl    resultio  * external
    .globl    _lfb      * bss
    .globl    _fcbs     * bss
    .globl    _unlink   * text
    .globl    _parse    * text
    .globl    _open     * text
    .globl    _ini2     * text
    .globl    _creat    * text
    .globl    _close    * text
    .globl    _wnb      * text
    .globl    _rnb      * text
    .globl    chain     * text
*
    .even
    .bss
*
_lfb:
    .ds.b     64
_fcbs:
    .ds.b     1720
*
    .text
*
getanfcb:
    link        a5,#-6
    move.w      #0,-6(a5)         * i := 0
dowhile:
    cmp.w       #10,-6(a5)        * while i < maxfcbs + 1
    bgt         nomore            * do begin
    move.l      #_fcbs,a4         * if not(_fcbs[i].active)
    clr.l       d6
    move.w      -6(a5),d6
    mulu        #172,d6
    adda.l      d6,a4
    move.w      (a4),ccr
    bcs         active            * then begin
    move.w      -6(a5),-4(a5)     * get_an_fcb := i
    move.w      #1,(a4)           * _fcbs[i].active := true
    bra         gfcbex            * exit
active:
    addq.w      #1,-6(a5)         * else i := i + 1
    bra.b       dowhile
nomore:
    move.w      #-1,-6(a5)        * i := -1
    move.l      #output,-(a7)     * writeln('fcb table exhausted!)
    jsr         _sfb
    bsr.b       lbl01
    dc.b        20,'FCB TABLE EXHAUSTED!',0
lbl01:
    move.l      #-1,-(a7)
    jsr         _wrs
    jsr         _crl
    jsr         _hlt
gfcbex:
    move.w      -4(a5),d7
    unlk        a5
    rts
*
freeanfcb:
    move.l      (a7)+,a0         * pop return address
    clr.l       d6
    move.w      (a7)+,d6         * fcbnum
    move.l      #_fcbs,a4
    mulu        #172,d6
    adda.l      d6,a4
    move.w      #0,(a4)
    jmp         (a0)
*
uppercase:
    move.l      (a7)+,a3         * pop return address
    move.w      (a7)+,d7         * pop character
    cmpi.b      #'a',d7
    blt         ucasex
    cmpi.b      #'z',d7
    bgt         ucasex
    andi.w      #223,d7          * make it uppercase
ucasex:
    jmp         (a3)
*
_parse:
    link        a5,#-20
    clr.w       d0               * while (length(s) <> 0) and (s[i] = ' ')
    move.l      8(a5),a4
    move.l      a4,a3
parse02:
    addq.l      #1,a4
    cmp.b       #0,(a3)
    beq         parse01
    cmp.b       #32,(a4)
    bne         parse01
    addq.w      #1,d0
    subq.b      #1,(a3)          * do delete(s,1,1)
    bra         parse02
parse01:
    addq.l      #1,a3
    cmp.l       a3,a4
    beq         parse03
    subq.w      #1,d0
parse09:
    move.b      (a3)+,(a4)+
    dbra        d0,parse09
parse03:
    move.l      8(a5),a4          * if length(s) <> 0
    cmp.b       #0,(a4)
    beq         parsex            * then begin
    move.b      #64,-2(a5)        * disk := '@'
    move.l      #$20202020,-10(a5) * name := '        '
    move.l      #$20202020,-6(a5)
    move.l      #$20202000,-14(a5) * ext := '   '
    addq.l      #2,a4             * if s[2] = ':'
    cmp.b       #':',(a4)
    bne         parse04           * then begin
    move.w      #3,-20(a5)        * i := 3
    clr.w       d7
    move.b      -(a4),d7          * disk := uppercase(s[1])
    move.w      d7,-(a7)
    bsr         uppercase
    move.b      d7,-2(a5)
    bra         parse05
parse04:
    move.w      #1,-20(a5)        * else i := 1
parse05:
    lea         -10(a5),a1
    move.w      -20(a5),-16(a5)   * max := i + 8
    addq.w      #8,-16(a5)
    move.w      #1,-18(a5)        * j := 1
    move.l      8(a5),a4          * while (
    move.l      a4,a0
    clr.l       d0
    move.w      -20(a5),d0
    adda.l      d0,a4
parsewh:
    cmp.b       #'.',(a4)         * (s[i] <> '.') or
    seq         d1
    cmp.b       #':',(a4)         * (s[i] <> ':')) and
    seq         d2
    or.w        d2,d1
    not.w       d1
    move.w      -20(a5),d2
    cmp.w       -16(a5),d2        * (i < max) and
    slt         d3
    and.w       d3,d1
    cmp.b       (a0),d2           * and (i <= length(s)))
    sle         d3
    and.w       d3,d1
    move.w      d1,ccr
    bcc         parse06           * do begin
    move.b      (a4)+,d7
    move.w      d7,-(a7)
    bsr         uppercase         * name[j] := uppercase(s[i])
    move.b      d7,(a1)+
    addq.w      #1,-20(a5)        * i := i + 1
    bra         parsewh
parse06:
    cmp.b       #'.',(a4)+        * if (s[i] = '.') and
    seq         d1
    move.w      -20(a5),d0
    cmp.b       (a0),d0           * (i <= length(s))
    sle         d0
    and.w       d0,d1
    move.w      d1,ccr
    bcc         parse07           * then begin
    addq.w      #1,-20(a5)        * i := i + 1
    lea         -14(a5),a1
    clr.l       d0
    move.w      -18(a5),d0
    subq.w      #1,d0
    adda.l      d0,a1
    move.b      #1,d0
parse08:
    cmp.b       #4,d0             * while (j < 4) and
    slt         d1
    move.w      -20(a5),d2
    cmp.b       (a0),d2           * (i <= length(s))
    sle         d2
    and.w       d1,d2
    move.w      d2,ccr
    bcc         parse07           * do begin
    move.b      (a4)+,d7
    move.w      d7,-(a7)
    bsr         uppercase         * ext[j] := uppercase(s[i])
    move.b      d7,(a1)+
    addq.b      #1,d0
    addq.w      #1,-20(a5)        * i := i + 1
    bra.b       parse08
parse07:
    move.l      12(a5),-(a7)
    move.w      #19,-(a7)
    move.w      #0,-(a7)
    jsr         fillchar          * fillchar(f,sizeof(fcblk)-18,chr(0))
    move.l      12(a5),a4         * f[0] := chr(ord(disk) - ord('@'))
    move.b      -2(a5),d7
    subi.b      #'@',d7
    move.b      d7,(a4)+
    move.l      a4,-(a7)
    lea         -10(a5),a0        * addr(name)
    move.w      #7,d0
parse10:
    move.b      (a0)+,(a4)+
    dbra        d0,parse10
    lea         -14(a5),a0        * addr(ext)
    move.w      #2,d0
parse11:
    move.b      (a0)+,(a4)+
    dbra        d0,parse11
parsex:
    unlk        a5
    move.l      (a7)+,a4
    addq.l      #8,a7
    jmp         (a4)
*
_ini2:
    move.l      #_fcbs,-(a7)
    move.w      #1720,-(a7)
    move.w      #0,-(a7)
    jsr         fillchar        * fillchar(_fcbs,sizeof(_fcbs),chr(0))
    rts
*
opcrfun:
    bsr         getanfcb
    move.w      d7,-6(a5)          * i := get_an_fcb
    move.w      d7,-4(a5)          * _opct := i
    cmp.w       #-1,d7             * if i <> -1
    beq         opcterr            * then begin
    move.l      #_fcbs,d6
    ext.l       d7
    mulu        #172,d7
    add.l       d6,d7
    addq.l      #2,d7
    move.l      d7,-(a7)
    move.l      d7,-(a7)
    move.w      #36,-(a7)
    move.w      #0,-(a7)
    jsr         fillchar           * fillchar(_fcbs[i].fcb,36,chr(0))
    move.l      10(a5),-(a7)
    bsr         _parse             * _parse(_fcbs[i].fcb,f.fname)
    move.l      10(a5),-(a7)
    bsr         _nok               * if not _nok(f.fname)
    not.w       d7
    move.w      d7,ccr
    bcc         opct01             * then begin
    move.w      #-1,-4(a5)         * _opct := -1
    move.w      #255,resultio      * resultio := 255
    move.w      -6(a5),-(a7)
    bsr         freeanfcb          * free_an_fcb(i)
    bra         opctex             * exit
opct01:
    move.l      10(a5),-(a7)
    bsr         _spn               * if _spn(f)
    move.w      d7,ccr
    bcc         opct02             * then begin
    move.w      #0,resultio        * resultio := 0
    move.l      #_fcbs,a4
    move.w      -6(a5),d6
    ext.l       d6
    mulu        #172,d6
    adda.l      d6,a4
    addq.l      #2,a4
    move.b      #255,(a4)           * _fcbs[i].fcb[0] := chr($ff)
    bra         opctex
opct02:
    move.l      #_fcbs,d7
    move.w      -6(a5),d6
    ext.l       d6
    mulu        #172,d6
    add.l       d6,d7
    addq.l      #2,d7
    cmpi.w      #1,8(a5)            * check if opct or create
    beq         opct04              * branch on create
    move.w      #15,-(a7)
    move.l      d7,-(a7)
    jsr         _bdos               * resultio := _bdos(15,addr(_fcbs[i].fcb))
    bra         opct05
opct04:
    move.l      d7,-(a7)            * push addr(fib)
    move.w      #19,-(a7)           * delete function code
    move.l      d7,-(a7)            * addr(fib)
    jsr         _bdos
    move.l      (a7)+,d7            * pop addr(fib)
    move.w      #22,-(a7)           * create function code
    move.l      d7,-(a7)            * push addr(fib)
    jsr         _bdos
opct05:
    move.w      d7,resultio
    cmpi.w      #255,d7             * if resultio = 255
    bne         opct03              * then begin
    move.w      #-1,-4(a5)          * _opct := -1
    move.w      -6(a5),-(a7)
    bsr         freeanfcb           * free_an_fcb(i)
    bra         opctex
opct03:
    move.l      #_fcbs,a4
    move.w      -6(a5),d6
    ext.l       d6
    mulu        #172,d6
    adda.l      d6,a4
    adda.l      #40,a4
    cmpi.w      #1,8(a5)            * check if create or open
    beq         opct06
    move.w      #128,(a4)           * _fcbs[i].bufidx := sizeof(sector)
    adda.l      #130,a4
    clr.w       (a4)                * _fcbs[i].endfile := false
    bra         opctex
opct06:
    clr.w       (a4)                * _fcbs[i].bufidx := 0;
    bra         opctex
opcterr:
    move.w      #255,resultio       * else resultio := 255
opctex:
    move.w      -4(a5),d7
    unlk        a5
    move.l      (a7)+,a4
    addq.l      #6,a7
    jmp         (a4)
*
_open:
    link        a5,#-6
    clr.w       8(a5)
    bra         opcrfun
*
_creat:
    link        a5,#-6
    move.w      #1,8(a5)
    bra         opcrfun
*
_unlink:
    link        a5,#-4
    move.l      8(a5),a4
    adda.l      #58,a4
    cmp.w       #0,(a4)             * if f.sysid = 0
    bne         un01                * then f.sysid := _open(f,2)
    move.l      a4,-(a7)
    move.l      8(a5),-(a7)
    move.w      #2,-(a7)
    bsr         _open
    move.l      (a7)+,a4
    move.w      d7,(a4)
un01:
    cmp.w       #-1,(a4)            * if f.sysid <> -1
    beq         un02                * then begin
    move.l      8(a5),a0
    adda.l      #38,a0
    cmpi.b      #3,(a0)             * if f.option <= frandom
    bgt         un03
    move.w      #19,-(a7)
    move.l      #_fcbs,d7
    move.w      (a4),d6
    ext.l       d6
    mulu        #172,d6
    add.l       d6,d7
    addq.l      #2,d7
    move.l      d7,-(a7)
    jsr         _bdos               * then resultio := _bdos(19,addr(_fcbs[f.sysid].fcb))
    move.w      d7,resultio
un03:
    clr.w       -4(a5)              * _unlink := 0
    move.l      8(a5),a4
    add.l       #58,a4
    move.w      (a4),-(a7)
    bsr         freeanfcb           * free_an_fcb(f.sysid)
un02:
    move.w      -4(a5),d7
    unlk        a5
    move.l      (a7)+,a4
    addq.l      #4,a7
    jmp         (a4)
*
putsector:
    move.l      (a7)+,a0         * pop return address
    clr.l       d6
    move.w      (a7)+,d6         * fcbnum
    move.l      #_fcbs,a4
    mulu        #172,d6
    adda.l      d6,a4
    move.l      a0,-(a7)         * push return address
    move.l      a4,-(a7)         * push addr(_fcbs[i])
    move.l      a4,-(a7)         * push addr(_fcbs[i])
    move.w      #26,-(a7)        * set dma
    add.l       #42,a4
    move.l      a4,-(a7)         * addr(_fcbs[i].buffer)
    jsr         _bdos
    move.l      (a7)+,a4         * pop addr(_fcbs[i])
    move.w      #21,-(a7)        * write sequential
    addq.l      #2,a4
    move.l      a4,-(a7)         * addr(_fcbs[i].fcb)
    jsr         _bdos
    move.w      d7,resultio
    add.l       #42,(a7)
    move.w      #128,-(a7)
    clr.w       -(a7)            * clear sector buffer
    jsr         fillchar
    rts
*
_close:
    link        a5,#-6
    move.w      8(a5),d7        * if (not an_infile) and
    not.w       d7
    move.l      #_fcbs,a0
    move.w      10(a5),d6
    ext.l       d6
    mulu        #172,d6
    adda.l      d6,a0
    addq.l      #2,a0
    move.l      a0,-6(a5)
    cmp.b       #255,(a0)       * (_fcbs[i].fcb[0] <> chr($ff))
    sne         d6
    and.w       d6,d7
    move.w      d7,ccr
    bcc         cls01           * then begin
    adda.l      #38,a0
    cmp.w       #0,(a0)         * if (_fcbs[i].bufidx <> 0)
    beq         cls02           * then begin
    cmp.w       #128,(a0)       * if (_fcbs[i].bufidx <> sizeof(sector))
    beq         cls03
    move.w      (a0),d7
    addq.l      #2,a0
    ext.l       d7
    adda.l      d7,a0
    move.l      a0,-(a7)
    move.w      #128,d0
    sub.w       d7,d0
    move.w      d0,-(a7)
    move.w      #26,-(a7)
    jsr         fillchar        * then fillchar(buffer[bufidx],sizeof(sector)-bufidx,26)
cls03:
    move.w      10(a5),-(a7)
    bsr         putsector       * putsector(i)
cls02:
    move.w      #48,-(a7)       * flush buffers
    move.l      -6(a5),-(a7)
    jsr         _bdos           * resultio := _bdos(48,addr(_fcbs[i].fcb))
    move.w      #16,-(a7)       * close file
    move.l      -6(a5),-(a7)
    jsr         _bdos           * resultio := _bdos(16,addr(_fcbs[i].fcb))
    move.w      d7,resultio
cls01:
    move.w      10(a5),-(a7)
    bsr         freeanfcb       * free_an_fcb(i)
    unlk        a5
    move.l      (a7)+,a4
    addq.l      #4,a7
    jmp         (a4)
*
getsector:
    move.l      (a7)+,a0         * pop return address
    clr.l       d6
    move.w      (a7)+,d6         * fcbnum
    move.l      #_fcbs,a4
    mulu        #172,d6
    adda.l      d6,a4
    move.l      a0,-(a7)         * push return address
    move.l      a4,-(a7)         * push addr(_fcbs[i])
    move.w      #26,-(a7)        * set dma
    add.l       #42,a4
    move.l      a4,-(a7)         * addr(_fcbs[i].buffer)
    jsr         _bdos
    move.l      (a7)+,a4
    move.w      #20,-(a7)        * resulti := _bdos(20,addr(_fcbs[i].fcb))
    addq.l      #2,a4
    move.l      a4,-(a7)
    jsr         _bdos
    move.w      d7,resultio
    move.w      d7,d6
    clr.w       d7
    cmpi.w      #0,d6            * if resultio = 0
    bne         gs01
    addq.w      #1,d7            * then getsector := true
gs01:
    rts
*
getbyte:
    link        a5,#-4
    move.l      #_fcbs,a0
    move.w      12(a5),d6
    ext.l       d6
    mulu        #172,d6
    adda.l      d6,a0
    move.l      a0,-4(a5)
    adda.l      #40,a0          * if bufidx >= sizeof(sector)
    cmp.w       #128,(a0)
    blt         gb01            * then begin
    move.l      a0,-(a7)
    move.w      12(a5),-(a7)
    bsr         getsector
    not.w       d7
    move.l      8(a5),a4
    move.w      d7,(a4)          * endfil := not getsector(i)
    move.l      (a7)+,a0
    clr.w       (a0)
gb01:
    move.l      -4(a5),a1
    add.l       #42,a1
    move.w      (a0),d6
    ext.l       d6
    adda.l      d6,a1
    clr.w       d7
    move.b      (a1),d7           * getbyte := buffer[bufidx]
    addq.w      #1,(a0)           * bufidx := bufidx + 1
    unlk        a5
    move.l      (a7)+,a4
    addq.l      #6,a7
    jmp         (a4)
*
putbyte:
    link        a5,#-4
    move.l      #_fcbs,a0
    move.w      8(a5),d6
    ext.l       d6
    mulu        #172,d6
    adda.l      d6,a0
    move.l      a0,-4(a5)
    adda.l      #40,a0
    cmp.w       #128,(a0)           * if bufidx >= sizeof(sector)
    blt         ps01                * then begin
    move.l      a0,-(a7)
    move.w      8(a5),-(a7)
    bsr         putsector           * putsector(i)
    move.l      (a7)+,a0
    clr.w       (a0)                * bufidx := 0
ps01:
    move.w      (a0),d7
    move.l      -4(a5),a1
    adda.l      #42,a1
    ext.l       d7
    adda.l      d7,a1
    move.b      11(a5),(a1)         * buffer[bufidx] := b
    addq.w      #1,(a0)             * bufidx := bufidx + 1
    unlk        a5
    move.l      (a7)+,a4
    addq.l      #4,a7
    jmp         (a4)
*
_rnb:
    link        a5,#-16
    clr.w       resultio          * resultio := 0
    move.l      _lfb,a0
    adda.l      #38,a0
    cmpi.b      #4,(a0)           * if _lfb^.option = fconio
    bne         rnb01             * then begin
    move.w      #1,-(a7)
    clr.l       -(a7)
    jsr         _bdos             * ch := _bdos(1,0)
    move.b      d7,-10(a5)
    cmp.b       #8,d7             * if ch = chr(8)
    bne         rnb02             * then begin
    move.b      #32,-8(a5)        * c := ' '
    move.w      #2,-(a7)          * i := _bdos(2,addr(c))
    pea         -8(a5)
    jsr         _bdos
    move.b      #8,-8(a5)         * c := chr(8)
    move.w      #2,-(a7)          * i := _bdos(2,addr(c))
    pea         -8(a5)
    jsr         _bdos
    bra         rnb03
rnb02:
    cmp.b       #13,d7            * else if ch = chr(13)
    bne         rnb03             * then begin
    move.b      #10,-8(a5)        * c := chr(10)
    move.w      #2,-(a7)          * i := _bdos(2,addr(c))
    pea         -8(a5)
    jsr         _bdos
rnb03:
    move.l      _lfb,a0
    adda.l      #60,a0
    move.b      -10(a5),(a0)      * _lfb^.fbuffer[0] := ch
    suba.l      #14,a0
    cmp.b       #26,-10(a5)
    seq         (a0)              * _lfb^.feof := (ch = chr(26))
    andi.w      #1,(a0)
    bra         rnbex
rnb01:
    move.l      _lfb,a0
    adda.l      #38,a0
    cmpi.b      #5,(a0)           * if _lfb^.option = ftrmio
    bne         rnb04             * then begin
rnb05:                            * repeat
    move.w      #-1,-6(a5)        * k := $ffff
    move.w      #6,-(a7)
    pea         -6(a5)
    jsr         _bdos             * i := _bdos(6,addr(k))
    cmp.w       #0,d7
    beq         rnb05             * until i <> 0
    move.l      _lfb,a0
    adda.l      #60,a0
    move.b      d7,(a0)           * _lfb^.fbuffer[0] := ch
    bra         rnbex
rnb04:
    move.l      _lfb,a0
    adda.l      #38,a0
    cmpi.b      #7,(a0)           * if _lfb^.option = fauxio
    bne         rnb06             * then begin
    move.w      #3,-(a7)
    pea         -6(a5)
    jsr         _bdos             * ch := _bdos(3,addr(k))
    move.l      _lfb,a0
    adda.l      #60,a0
    move.b      d7,(a0)           * _lfb^.fbuffer[0] := ch
    bra         rnbex             * exit
rnb06:
    move.l      _lfb,a0
    adda.l      #58,a0
    move.w      (a0),-2(a5)       * i := _lfb^.sysid
    suba.l      #12,a0
    move.w      (a0),-12(a5)      * endfile := _lfb^.feof
    move.w      #1,-4(a5)         * j := 1
rnb07:
    move.w      -4(a5),d0
    move.l      _lfb,a0
    adda.l      #40,a0
    cmp.w       (a0),d0           * while (j <= _lfb^.buflen) and
    sle         d7
    move.w      -12(a5),d6
    not.w       d6                * (not endfile)
    and.w       d6,d7
    move.w      d7,ccr
    bcc         rnb08             * do begin
    move.w      -2(a5),-(a7)
    pea         -12(a5)
    bsr         getbyte
    move.l      _lfb,a0
    adda.l      #60,a0
    move.w      -4(a5),d0
    subq.w      #1,d0
    ext.l       d0
    adda.l      d0,a0
    move.b      d7,(a0)           * do fbuffer[j-1] := getbyte(i,endfile)
    addq.w      #1,-4(a5)         * j := j + 1
    bra.b       rnb07
rnb08:
    move.l      _lfb,a0
    adda.l      #46,a0
    move.w      -12(a5),(a0)      * _lfb^.feof := endfile
    subq.l      #2,a0
    move.w      -4(a5),(a0)
    subq.w      #1,(a0)           * _lfb^.iosize := j - 1
rnbex:
    unlk        a5
    rts
*
_wnb:
    link        a5,#-12
    clr.w       resultio          * resultio := 0
    move.l      _lfb,a0
    add.l       #38,a0
    cmpi.b      #4,(a0)           * if _lfb^.option = fconio
    bne         wnb01             * then begin
    move.w      #2,-(a7)
    adda.l      #22,a0
    move.l      a0,-(a7)
    jsr         _bdos             * i := _bdos(2,addr(_lfb^.fbuffer[0]))
    bra         wnbex             * exit
wnb01:
    cmpi.b      #5,(a0)           * if _lfb^.option = ftrmio
    bne         wnb02             * then begin
    move.w      #6,-(a7)
    adda.l      #22,a0
    move.l      a0,-(a7)
    jsr         _bdos             * i := _bdos(6,addr(_lfb^.fbuffer[0]))
    bra         wnbex             * exit
wnb02:
    cmpi.b      #6,(a0)           * if _lfb^.option = flstout
    bne         wnb03             * then begin
    move.w      #5,-(a7)
    adda.l      #22,a0
    move.l      a0,-(a7)
    jsr         _bdos             * i := _bdos(5,addr(_lfb^.fbuffer[0]))
    bra         wnbex             * exit
wnb03:
    cmpi.b      #7,(a0)           * if _lfb^.option = fauxio
    bne         wnb04             * then begin
    move.w      #4,-(a7)
    adda.l      #22,a0
    move.l      a0,-(a7)
    jsr         _bdos             * i := _bdos(4,addr(_lfb^.fbuffer[0]))
    bra         wnbex             * exit
wnb04:
    move.l      _lfb,a0
    adda.l      #58,a0
    move.w      (a0),-2(a5)       * i := _lfb^.sysid
    move.w      #1,d0             * for j := 1 to _lfb^.buflen
    move.l      _lfb,a0
    adda.l      #40,a0
    move.w      (a0),d1
    sub.w       d0,d1
    tst.w       d1
    blt         wnb05
    move.w      d0,-4(a5)
wnb06:
    move.w      d1,-8(a5)
    move.l      _lfb,a0
    adda.l      #60,a0
    move.w      -4(a5),d2
    subq.w      #1,d2
    ext.l       d2
    adda.l      d2,a0
    clr.w       d7
    move.b      (a0),d7
    move.w      d7,-(a7)
    move.w      -2(a5),-(a7)
    bsr         putbyte            * do putbyte(fbuffer[j-1],i)
    addq.w      #1,-4(a5)
    move.w      -8(a5),d1
    dbra        d1,wnb06
wnb05:
    move.l      _lfb,a0
    add.l       #42,a0
    clr.w       (a0)               * _lfb^.bufidx := 0
wnbex:
    unlk        a5
    rts
*
chain:
    addq.l      #6,a7           * throw away return address and size
    move.l      (a7)+,a0
    clr.l       d7
    move.b      (a0),d7         * length of fname
    clr.b       1(a0,d7)        * need trailing null
    move.l      a0,-(a7)
    jsr         _chn            * _chn(addr(f.fname))
*
    .end
