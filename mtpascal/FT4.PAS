PROGRAM file_transfer_4;

(*---------------------------------------------*)   
(* Transfer file_a to file_b using GET and PUT *)   
(*---------------------------------------------*)

TYPE   
  char_file = FILE OF CHAR;   

VAR   
  file_a,file_b : char_file;   
           name : STRING;   

PROCEDURE transfer(VAR source: char_file; VAR destination : char_file);   

VAR   
  result : INTEGER;   

BEGIN (* Body of TRANSFER procedure *)
  WHILE NOT EOF(source) DO   
    BEGIN   
      destination^ := source^;   
      PUT(destination);   
      GET(source);
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
