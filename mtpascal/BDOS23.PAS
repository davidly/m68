PROGRAM BDOS_23;  (* Use BDOS Function 23 to rename files *)

TYPE
  ptr = ^INTEGER;
  fcblk = PACKED ARRAY [0..36] OF CHAR;

VAR
  oldname,newname : STRING;
            f1,f2 : fcblk;
                i : INTEGER;
  
  EXTERNAL FUNCTION @BDOS(func : INTEGER; parm : ptr) : INTEGER;
  
  EXTERNAL PROCEDURE @PARSE(VAR f : fcblk; s : STRING);
  
  (* @PARSE converts a string into internal *)
  (*  CP/M filename format *)

BEGIN (* Main Program *)
  WRITE('Enter old filename: '); (* Get the old filename *)
  READLN(oldname);
  @PARSE(f1,oldname);

  WRITE('Enter new filename: '); (* Get the new filename *)
  READLN(newname);
  @PARSE(f2,newname);

  (* Create the FCB required by BDOS call 23 *)
  
  MOVE(f2,f1[16],12);  

  (* Now call the rename function passing a pointer to the FCB *)
  (* containing the old and new filenames *)

  IF @BDOS(23,ADDR(f1)) = 255 THEN 
    WRITELN('Rename failed. ',oldname,' not found.') 
  ELSE
    WRITELN('File ',oldname,' renamed to ',newname);

END. (* Main Program *)
