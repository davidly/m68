PROGRAM pprime;   (* Uses sieve of Eratosthenes *)

CONST   
  size = 8190;   

VAR
  flags     : ARRAY[0..size] OF BOOLEAN;
  i,k       : INTEGER;
  prime     : INTEGER;
  count     : INTEGER;
  iteration : INTEGER;

BEGIN  (* Main Program *)
  count := 0;   
  writeln('Do 10 iterations');
  
  FOR iteration := 1 TO 10 DO   
    BEGIN   
      count := 0;

      FILLCHAR(flags,SIZEOF(flags),CHR(TRUE));

      FOR i := 0 TO size DO
        IF flags[i] THEN
           BEGIN
             prime := i + i + 3;
             k := i + prime;
             WHILE k <= size DO
             BEGIN
               flags[k] := FALSE;
               k := k + prime;
             END;
             count := count + 1;
           END
    END;
    WRITELN(count,' Primes');   
END.  (Main Program *)

