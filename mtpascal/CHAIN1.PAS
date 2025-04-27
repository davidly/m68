  PROGRAM chain_1; (* Program #1 in chain demonstration *)
  TYPE
    comm_area = RECORD
                  i,j,k : INTEGER;
                END;
  VAR
    globals : ABSOLUTE [$8000] comm_area;
    (* this address is arbitrary; *) 
    (* it may not work on your system *)
    chain_file : file;
    title : string;
 
  BEGIN (* Main program #1 *)
    title := 'F:CHAIN2.68K';
    WITH globals DO
      BEGIN
        i := 3;
        j := 3;
        k := I * J
      END;
    assign(chain_file,title);
    CHAIN(chain_file)
  END. (* End chain_1 *)
