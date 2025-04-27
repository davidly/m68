PROGRAM file_transfer_2;

(*-----------------------------------------------*)   
(* Transfer file_a to file_b using GNB and WNB   *)   
(*-----------------------------------------------*)   

CONST   
  buffer_size = 2047;   

TYPE   
  paoc = ARRAY[1..buffer_size] OF CHAR;   
  text_file = FILE OF paoc;   
  char_file = FILE OF CHAR;   

VAR   
  file_a : text_file;   
  file_b : char_file;   
    name : STRING;   

PROCEDURE transfer(VAR source: text_file; VAR destination : char_file);

VAR   
  ch : CHAR;   
  result : INTEGER;   
  stop_it  : BOOLEAN;   

BEGIN (* Body of TRANSFER procedure *)
  stop_it := FALSE;   
  WHILE (NOT EOF(source)) AND (NOT stop_it) DO   
    BEGIN   
      ch := GNB(source);   
      IF WNB(destination,ch) THEN   
         BEGIN   
           WRITELN('Error writing character');   
           stop_it := TRUE;
         END;   
    END;

  CLOSE(destination,result);   
  IF result = 255 THEN   
     WRITELN('Error closing ')   

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

  transfer(file_a,file_b) (* Call TRANSFER procedure *)

END. (* Main Program *)
