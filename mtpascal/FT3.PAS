PROGRAM file_transfer_3;

(*--------------------------------------------------------*)   
(* Transfer file_a to file_b using SEEKREAD and SEEKWRITE *)   
(*--------------------------------------------------------*)   

CONST   
  buffer_size = 2047;   

TYPE   
  paoc = ARRAY[0..buffer_size] OF CHAR;   
  text_file = FILE OF paoc;   
  char_file = FILE OF paoc;   

VAR   
  file_a : text_file;   
  file_b : text_file;   
    name : STRING;   

PROCEDURE transfer(VAR source: text_file; VAR destination : text_file);

VAR   
  result,i : INTEGER;   
  stop_it  : BOOLEAN;
        ch : CHAR;   

BEGIN (* Body of TRANSFER procedure *)
  ch := 'A';   
  result := 0;   
  i := 0;   
  WHILE result <> 1 DO   
    BEGIN   
      SEEKREAD(source,i);   
      result := IORESULT;   
      IF result = 0 THEN   
         BEGIN   
           destination^ := source^;   
           SEEKWRITE(destination,i);   
         END;   
      i := i + 1;   
    END;   

  CLOSE(destination,result);   
  IF result = 255 THEN   
     WRITELN('Error closing destination file')   

END; (* TRANSFER procedure *)

BEGIN (* Main Program *)

  WRITE('Name of Source File ? ');   
  READLN(name);   
  ASSIGN(file_a,name);   
  RESET(file_a);   
  IF IORESULT = 255 THEN   
     BEGIN   
       WRITELN('Sorry, cannot open ',name);   
       EXIT   
     END;   
  
  WRITE('Name of Destination File ? ');   
  READLN(name);   
  ASSIGN(file_b,name);   
  REWRITE(file_b);
  IF IORESULT = 255 THEN   
     BEGIN   
       WRITELN('Sorry, cannot open ',name);   
       EXIT   
     END;   

  transfer(file_a,file_b) (* Call specific TRANSFER procedure *)

END. (* Main Program *)
