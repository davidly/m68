integer b, n, c, s, r, sa, st, lc, lr, h
rem b  -- the board
rem c  -- current column
rem r  -- current row
rem s  -- solution count
rem n  -- current number of columns to solve for
rem h  -- hightest board dimension to solve for
rem st -- stack to save current r when recursing
rem sa -- returns if a board position is safe
rem lr and lc -- local r and c in the "is safe" function

1 h = 8
10 dim b(h)
20 dim st(h)
21 for n = 0 to h-1
22    b(n) = 255
23 next n
30 for n = 1 to h
40     c = 0
50     s = 0
60     gosub 100
70     print "solutions for n "; n; ": "; s
80     next n
90 goto 5000

100 if c = n then s = s + 1 : return
130 r = 0
140     gosub 500
200     if 0 = sa then goto 300
210         b(r) = c
220         st(c) = r
230         c = c + 1
240         gosub 100
250         c = c - 1
260         r = st(c)
270         b(r) = 255
300     r = r + 1
310     if r < n then goto 140
320 return

500 sa = 0
510 if 255 <> b(r) then return
530 lr = r - 1
540 lc = c - 1
550     if lr < 0 or lc < 0 then goto 600
560     if b(lr) = lc then return
570     lr = lr - 1
580     lc = lc - 1
590     goto 550
600 lr = r + 1
610 lc = c - 1
620     if lr = n or lc < 0 then goto 670
630     if b(lr) = lc then return
640     lr = lr + 1
650     lc = lc - 1
660     goto 620
670 sa = 1
680 return

5000 end

