PROGRAM BDOS_6;  (* Use BDOS Function 6 for console I/O *)

(*  Since the BDOS call requires a pointer parameter   *)
(*  we must define a record that allows us to pass an  *)
(*  INTEGER as a pointer type.  In the record, the     *)
(*  FALSE tagfield occupies the same memory as the two *)
(*  INTEGERS in the TRUE tagfield.                     *)

TYPE
  ptr = ^CHAR;

VAR
  i  : INTEGER;
  ch : CHAR;

  pchar : ptr;


  EXTERNAL FUNCTION _BDOS(func : INTEGER; parm : ptr) : INTEGER;

(* The main program echos any input character *) 
(* at the console until a : is READ           *)

BEGIN (* Main Program *)
  new(pchar);
  REPEAT
    pchar^ := chr(255);

    REPEAT (* Read a character *)
      ch := CHR(_BDOS(6,pchar)); 
    UNTIL ch <> CHR(0);

    IF ch <> ':' THEN
      BEGIN  (* convert ch to INTEGER, pass as a pointer *)
        pchar^ := ch; 
        i := _BDOS(6,pchar);  (* Write a character *)
      END;

  UNTIL ch= ':';
END. (* Main Program *)
