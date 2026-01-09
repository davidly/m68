/*
 *              Explorer - Version 1.0
 *
 *          A Desk Accessory for the Macintosh
 *
 *  Copyright (C) 1984 by Manx Software Systems, Inc.
 *      May be used, but not sold without permission.
 *
 *  written by Jim Goodnow II, December 7-8, 1984
 */

#asm
main
    dc.w    $2400           ;ctl-enable, need time
    dc.w    5*60            ;update every 5 seconds
    dc.w    $000a           ;detect mouse and key down events
    dc.w    -777            ;menu ID number (must be negative)

    dc.w    open_-main      ;open routine
    dc.w    nop_-main       ;prime routine
    dc.w    control_-main   ;control routine
    dc.w    nop_-main       ;status routine
    dc.w    close_-main     ;close routine

title_
    dc.b    8
    dc.b    "Explorer"
    ds      0               ;for alignment

    public  _Uend_,_Dorg_,_Cend_

save_
    lea     main+(_Uend_-_Dorg_)+(_Cend_-main),a4       ;set up globals
    move.l  a0,Pbp_                                     ;save pb pointer
    move.l  a1,Dp_                                      ;save DCE pointer
    rts

restore_
    move.l  Pbp_,a0
    rts
#endasm

#define _DRIVER
#define SMALL_MEM
#include    <quickdraw.h>
#include    <toolutil.h>
#include    <window.h>
#include    <memory.h>
#include    <osutil.h>
#include    <menu.h>
#include    <control.h>
#undef SMALL_MEM
#include    <event.h>
#include    <textedit.h>
#include    <pb.h>
#include    <desk.h>

#define TRUE    0x100
#define FALSE   0x000

#define NLINES  16                  /* number of lines in window    */
#define MENUID  -777                /* must be negative             */
#define MEMTOP  (*(long *)0x108)    /* top of memory 128 or 512K    */
#define SP      (*(struct storage **)Dp->dCtlStorage)

DCEPtr Dp;
ParmBlkPtr Pbp;

Rect Wind_rect = {100, 150, 275, 360};
Rect Scrl_rect = {-1, 194, 176, 211};
Rect Cont_rect = {0, 0, 176, 194};
Rect Full_rect = {0, 0, 176, 211};
Rect Edit_rect = {-1, 44, 12, 90};

Cursor Ibeam = {
0x3838, 0x3C78, 0x0280, 0x0100, 0x0100, 0x0100, 0x0100, 0x0100,
0x0100, 0x0100, 0x0100, 0x0100, 0x0100, 0x0280, 0x3C78, 0x3838,
0x3838, 0x3C78, 0x0280, 0x0100, 0x0100, 0x0100, 0x0100, 0x0100,
0x0100, 0x0100, 0x0100, 0x0100, 0x0100, 0x0280, 0x3C78, 0x3838,
0x0008, 0x0008 };

struct storage {
    MenuHandle      menu;
    ControlHandle   vscrl;
    char *          where;
    short           autoupdate;
    short           size;
    short           incr;
    TEHandle        hte;
};

open()
{
    register WindowPtr wp;
    register struct DCE *dp;
    register struct storage *sp;
    extern char title[];
    struct windowpeek {
        GrafPort    port;
        int         windowKind;
    };

    save();
    dp = Dp;
    if (dp->dCtlWindow == 0) {
        HLock(dp->dCtlStorage = NewHandle((long)sizeof(struct storage)));
        sp = SP;
        dp->dCtlWindow =
        wp = NewWindow(0L, &Wind_rect, title, TRUE, noGrowDocProc, -1L, TRUE, 0L);
        ((struct windowpeek *)wp)->windowKind = dp->dCtlRefNum;
        sp->vscrl = NewControl(wp, &Scrl_rect, "", FALSE, 0, 0, 0x7fff, scrollBarProc, 0L);
        sp->hte = 0;
        sp->where = 0;
        sp->size = 0;
        sp->autoupdate = 0;
        sp->incr = MEMTOP / 0x8000L;
        sp->menu = NewMenu(MENUID, "\PExplorer");
        AppendMenu(sp->menu,
            "\PAuto-Refresh;Hand-Refresh;(-;Hexadecimal!\x12;Ascii");
        HUnlock(dp->dCtlStorage);
    }
    restore();
    return(0);
}

close()
{
    register struct DCE *dp;

    save();
    dp = Dp;
    TEDispose(SP->hte);
    DisposeWindow(dp->dCtlWindow);
    dp->dCtlWindow = 0;
    DeleteMenu(MENUID);
    DrawMenuBar();
    DisposHandle(SP->menu);
    DisposHandle(dp->dCtlStorage);
    restore();
    return(0);
}

nop()
{
    return(0);
}

control()
{
    register struct storage *sp;
    Point pt;
    register int item;

    save();
    HLock(Dp->dCtlStorage);
    sp = SP;
    SetPort(Dp->dCtlWindow);
    switch(Pbp->u.cp.csCode) {
    case accEvent:
        doevent(sp, *(EventRecord **)&Pbp->u.cp.csParam);
        break;
    case accRun:
        if (sp->autoupdate)
            draw_wind();
        break;
    case accCursor:
        if (sp->hte)
            TEIdle(sp->hte);
        GetMouse(&pt);
        if (PtInRect(pass(pt), &Edit_rect) & TRUE)
            SetCursor(&Ibeam);
        else
            InitCursor();
        break;
    case accMenu:
        switch(item = ((int *)&Pbp->u.cp.csParam)[1]) {
        case 1:                             /* auto refresh     */
            sp->autoupdate ^= TRUE;
            CheckItem(sp->menu, 1, sp->autoupdate);
            break;
        case 2:                             /* manual refresh   */
            draw_wind();
            break;
        case 4:
        case 5:
            sp->size = item==4?8:16;
            CheckItem(sp->menu, 4, item==4?TRUE:FALSE);
            CheckItem(sp->menu, 5, item==5?TRUE:FALSE);
            EraseRect(&Cont_rect);
            draw_wind();
            break;
        }
        break;
    case accUndo:
        break;
    case accCut:
        TECut(sp->hte);
        break;
    case accCopy:
        TECopy(sp->hte);
        break;
    case accPaste:
        TEPaste(sp->hte);
        break;
    case accClear:
        TEDelete(sp->hte);
        break;
    }
    HUnlock(Dp->dCtlStorage);
    restore();
    return(0);
}

doevent(sp, ep)
register struct storage *sp;
register EventRecord *ep;
{
    register int c;
    register long l;
    ControlHandle chdl;
    pascal void scrlup(), scrldn();
    long xtol();

    switch(ep->what) {
    case keyDown:
        if ((ep->modifiers & (cmdKey | optionKey)) == 0) {
            c = (char)ep->message;
            if ((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || c == 8) {
                TEKey(c, sp->hte);
                break;
            }
            if ((c == '\r' || c == 3) && (l = xtol(sp->hte)) != -1) {
                sp->where = l;
                TESetSelect(0L, 1000L, sp->hte);
                draw_wind();
                break;
            }
        }
        SysBeep(2);
        break;
    case mouseDown:
        GlobalToLocal(&ep->where);
        if (PtInRect(pass(ep->where), &Cont_rect)&TRUE) {
            if (PtInRect(pass(ep->where), &Edit_rect) & TRUE)
                TEClick(pass(ep->where), ep->modifiers&shiftKey?TRUE:FALSE, sp->hte);
        }
        else {
            c = FindControl(pass(ep->where), Dp->dCtlWindow, &chdl);
            switch(c) {
            case inUpButton:
                TrackControl(chdl, pass(ep->where), scrlup);
                break;
            case inDownButton:
                TrackControl(chdl, pass(ep->where), scrldn);
                break;
            case inPageUp:
                scrlpage(chdl, c, -NLINES);
                break;
            case inPageDown:
                scrlpage(chdl, c, NLINES);
                break;
            case inThumb:
                TrackControl(chdl, pass(ep->where), 0L);
                sp->where = (char *)((long)sp->incr * GetCtlValue(chdl));
                draw_wind();
                break;
            }
        }
        break;
    case activateEvt:
        if (ep->modifiers&1) {
            if (sp->size == 0) {
                signature();
                sp->size = 8;
            }
            InsertMenu(sp->menu, 0);
            ShowControl(sp->vscrl);
            TEActivate(sp->hte);
        }
        else {
            DeleteMenu(MENUID);
            HideControl(sp->vscrl);
            TEDeactivate(sp->hte);
        }
        DrawMenuBar();
        break;
    case updateEvt:
        BeginUpdate(ep->message);
        draw_wind();
        DrawControls(ep->message);
        EndUpdate(ep->message);
        break;
    }
}

signature()
{
    register WindowPtr wp;
    register long tick;
    Rect r;

    wp = Dp->dCtlWindow;
    wp->txFont = 0;
    wp->txSize = 0;
    wp->txMode = srcCopy;
    MoveTo(90, 20);
    DrawString("\Pby");
    MoveTo(50,40);
    DrawString("\PJim Goodnow II");
    MoveTo(55, 60);
    DrawString("\Pusing Aztec C");
    MoveTo(10, 90);
    DrawString("\PManx ");
    Move(0, 20);
    DrawString("\PSoftware ");
    Move(0, 20);
    DrawString("\PSystems");
    Move(0, 20);
    DrawString("\PInc.");
    for (tick=TickCount()+100;TickCount() < tick;)
        ;
    EraseRect(&wp->portRect);
    wp->txFont = 4;
    wp->txSize = 9;
    r = Edit_rect;
    InsetRect(&r, 4, 1);
    SP->hte = TENew(&r, &r);
}

draw_wind()
{
    register unsigned char *cp, *wp;
    register unsigned long l;
    register int k, i, j;
    struct storage *sp;
    char buf[40];
    static char hex[] = "0123456789abcdef";

    sp = SP;
    l = sp->where;
    if (l < 0)
        l = 0;
    if (l > MEMTOP - sp->size*NLINES)
        l = MEMTOP - sp->size * NLINES;
    wp = sp->where = l;
    SetCtlValue(sp->vscrl, (int)(l/sp->incr));
    RectRgn(Dp->dCtlWindow->clipRgn, &Cont_rect);
    MoveTo(4, 9);
    DrawString("\PStart: ");
    TEUpdate(&Edit_rect, sp->hte);
    FrameRect(&Edit_rect);
    for (i=0;i<NLINES;i++) {
        k = i * sp->size;
        MoveTo(4, i*10+24);
        cp = buf;
        l = wp + k;
        for (j=5;j>=0;l>>=4)
            cp[j--] = hex[l%16];
        cp += 6;
        *cp++ = ':';
        if (sp->size == 8) {
            for (j=0;j<8;j++) {
                *cp++ = ' ';
                *cp++ = hex[wp[k+j]/16];
                *cp++ = hex[wp[k+j]%16];
            }
        }
        *cp = 0;
        DrawString(ctop(buf));
        if (sp->size == 16) {
            cp = buf;
            *cp++ = 17;
            *cp++ = ' ';
            for (j=0;j<16;j++) {
                if (wp[k+j] > 0x1f && wp[k+j] < 0x80)
                    *cp++ = wp[k+j];
                else
                    *cp++ = '.';
            }
            DrawString(buf);
        }
    }
    RectRgn(Dp->dCtlWindow->clipRgn, &Full_rect);
}

pascal void
scrlup(chdl, code)
ControlHandle chdl;
int code;
{
    scroll(chdl, code, inUpButton);
}

pascal void
scrldn(chdl, code)
ControlHandle chdl;
int code;
{
    scroll(chdl, code, inDownButton);
}

scroll(chdl, code, where)
ControlHandle chdl;
{
    register struct storage *sp;

#asm
    move.l  a4,-(sp)
    lea     main+(_Uend_-_Dorg_)+(_Cend_-main),a4       ;set up globals
#endasm
    if (code == where) {
        sp = SP;
        if (where == inUpButton)
            sp->where -= sp->size;
        else
            sp->where += sp->size;
        draw_wind();
    }
    ;
#asm
    move.l  (sp)+,a4
#endasm
}

scrlpage(chdl, code, amount)
ControlHandle chdl;
{
    Point pt;
    struct storage *sp;

    sp = SP;
    do {
        GetMouse(&pt);
        if (TestControl(chdl, pass(pt)) == code) {
            sp->where += sp->size*amount;
            draw_wind();
        }
    } while (StillDown()&TRUE);
}

long
xtol(hte)
TEHandle hte;
{
    register char *cp;
    register long i = 0, l = 0;
    register int c, n;

    n = (*hte)->length;
    cp = *(*hte)->hText;
    while (n--) {
        c = *cp++;
        if (c >= 'a' && c <= 'f')
            c -= 'a' - 10;
        else if (c >= '0' && c <= '9')
            c -= '0';
        else {
            TESetSelect(i, i+1, hte);
            return(-1);
        }
        l = l * 16 + c;
        i++;
    }
    return(l);
}
