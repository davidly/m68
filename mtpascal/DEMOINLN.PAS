PROGRAM demo_inline;

  CONST
    element = 3;   {Third array element}
 
  TYPE
    id_field = ARRAY [1..8] OF CHAR;
    id_ptr   = ^id_field;
    pointerkludge = RECORD
                      CASE BOOLEAN OF
                        TRUE : (p : id_ptr);
                        FALSE: (l : longint);
                    END;
 
  VAR
    table_ptr : id_ptr;
            p : pointerkludge;
            offset : integer;
 
  PROCEDURE table;
  
  BEGIN
    INLINE('Digital ' /'Research' /'Software');
  END;

  BEGIN (* Main Program *)

    new(table_ptr);
    p.p := ADDR(table);
    p.l := p.l + #14;
    offset := sizeof(table_ptr^) * (element - 1);
    p.l := p.l + long(offset);
    table_ptr := p.p;
    WRITELN(table_ptr^); (* This should write 'Software' *)
 
  END. (* Main Program *)
