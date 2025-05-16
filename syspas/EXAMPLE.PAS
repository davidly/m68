program yyy;
	var i: longint;
	procedure callme;
	begin
		writeln('I got called');
	end;
begin
	i := 2001;
	writeln(i);
	callme;
end.
