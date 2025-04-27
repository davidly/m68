PROGRAM file_transfer_1;

(*----------------------------------------------------------*)
(* Transfer file_a to file_b using BLOCKREAD and BLOCKWRITE *)
(*----------------------------------------------------------*)

CONST   
  buffer_size = 2047;   

TYPE   
  paoc = ARRAY[0..buffer_size] OF CHAR;   
  fyle = FILE;   

VAR   
  file_a,file_b : fyle;   
           name : STRING;   
         buffer : paoc;   

PROCEDURE transfer(VAR source : fyle; VAR destination : fyle);

VAR   
   result,i : INTEGER;   
       quit : BOOLEAN;   

BEGIN (* Body of TRANSFER procedure *)   
   i := -1;
   REPEAT   
        BLOCKREAD(source,buffer,result,SIZEOF(buffer),i);
        IF   result = 0
        THEN BLOCKWRITE(destination,buffer,result,SIZEOF(buffer),i)
        ELSE quit := TRUE;   
   UNTIL quit;
   CLOSE(destination,result);
   IF   result = 255
   THEN WRITELN('Error closing destination file')   
END; (* TRANSFER procedure *)

BEGIN (* Main Program *)
   WRITE('Name of Source File ? ');   
   READLN(name);   
   ASSIGN(file_a,name);   
   RESET(file_a);   
   IF IORESULT = 255   
      THEN BEGIN   
             WRITELN('Sorry, cannot open ',name);   
             EXIT   
           END;   
   WRITE('Name of Destination File ? ');   
   READLN(name);   
   ASSIGN(file_b,name);   
   REWRITE(file_b);
   IF IORESULT = 255   
      THEN BEGIN   
             WRITELN('Sorry, cannot open ',name);   
             EXIT   
           END;   
   transfer(file_a,file_b) (* Call TRANSFER procedure *)
END. (* Main Program *)
