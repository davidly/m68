PROGRAM peek_poke;

TYPE
  byte_ptr = ^BYTE;

  pointerkludge = RECORD
                    CASE BOOLEAN OF
                      TRUE : (p : byte_ptr);
                      FALSE: (q : LONGINT)
                  END;
VAR
  choice : INTEGER;
  bbb : BYTE;
  ppp : pointerkludge;

EXTERNAL PROCEDURE poke(b : BYTE; p : byte_ptr);

EXTERNAL FUNCTION peek(p : byte_ptr) : BYTE;

BEGIN (* Main Program *)
  REPEAT
    WRITE('Which address? ');
    READLN(ppp.q);
    WRITE('1) Peek    2) Poke    3) Exit  ');
    READLN(choice);
    IF choice = 1 THEN
      WRITELN(ppp.q,' contains ',peek(ppp.p))
    ELSE
      IF choice = 2 THEN
        BEGIN
          WRITE('Enter byte of data: ');
          READLN(bbb);
          POKE(bbb,ppp.p)
        END
  UNTIL choice = 3
END. (* Main Program *)
