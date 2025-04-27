  PROGRAM chain_2; (* Program #2 in chain demonstration *)
    
  TYPE
    comm_area = RECORD
                  i,j,k : INTEGER
                END;
  VAR
    globals : ABSOLUTE [$8000] comm_area;

  BEGIN (* Main program #2 *)
    WITH globals DO
      WRITELN('Result of ',i,' times ',j,' is =', k)

  END. (* End chain_2; return to operating system *)
